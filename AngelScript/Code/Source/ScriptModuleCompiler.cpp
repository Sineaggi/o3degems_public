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
