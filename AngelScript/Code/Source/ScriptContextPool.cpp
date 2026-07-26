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
        if (!m_engine)
        {
            // Pool was shut down while this context was in flight (e.g. Shutdown()
            // ran concurrently between Acquire() and Release()). m_free has already
            // been cleared and won't be drained again, so pushing here would strand
            // the context forever. Free it immediately instead.
            context->Release();
            return;
        }
        m_free.push_back(context);
    }
} // namespace AngelScript
