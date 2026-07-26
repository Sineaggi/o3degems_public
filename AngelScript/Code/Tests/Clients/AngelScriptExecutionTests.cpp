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
