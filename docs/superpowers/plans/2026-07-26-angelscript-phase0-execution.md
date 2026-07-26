# AngelScript Phase 0 Execution — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Compile and execute an AngelScript `.as` script on an O3DE entity — its class is instantiated and its `OnCreate()` / `OnTick(float)` / `OnDestroy()` methods run.

**Architecture:** Source-compiled-at-runtime. The builder stores `.as` source in the asset; at runtime the system component compiles it into a named module via the vendored `CScriptBuilder` add-on, hands out execution contexts from a small pool, and the per-entity `AngelScriptComponent` instantiates the script class and drives its lifecycle. Two new focused units — `ScriptContextPool` and `CompileModuleFromSource` — are unit-tested directly; the end-to-end execution core is proven by a gtest before any Editor wiring.

**Tech Stack:** C++17, O3DE 26.05 SDK (VS 2022, profile config), AngelScript 2.37.0 (vendored, generic build), `CScriptBuilder` add-on, O3DE AzTest/googletest.

## Global Constraints

- **Engine build/run commands** (run from anywhere; paths absolute):
  - Build a target: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target <TARGET> --config profile`
  - Reconfigure (after CMake file edits): `cmake -B C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -S C:/Users/Clayton/O3DE/Projects/PlatformerTest`
  - List tests: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -N`
  - Run gem tests: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
- **profile config links the Release AngelScript lib** (`angelscript64.lib`) via the `$<IF:$<CONFIG:Debug>,d,>` expression — do not switch configs mid-stream.
- **Gem source lives in** `C:/Users/Clayton/Source/o3degems_public/AngelScript` (repo root `C:/Users/Clayton/Source/o3degems_public`, branch `phase0-hello-world`). Commit there.
- **New runtime helper files go in** `AngelScript/Code/Source/` (already on the `.Private.Object` include path) and get added to `AngelScript/Code/angelscript_private_files.cmake`.
- **Keep third-party headers out of our headers:** include `<scriptbuilder/scriptbuilder.h>` and `<angelscript.h>` only in `.cpp` files; forward-declare `asIScript*` types in our headers.
- **Commit after each task** with a `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>` trailer.
- **Class-name convention:** a script's class name equals its module name equals the `.as` filename stem.

---

### Task 1: Wire the `scriptbuilder` add-on into the gem build

**Files:**
- Modify: `AngelScript/3rdParty/FindAngelScript.cmake:16`
- Modify: `AngelScript/Code/angelscript_private_files.cmake`

**Interfaces:**
- Consumes: nothing (build-system only).
- Produces: `<scriptbuilder/scriptbuilder.h>` becomes includable in gem `.cpp` files, and `CScriptBuilder` symbols are compiled into `Gem::AngelScript.Private.Object` (and thus available transitively to the runtime module and, through `.Editor.Private.Object`, to the builder).

- [ ] **Step 1: Add the add-on include directory to the AngelScript external target**

In `AngelScript/3rdParty/FindAngelScript.cmake`, change line 16 from:

```cmake
    INCLUDE_DIRECTORIES sdk/angelscript/include
```
to:
```cmake
    INCLUDE_DIRECTORIES sdk/angelscript/include sdk/add_on
```

- [ ] **Step 2: Add the add-on source to the runtime object library, skipping unity build**

In `AngelScript/Code/angelscript_private_files.cmake`, replace the whole file with:

```cmake

set(FILES
    Source/AngelScriptModuleInterface.cpp
    Source/AngelScriptModuleInterface.h
    Source/Clients/AngelScriptSystemComponent.cpp
    Source/Clients/AngelScriptSystemComponent.h
    Source/AngelScriptAsset.cpp

    Source/AngelScriptComponent.h
    Source/AngelScriptComponent.cpp
    Source/AngelScriptAssetHandler.h
    Source/AngelScriptAssetHandler.cpp

# Preprocessor
    Source/Preprocessor/AngelScriptPreprocessor.h
    Source/Preprocessor/AngelScriptPreprocessor.cpp

# Vendored AngelScript add-ons (compiled into the gem)
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.h
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp
)

# scriptbuilder is third-party; keep it out of the gem's unity blob to avoid
# symbol collisions and warning-as-error churn.
set(SKIP_UNITY_BUILD_INCLUSION_FILES
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp
)
```

- [ ] **Step 3: Reconfigure**

Run: `cmake -B C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -S C:/Users/Clayton/O3DE/Projects/PlatformerTest`
Expected: configures with no errors; still prints `ANGELSCRIPT: 2.37.0`.

- [ ] **Step 4: Build the runtime object library to confirm the add-on compiles**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Private.Object --config profile`
Expected: build succeeds; `scriptbuilder.cpp` appears in the compile output.

- [ ] **Step 5: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/3rdParty/FindAngelScript.cmake AngelScript/Code/angelscript_private_files.cmake
git commit -m "Wire scriptbuilder add-on into the gem build"
```

---

### Task 2: `ScriptContextPool` — reusable execution-context free-list

**Files:**
- Create: `AngelScript/Code/Source/ScriptContextPool.h`
- Create: `AngelScript/Code/Source/ScriptContextPool.cpp`
- Modify: `AngelScript/Code/angelscript_private_files.cmake` (add the two files)
- Create: `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp`
- Modify: `AngelScript/Code/angelscript_tests_files.cmake` (add the test file)

**Interfaces:**
- Consumes: nothing new.
- Produces:
  ```cpp
  namespace AngelScript {
      class ScriptContextPool {
      public:
          void Initialize(asIScriptEngine* engine); // set owning engine
          void Shutdown();                            // Release() all pooled contexts, clear
          asIScriptContext* Acquire();                // pop idle or create; nullptr if no engine
          void Release(asIScriptContext* context);    // Unprepare() and return to pool
      };
  }
  ```

- [ ] **Step 1: Preflight — confirm the test target builds and runs on this SDK**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Then: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -N | grep -i angelscript`
Expected: the target builds and `ctest -N` lists an `AngelScript.Tests` entry. If the installed SDK does not support building test modules (target missing or link errors referencing AzTest), STOP and report — the fallback is to verify the execution core via a standalone console target instead of gtest; do not proceed silently.

- [ ] **Step 2: Write the failing test**

Create `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp`:

```cpp
#include <AzTest/AzTest.h>
#include <angelscript.h>
#include <ScriptContextPool.h>

namespace AngelScriptTests
{
    // Fixture owns a bare AngelScript engine for hermetic, app-free unit tests.
    class AngelScriptExecutionFixture : public ::testing::Test
    {
    protected:
        void SetUp() override { m_engine = asCreateScriptEngine(); }
        void TearDown() override { if (m_engine) { m_engine->ShutDownAndRelease(); m_engine = nullptr; } }
        asIScriptEngine* m_engine = nullptr;
    };

    TEST_F(AngelScriptExecutionFixture, ContextPool_ReusesReturnedContext)
    {
        AngelScript::ScriptContextPool pool;
        pool.Initialize(m_engine);

        asIScriptContext* a = pool.Acquire();
        EXPECT_NE(a, nullptr);
        pool.Release(a);

        // The next Acquire should hand back the same (recycled) context object.
        asIScriptContext* b = pool.Acquire();
        EXPECT_EQ(a, b);

        pool.Release(b);
        pool.Shutdown();
    }

    TEST_F(AngelScriptExecutionFixture, ContextPool_NoEngineReturnsNull)
    {
        AngelScript::ScriptContextPool pool; // never Initialize()d
        EXPECT_EQ(pool.Acquire(), nullptr);
    }
}
```

- [ ] **Step 3: Register the new test file**

In `AngelScript/Code/angelscript_tests_files.cmake`, change the `FILES` list to:

```cmake

set(FILES
    Tests/Clients/AngelScriptTest.cpp
    Tests/Clients/AngelScriptExecutionTests.cpp
)
```

- [ ] **Step 4: Run the test to verify it fails to build**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Expected: FAIL — `ScriptContextPool.h` not found / unresolved `ScriptContextPool`.

- [ ] **Step 5: Create the header**

Create `AngelScript/Code/Source/ScriptContextPool.h`:

```cpp
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>

class asIScriptEngine;
class asIScriptContext;

namespace AngelScript
{
    //! A small free-list of reusable asIScriptContext objects for one engine.
    //! Acquire() hands out an idle context (or creates one); Release() returns it.
    class ScriptContextPool
    {
    public:
        ScriptContextPool() = default;
        ~ScriptContextPool();

        void Initialize(asIScriptEngine* engine);
        void Shutdown();

        asIScriptContext* Acquire();
        void Release(asIScriptContext* context);

    private:
        asIScriptEngine* m_engine = nullptr;
        AZStd::vector<asIScriptContext*> m_free;
        AZStd::mutex m_mutex;
    };
} // namespace AngelScript
```

- [ ] **Step 6: Create the implementation**

Create `AngelScript/Code/Source/ScriptContextPool.cpp`:

```cpp
#include <ScriptContextPool.h>
#include <angelscript.h>

namespace AngelScript
{
    ScriptContextPool::~ScriptContextPool()
    {
        Shutdown();
    }

    void ScriptContextPool::Initialize(asIScriptEngine* engine)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_engine = engine;
    }

    void ScriptContextPool::Shutdown()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        for (asIScriptContext* ctx : m_free)
        {
            ctx->Release();
        }
        m_free.clear();
        m_engine = nullptr;
    }

    asIScriptContext* ScriptContextPool::Acquire()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        if (!m_engine)
        {
            return nullptr;
        }
        if (!m_free.empty())
        {
            asIScriptContext* ctx = m_free.back();
            m_free.pop_back();
            return ctx;
        }
        return m_engine->CreateContext();
    }

    void ScriptContextPool::Release(asIScriptContext* context)
    {
        if (!context)
        {
            return;
        }
        context->Unprepare();
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_free.push_back(context);
    }
} // namespace AngelScript
```

- [ ] **Step 7: Add the pool files to the object library**

In `AngelScript/Code/angelscript_private_files.cmake`, add these two lines inside `set(FILES ...)` (just after the `AngelScriptAssetHandler.cpp` line):

```cmake
    Source/ScriptContextPool.h
    Source/ScriptContextPool.cpp
```

- [ ] **Step 8: Reconfigure, build, and run the tests**

Run: `cmake -B C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -S C:/Users/Clayton/O3DE/Projects/PlatformerTest`
Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Run: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
Expected: PASS (both `ContextPool_*` tests).

- [ ] **Step 9: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Source/ScriptContextPool.h AngelScript/Code/Source/ScriptContextPool.cpp AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp AngelScript/Code/angelscript_private_files.cmake AngelScript/Code/angelscript_tests_files.cmake
git commit -m "Add ScriptContextPool with unit tests"
```

---

### Task 3: `CompileModuleFromSource` — compile `.as` source into a module

**Files:**
- Create: `AngelScript/Code/Source/ScriptModuleCompiler.h`
- Create: `AngelScript/Code/Source/ScriptModuleCompiler.cpp`
- Modify: `AngelScript/Code/angelscript_private_files.cmake` (add the two files)
- Modify: `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp` (add tests)

**Interfaces:**
- Consumes: `<angelscript.h>` engine; `<scriptbuilder/scriptbuilder.h>` (Task 1).
- Produces:
  ```cpp
  namespace AngelScript {
      // Compiles `source` into a module named `moduleName` (replacing any existing
      // module of that name). Returns the module, or nullptr on compile failure.
      asIScriptModule* CompileModuleFromSource(
          asIScriptEngine* engine, const char* moduleName, const char* source);
  }
  ```

- [ ] **Step 1: Write the failing tests**

Append to `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp` (inside `namespace AngelScriptTests`, before its closing brace), and add `#include <ScriptModuleCompiler.h>` to the top includes:

```cpp
    TEST_F(AngelScriptExecutionFixture, Compile_ValidSourceProducesModuleWithClass)
    {
        const char* source =
            "class Hello {\n"
            "  void OnCreate() {}\n"
            "}\n";
        asIScriptModule* module =
            AngelScript::CompileModuleFromSource(m_engine, "Hello", source);
        ASSERT_NE(module, nullptr);
        EXPECT_NE(module->GetTypeInfoByDecl("Hello"), nullptr);
    }

    TEST_F(AngelScriptExecutionFixture, Compile_InvalidSourceReturnsNull)
    {
        const char* source = "class Broken { this is not valid angelscript }";
        asIScriptModule* module =
            AngelScript::CompileModuleFromSource(m_engine, "Broken", source);
        EXPECT_EQ(module, nullptr);
    }
```

- [ ] **Step 2: Run the tests to verify they fail to build**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Expected: FAIL — `ScriptModuleCompiler.h` not found / unresolved `CompileModuleFromSource`.

- [ ] **Step 3: Create the header**

Create `AngelScript/Code/Source/ScriptModuleCompiler.h`:

```cpp
#pragma once

class asIScriptEngine;
class asIScriptModule;

namespace AngelScript
{
    //! Compile AngelScript `source` into a module named `moduleName` on `engine`,
    //! replacing any existing module of that name. Returns the compiled module,
    //! or nullptr on failure (compiler diagnostics go to the engine message callback).
    asIScriptModule* CompileModuleFromSource(
        asIScriptEngine* engine, const char* moduleName, const char* source);
} // namespace AngelScript
```

- [ ] **Step 4: Create the implementation**

Create `AngelScript/Code/Source/ScriptModuleCompiler.cpp`:

```cpp
#include <ScriptModuleCompiler.h>

#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>

namespace AngelScript
{
    asIScriptModule* CompileModuleFromSource(
        asIScriptEngine* engine, const char* moduleName, const char* source)
    {
        if (!engine || !moduleName || !source)
        {
            return nullptr;
        }

        CScriptBuilder builder;
        // asGM_ALWAYS_CREATE inside StartNewModule discards any existing module of this name.
        if (builder.StartNewModule(engine, moduleName) < 0)
        {
            return nullptr;
        }
        if (builder.AddSectionFromMemory(moduleName, source) < 0)
        {
            engine->DiscardModule(moduleName);
            return nullptr;
        }
        if (builder.BuildModule() < 0)
        {
            engine->DiscardModule(moduleName);
            return nullptr;
        }
        return builder.GetModule();
    }
} // namespace AngelScript
```

- [ ] **Step 5: Add the compiler files to the object library**

In `AngelScript/Code/angelscript_private_files.cmake`, add inside `set(FILES ...)` (just after the `ScriptContextPool.cpp` line):

```cmake
    Source/ScriptModuleCompiler.h
    Source/ScriptModuleCompiler.cpp
```

- [ ] **Step 6: Reconfigure, build, and run the tests**

Run: `cmake -B C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -S C:/Users/Clayton/O3DE/Projects/PlatformerTest`
Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Run: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
Expected: PASS (both `Compile_*` tests, plus the earlier pool tests).

- [ ] **Step 7: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Source/ScriptModuleCompiler.h AngelScript/Code/Source/ScriptModuleCompiler.cpp AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp AngelScript/Code/angelscript_private_files.cmake
git commit -m "Add CompileModuleFromSource with unit tests"
```

---

### Task 4: Execution-core test (milestone 0a) — instantiate a class and run its lifecycle

**Files:**
- Modify: `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp` (add the end-to-end test)

**Interfaces:**
- Consumes: `ScriptContextPool` (Task 2), `CompileModuleFromSource` (Task 3), AngelScript engine API.
- Produces: proof that compile → instantiate → `OnCreate`/`OnTick` dispatch works. No new production symbols.

- [ ] **Step 1: Write the failing end-to-end test**

Append to `AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp` (inside `namespace AngelScriptTests`). It registers a native `void RecordCall()` that a script calls from its lifecycle methods, then drives the object exactly as the component will:

```cpp
    namespace
    {
        int s_recordCallCount = 0;
        void RecordCall() { ++s_recordCallCount; }
    }

    TEST_F(AngelScriptExecutionFixture, ExecutionCore_InstantiateAndRunLifecycle)
    {
        s_recordCallCount = 0;

        // Native function the script can call to record an observable side-effect.
        ASSERT_GE(m_engine->RegisterGlobalFunction(
            "void RecordCall()", asFUNCTION(RecordCall), asCALL_CDECL), 0);

        const char* source =
            "class Hello {\n"
            "  void OnCreate() { RecordCall(); }\n"
            "  void OnTick(float dt) { RecordCall(); }\n"
            "}\n";

        asIScriptModule* module =
            AngelScript::CompileModuleFromSource(m_engine, "Hello", source);
        ASSERT_NE(module, nullptr);

        asITypeInfo* type = module->GetTypeInfoByDecl("Hello");
        ASSERT_NE(type, nullptr);

        AngelScript::ScriptContextPool pool;
        pool.Initialize(m_engine);

        // Instantiate via the type's factory.
        asIScriptContext* ctx = pool.Acquire();
        ASSERT_NE(ctx, nullptr);
        ASSERT_GE(ctx->Prepare(type->GetFactoryByIndex(0)), 0);
        ASSERT_EQ(ctx->Execute(), asEXECUTION_FINISHED);
        asIScriptObject* obj = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
        ASSERT_NE(obj, nullptr);
        obj->AddRef();
        pool.Release(ctx);

        // Call OnCreate().
        asIScriptFunction* onCreate = type->GetMethodByDecl("void OnCreate()");
        ASSERT_NE(onCreate, nullptr);
        ctx = pool.Acquire();
        ctx->Prepare(onCreate);
        ctx->SetObject(obj);
        ASSERT_EQ(ctx->Execute(), asEXECUTION_FINISHED);
        pool.Release(ctx);

        // Call OnTick(0.016).
        asIScriptFunction* onTick = type->GetMethodByDecl("void OnTick(float)");
        ASSERT_NE(onTick, nullptr);
        ctx = pool.Acquire();
        ctx->Prepare(onTick);
        ctx->SetObject(obj);
        ctx->SetArgFloat(0, 0.016f);
        ASSERT_EQ(ctx->Execute(), asEXECUTION_FINISHED);
        pool.Release(ctx);

        obj->Release();
        pool.Shutdown();

        EXPECT_EQ(s_recordCallCount, 2); // OnCreate + OnTick
    }
```

- [ ] **Step 2: Build and run — verify it passes**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Tests --config profile`
Run: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
Expected: PASS — `ExecutionCore_InstantiateAndRunLifecycle` green (counter == 2). **This is milestone 0a.**

- [ ] **Step 3: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Tests/Clients/AngelScriptExecutionTests.cpp
git commit -m "Add end-to-end AngelScript execution-core test (milestone 0a)"
```

---

### Task 5: Expose the pool and module-compile on the system component + bus

**Files:**
- Modify: `AngelScript/Code/Include/AngelScript/AngelScriptBus.h` (add 3 virtual methods)
- Modify: `AngelScript/Code/Source/Clients/AngelScriptSystemComponent.h` (pool member + overrides)
- Modify: `AngelScript/Code/Source/Clients/AngelScriptSystemComponent.cpp` (implement; init/shutdown pool)

**Interfaces:**
- Consumes: `ScriptContextPool` (Task 2), `CompileModuleFromSource` (Task 3).
- Produces (on `AngelScriptRequestBus` / `AngelScriptInterface`):
  ```cpp
  virtual asIScriptContext* RequestContext() = 0;
  virtual void ReturnContext(asIScriptContext* context) = 0;
  virtual asIScriptModule* EnsureModule(const AZStd::string& moduleName,
                                        const AZStd::string& source) = 0;
  ```

- [ ] **Step 1: Add the three methods to the bus interface**

In `AngelScript/Code/Include/AngelScript/AngelScriptBus.h`, inside `class AngelScriptRequests`, after the existing `GetModule` declaration (around line 35), add:

```cpp
        /// @brief Borrow an execution context from the pool (return it with ReturnContext).
        virtual asIScriptContext* RequestContext() = 0;

        /// @brief Return a context previously obtained from RequestContext.
        virtual void ReturnContext(asIScriptContext* context) = 0;

        /// @brief Get the module named moduleName, compiling it from source if absent.
        /// @return The module, or nullptr on compile failure.
        virtual asIScriptModule* EnsureModule(const AZStd::string& moduleName,
                                              const AZStd::string& source) = 0;
```

Also add near the top of the file, next to the other forward declarations, if not already present: `class asIScriptModule;` (it is already declared — verify).

- [ ] **Step 2: Declare the overrides and the pool member on the system component**

In `AngelScript/Code/Source/Clients/AngelScriptSystemComponent.h`:

Add the include near the other Source includes (top of file, after `#include <angelscript.h>`):
```cpp
#include <ScriptContextPool.h>
```

In the `AngelScriptRequestBus::Handler interface implementation` block (after the `GetModule` override, ~line 68), add:
```cpp
        asIScriptContext* RequestContext() override;
        void ReturnContext(asIScriptContext* context) override;
        asIScriptModule* EnsureModule(const AZStd::string& moduleName, const AZStd::string& source) override;
```

In the `private:` data members (near `m_scriptContext`, ~line 44), add:
```cpp
        ScriptContextPool m_contextPool;
```

- [ ] **Step 3: Implement the overrides and wire pool lifecycle**

In `AngelScript/Code/Source/Clients/AngelScriptSystemComponent.cpp`:

Add includes near the top (after the existing `#include <angelscript.h>`):
```cpp
#include <ScriptContextPool.h>
#include <ScriptModuleCompiler.h>
```

In `InitializeAngelScriptEngine()`, immediately after the engine is created and the message callback is set (right before the `#define AS_SET_ENGINE_PROPERTY` block), add:
```cpp
        m_contextPool.Initialize(m_scriptEngine);
```

In `ShutdownAngelScriptEngine()`, inside the `if (m_scriptEngine)` block, before `m_scriptEngine->ShutDownAndRelease();`, add:
```cpp
            m_contextPool.Shutdown();
```

Add these three method definitions (place them next to `GetModule`, before `ExecuteString`):
```cpp
    asIScriptContext* AngelScriptSystemComponent::RequestContext()
    {
        return m_contextPool.Acquire();
    }

    void AngelScriptSystemComponent::ReturnContext(asIScriptContext* context)
    {
        m_contextPool.Release(context);
    }

    asIScriptModule* AngelScriptSystemComponent::EnsureModule(
        const AZStd::string& moduleName, const AZStd::string& source)
    {
        if (!m_scriptEngine)
        {
            return nullptr;
        }
        if (asIScriptModule* existing = m_scriptEngine->GetModule(moduleName.c_str(), asGM_ONLY_IF_EXISTS))
        {
            return existing;
        }
        return CompileModuleFromSource(m_scriptEngine, moduleName.c_str(), source.c_str());
    }
```

- [ ] **Step 4: Build the runtime module to confirm compile + link**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript --config profile`
Expected: builds and links (all three pure-virtuals now implemented, so `AngelScriptSystemComponent` is concrete).

- [ ] **Step 5: Run the gem tests to confirm no regressions**

Run: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
Expected: PASS (all prior tests still green).

- [ ] **Step 6: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Include/AngelScript/AngelScriptBus.h AngelScript/Code/Source/Clients/AngelScriptSystemComponent.h AngelScript/Code/Source/Clients/AngelScriptSystemComponent.cpp
git commit -m "Expose context pool + EnsureModule on the AngelScript system component and bus"
```

---

### Task 6: Wire per-entity execution in `AngelScriptComponent`

**Files:**
- Modify: `AngelScript/Code/Source/AngelScriptComponent.cpp` (`CreateScriptObject`, `OnTick`, `DestroyScriptObject`)

**Interfaces:**
- Consumes: `AngelScriptRequestBus` `RequestContext` / `ReturnContext` / `EnsureModule` / `GetScriptEngine` (Task 5); `AngelScriptAsset::m_scriptData.m_script` + `m_moduleName`.
- Produces: an `AngelScriptComponent` that, given a ready asset, instantiates the script class and runs `OnCreate`/`OnTick`/`OnDestroy`.

- [ ] **Step 1: Replace `CreateScriptObject` with the source-compiling, pool-using implementation**

In `AngelScript/Code/Source/AngelScriptComponent.cpp`, replace the entire body of `CreateScriptObject()` with:

```cpp
    void AngelScriptComponent::CreateScriptObject()
    {
        DestroyScriptObject(); // Clean up any existing object first.

        if (!m_scriptAsset.IsReady())
        {
            return;
        }

        // Compile (or fetch) the module from the asset's stored source.
        const AZStd::vector<char>& scriptBuffer = m_scriptAsset.Get()->m_scriptData.m_script;
        const AZStd::string source(scriptBuffer.data(), scriptBuffer.size());
        const AZStd::string& moduleName = m_scriptAsset.Get()->m_moduleName;

        asIScriptModule* module = nullptr;
        AngelScriptRequestBus::BroadcastResult(module, &AngelScriptRequestBus::Events::EnsureModule, moduleName, source);
        if (!module)
        {
            AZ_Error("AngelScript", false, "Failed to compile module '%s' for entity %s",
                moduleName.c_str(), GetEntityId().ToString().c_str());
            return;
        }

        // Convention: class name == module name == filename stem.
        AZStd::string className = moduleName;
        AZStd::string::size_type dotPos = className.rfind('.');
        if (dotPos != AZStd::string::npos)
        {
            className = className.substr(0, dotPos);
        }

        asITypeInfo* type = module->GetTypeInfoByDecl(className.c_str());
        if (!type)
        {
            AZ_Error("AngelScript", false, "Class '%s' not found in module '%s'.", className.c_str(), module->GetName());
            return;
        }

        // Instantiate via the type's factory.
        asIScriptContext* ctx = nullptr;
        AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
        if (!ctx)
        {
            AZ_Error("AngelScript", false, "No script context available to instantiate '%s'.", className.c_str());
            return;
        }

        ctx->Prepare(type->GetFactoryByIndex(0));
        if (ctx->Execute() == asEXECUTION_FINISHED)
        {
            m_scriptObject = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
            m_scriptObject->AddRef();
        }
        AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);

        if (!m_scriptObject)
        {
            AZ_Error("AngelScript", false, "Failed to instantiate script object for class '%s'", className.c_str());
            return;
        }

        // Cache lifecycle methods.
        m_onCreateFunction = type->GetMethodByDecl("void OnCreate()");
        m_onDestroyFunction = type->GetMethodByDecl("void OnDestroy()");
        m_onTickFunction = type->GetMethodByDecl("void OnTick(float)");

        if (m_onCreateFunction)
        {
            asIScriptContext* createCtx = nullptr;
            AngelScriptRequestBus::BroadcastResult(createCtx, &AngelScriptRequestBus::Events::RequestContext);
            if (createCtx)
            {
                createCtx->Prepare(m_onCreateFunction);
                createCtx->SetObject(m_scriptObject);
                createCtx->Execute();
                AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, createCtx);
            }
        }

        if (m_onTickFunction)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }
```

- [ ] **Step 2: Replace `OnTick` with the pool-using implementation**

Replace the entire body of `OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)` with:

```cpp
    void AngelScriptComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        if (m_scriptObject && m_onTickFunction)
        {
            asIScriptContext* ctx = nullptr;
            AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
            if (ctx)
            {
                ctx->Prepare(m_onTickFunction);
                ctx->SetObject(m_scriptObject);
                ctx->SetArgFloat(0, deltaTime);
                ctx->Execute();
                AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
            }
        }
    }
```

Note: the parameter is now named `deltaTime` (was `/*deltaTime*/`).

- [ ] **Step 3: Replace the `OnDestroy` dispatch block in `DestroyScriptObject`**

In `DestroyScriptObject()`, replace the `if (m_onDestroyFunction) { ... }` block (the one with the commented-out `RequestContext`) with:

```cpp
            if (m_onDestroyFunction)
            {
                asIScriptContext* ctx = nullptr;
                AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
                if (ctx)
                {
                    ctx->Prepare(m_onDestroyFunction);
                    ctx->SetObject(m_scriptObject);
                    ctx->Execute();
                    AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
                }
            }
```

- [ ] **Step 4: Ensure `<angelscript.h>` is included in the component .cpp**

At the top of `AngelScriptComponent.cpp`, uncomment / add:
```cpp
#include <angelscript.h>
```
(It is currently commented at line ~11.)

- [ ] **Step 5: Build the runtime + editor modules**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Editor --config profile`
Expected: compiles and links (this also builds the runtime `AngelScript` module it depends on).

- [ ] **Step 6: Run the gem tests (no regressions)**

Run: `ctest --test-dir C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows -C profile -R AngelScript --output-on-failure`
Expected: PASS.

- [ ] **Step 7: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Source/AngelScriptComponent.cpp
git commit -m "Wire per-entity AngelScript execution via context pool + EnsureModule"
```

---

### Task 7: Builder stores source; align asset (de)serialization

**Files:**
- Modify: `AngelScript/Code/Source/Builders/AngelScriptBuilderWorker.cpp` (`ProcessJob`)
- Modify: `AngelScript/Code/Source/AngelScriptAssetHandler.cpp` (`LoadAssetData`)

**Interfaces:**
- Consumes: `AngelScriptAsset` (`m_scriptData.m_script`, `m_scriptData.m_debugName`, `m_moduleName`).
- Produces: a `.as` product asset whose stored source + module name round-trip to the runtime, so `AngelScriptComponent` (Task 6) can compile it.

- [ ] **Step 1: Store the source text in the product asset**

In `AngelScript/Code/Source/Builders/AngelScriptBuilderWorker.cpp`, in `ProcessJob`, replace the block that constructs the asset (the lines from `// 5. Create and serialize the AngelScriptAsset` through `asset.m_moduleName = moduleName;`) with:

```cpp
        // 5. Create the product asset: store the SOURCE text (compiled at runtime).
        AngelScriptAsset asset;
        asset.m_moduleName = moduleName;
        asset.m_scriptData.m_debugName = request.m_sourceFile;
        // fileBuffer was sized fileSize + 1 (trailing null); store just the source bytes.
        asset.m_scriptData.m_script.assign(fileBuffer.begin(), fileBuffer.begin() + fileSize);
```

- [ ] **Step 2: Deserialize the full asset (not just AngelScriptData) at load**

In `AngelScript/Code/Source/AngelScriptAssetHandler.cpp`, in `LoadAssetData`, replace the `AZ::Utils::LoadObjectFromStreamInPlace<AngelScriptData>(...)` call (and its `if/else`) with a load of the full asset so `m_moduleName` round-trips:

```cpp
        if (AZ::Utils::LoadObjectFromStreamInPlace<AngelScriptAsset>
            (*stream
                , *assetData
                , serializeContext
                , AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB)))
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }
        else
        {
            return AZ::Data::AssetHandler::LoadResult::Error;
        }
```

- [ ] **Step 3: Build the editor module (compiles the builder + runtime)**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Editor --config profile`
Expected: compiles and links.

- [ ] **Step 4: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Source/Builders/AngelScriptBuilderWorker.cpp AngelScript/Code/Source/AngelScriptAssetHandler.cpp
git commit -m "Builder stores .as source; align asset load to full AngelScriptAsset"
```

---

### Task 8: Enable assigning a `.as` asset in the Editor Inspector

**Files:**
- Modify: `AngelScript/Code/Source/AngelScriptComponent.cpp` (`Reflect`)

**Interfaces:**
- Consumes: `AngelScriptComponent::m_scriptAsset`.
- Produces: an Inspector field to assign a `.as` asset to the component.

- [ ] **Step 1: Uncomment the DataElement for the script asset**

In `AngelScriptComponent::Reflect`, in the `EditContext` block, replace the two commented lines:
```cpp
                    //->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_scriptAsset, "Script", "The AngelScript asset to execute.")
                    //->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree"))
```
with:
```cpp
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_scriptAsset, "Script", "The AngelScript asset to execute.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
```

- [ ] **Step 2: Build the editor module**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target AngelScript.Editor --config profile`
Expected: compiles and links.

- [ ] **Step 3: Commit**

```bash
cd C:/Users/Clayton/Source/o3degems_public
git add AngelScript/Code/Source/AngelScriptComponent.cpp
git commit -m "Expose script asset assignment in the Editor Inspector"
```

---

### Task 9: Editor demo (milestone 0b) — manual verification

**Files:**
- Create: `C:/Users/Clayton/O3DE/Projects/PlatformerTest/Assets/Scripts/HelloEntity.as` (sample script; path under the project's asset tree)

**Interfaces:**
- Consumes: the full pipeline from Tasks 1–8.
- Produces: visible confirmation that a script runs on an entity in the Editor.

- [ ] **Step 1: Create a sample script**

Create `C:/Users/Clayton/O3DE/Projects/PlatformerTest/Assets/Scripts/HelloEntity.as`:

```angelscript
class HelloEntity
{
    void OnCreate()
    {
        print("HelloEntity: OnCreate\n");
    }

    void OnTick(float dt)
    {
        // Intentionally quiet per-frame; OnCreate proves execution.
    }

    void OnDestroy()
    {
        print("HelloEntity: OnDestroy\n");
    }
}
```

Note: `print` is AngelScript's built-in only if registered. If the engine has no `print` yet, replace the bodies with a call the engine *does* expose, or temporarily register a `void print(const string &in)` in `InitializeAngelScriptEngine`. Simplest for the demo: register a native `void Log(const string &in)` routed to `AZLOG_INFO` during engine init, and call `Log("...")` from the script. (Decide during implementation; keep it a 1–2 line native registration, not new scope.)

- [ ] **Step 2: Build the Editor and Asset Processor**

Run: `cmake --build C:/Users/Clayton/O3DE/Projects/PlatformerTest/build/windows --target Editor AssetProcessor --config profile`
Expected: builds. (First full Editor build may take a while.)

- [ ] **Step 3: Process the asset**

Launch the Asset Processor; confirm `HelloEntity.as` processes to a product with no errors (check the AP log / the AngelScript builder job).

- [ ] **Step 4: Run the demo in the Editor**

1. Open the `PlatformerTest` project in the Editor.
2. Create an entity; add the **AngelScript** component.
3. In the Inspector, assign the `HelloEntity.as` asset to the component's Script field.
4. Enter game mode.
5. Confirm the console/log shows the `OnCreate` output.

Expected: the script's `OnCreate` log line appears — **milestone 0b reached.**

- [ ] **Step 5: Commit the sample script (if kept in-repo) and update memory**

If you want the sample tracked, commit it in the project repo (note: it lives in the O3DE project, not the gem repo — commit only if that project is version-controlled). Then update the project memory to record that Phase 0 (0a + 0b) is complete.

---

## Self-Review

**Spec coverage:**
- Build wiring (spec §1) → Task 1. ✅
- Builder stores source + optional syntax check (spec §2) → Task 7. *Note:* the optional builder syntax-check was dropped as YAGNI for the milestone (runtime compile is authoritative); the spec marked it optional/cuttable, so this is within scope. ✅
- Asset handler load (spec §3) → Task 7 aligns it to load the full `AngelScriptAsset` (spec said "unchanged", but the builder writes an `AngelScriptAsset` while the handler read `AngelScriptData` — a real type-ID mismatch that would fail loading `m_moduleName`; fixing it is required for the source pipeline and consistent with spec intent). ✅
- Context pool on system component + bus `RequestContext`/`ReturnContext` (spec §4) → Tasks 2, 5. ✅
- `EnsureModule` module compile (spec §4) → Tasks 3, 5. ✅
- Component per-entity execution (spec §5) → Task 6. ✅
- Editor asset assignment (spec §6) → Task 8. ✅
- Error handling (spec) → guarded returns + `AZ_Error` in Tasks 3, 5, 6. ✅
- Testing / 0a (spec) → Tasks 2, 3, 4. ✅
- 0b Editor demo (spec) → Task 9. ✅
- Out-of-scope items (spec) → none implemented. ✅

**Placeholder scan:** No "TBD"/"handle edge cases"/"similar to Task N"/uncoded steps. Task 9 Step 1 flags a real runtime decision (`print` vs a registered `Log`) with a concrete 1–2 line resolution, not a placeholder.

**Type consistency:** `ScriptContextPool::{Initialize,Shutdown,Acquire,Release}` used consistently in Tasks 2, 4, 5. `CompileModuleFromSource(asIScriptEngine*, const char*, const char*)` consistent in Tasks 3, 4, 5. Bus methods `RequestContext()` / `ReturnContext(asIScriptContext*)` / `EnsureModule(const AZStd::string&, const AZStd::string&)` consistent in Tasks 5, 6. Asset fields `m_scriptData.m_script`, `m_scriptData.m_debugName`, `m_moduleName` consistent in Tasks 6, 7.

## Notes / risks

- **Installed-SDK test support:** Task 2 Step 1 is a preflight — if the installed O3DE SDK can't build/run gem gtests, switch 0a to a standalone console harness (same code as Task 4) before proceeding.
- **Bytecode config sensitivity:** avoided by compiling from source (per design). Do not reintroduce a bytecode path until Phase 1's binding surface stabilizes.
- **`AZLOG`/`print` in the demo:** the runtime engine currently registers no script-callable functions; Task 9 registers a one-line native log so the script has something observable to call.
