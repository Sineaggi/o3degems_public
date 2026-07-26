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
