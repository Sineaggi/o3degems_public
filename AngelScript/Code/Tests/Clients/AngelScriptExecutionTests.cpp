#include <AzTest/AzTest.h>
#include <angelscript.h>
#include <ScriptContextPool.h>
#include <ScriptModuleCompiler.h>

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

    TEST_F(AngelScriptExecutionFixture, ContextPool_ReleaseAfterShutdownDoesNotStrand)
    {
        AngelScript::ScriptContextPool pool;
        pool.Initialize(m_engine);

        asIScriptContext* ctx = pool.Acquire();
        ASSERT_NE(ctx, nullptr);

        // Simulates Release() racing behind a concurrent Shutdown(): the context is
        // still in flight when the pool is torn down. Release() must free it here
        // rather than push it into the (already-cleared) free-list.
        pool.Shutdown();
        pool.Release(ctx);

        // Pool remains shut down afterward; no stray context resurrected it.
        EXPECT_EQ(pool.Acquire(), nullptr);
    }

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

    // --- Issue 1: module identity (unique key, decoupled class name, reload) ---

    // Two scripts that share a class name (e.g. two "Hello" classes from different files)
    // must live in separate engine modules keyed independently, and discarding one must not
    // affect the other. Under the old filename-stem keying these collided into one module.
    TEST_F(AngelScriptExecutionFixture, Compile_DistinctModuleKeysSameClassCoexist)
    {
        const char* src = "class Hello { }";
        asIScriptModule* a = AngelScript::CompileModuleFromSource(m_engine, "keyA", src, "a.as");
        asIScriptModule* b = AngelScript::CompileModuleFromSource(m_engine, "keyB", src, "b.as");
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);
        EXPECT_NE(a, b);
        EXPECT_NE(a->GetTypeInfoByDecl("Hello"), nullptr);
        EXPECT_NE(b->GetTypeInfoByDecl("Hello"), nullptr);

        // Discarding one module leaves the sibling intact.
        m_engine->DiscardModule("keyA");
        EXPECT_EQ(m_engine->GetModule("keyA", asGM_ONLY_IF_EXISTS), nullptr);
        EXPECT_NE(m_engine->GetModule("keyB", asGM_ONLY_IF_EXISTS), nullptr);
    }

    // Without forceRecompile, EnsureModule returns the already-compiled module untouched
    // (the second source is ignored) -- this is the fast path for an already-loaded script.
    TEST_F(AngelScriptExecutionFixture, EnsureModule_IdempotentWithoutForce)
    {
        asIScriptModule* first =
            AngelScript::EnsureModule(m_engine, "key", "class Foo { }", "foo.as", false);
        ASSERT_NE(first, nullptr);

        asIScriptModule* second =
            AngelScript::EnsureModule(m_engine, "key", "class Bar { }", "bar.as", false);
        EXPECT_EQ(first, second);                                 // same module handed back
        EXPECT_NE(second->GetTypeInfoByDecl("Foo"), nullptr);     // original content kept
        EXPECT_EQ(second->GetTypeInfoByDecl("Bar"), nullptr);     // second source NOT compiled
    }

    // With forceRecompile (the reload path), EnsureModule rebuilds the module from the new
    // source, replacing the old definition.
    TEST_F(AngelScriptExecutionFixture, EnsureModule_ForceRecompilesReplacesContent)
    {
        asIScriptModule* first =
            AngelScript::EnsureModule(m_engine, "key", "class Foo { }", "foo.as", false);
        ASSERT_NE(first, nullptr);

        asIScriptModule* rebuilt =
            AngelScript::EnsureModule(m_engine, "key", "class Bar { }", "bar.as", true);
        ASSERT_NE(rebuilt, nullptr);
        EXPECT_NE(rebuilt->GetTypeInfoByDecl("Bar"), nullptr);    // new source compiled
        EXPECT_EQ(rebuilt->GetTypeInfoByDecl("Foo"), nullptr);    // old definition gone
    }
}
