#include <ScriptModuleCompiler.h>

#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>

namespace AngelScript
{
    asIScriptModule* CompileModuleFromSource(
        asIScriptEngine* engine, const char* moduleName, const char* source, const char* sectionName)
    {
        if (!engine || !moduleName || !source)
        {
            return nullptr;
        }

        // Section name appears in compiler diagnostics; default to the module name when not given.
        const char* section = sectionName ? sectionName : moduleName;

        CScriptBuilder builder;
        // asGM_ALWAYS_CREATE inside StartNewModule discards any existing module of this name.
        if (builder.StartNewModule(engine, moduleName) < 0)
        {
            return nullptr;
        }
        if (builder.AddSectionFromMemory(section, source) < 0)
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

    asIScriptModule* EnsureModule(
        asIScriptEngine* engine, const char* moduleName, const char* source,
        const char* sectionName, bool forceRecompile)
    {
        if (!engine || !moduleName)
        {
            return nullptr;
        }

        if (!forceRecompile)
        {
            if (asIScriptModule* existing = engine->GetModule(moduleName, asGM_ONLY_IF_EXISTS))
            {
                return existing;
            }
        }
        return CompileModuleFromSource(engine, moduleName, source, sectionName);
    }
} // namespace AngelScript
