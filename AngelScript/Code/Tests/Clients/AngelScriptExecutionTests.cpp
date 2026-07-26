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
}
