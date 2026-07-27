#pragma once

class asIScriptEngine;
class asIScriptModule;

namespace AngelScript
{
    //! Compile AngelScript `source` into a module named `moduleName` on `engine`,
    //! replacing any existing module of that name. `sectionName` is the name used for the
    //! script section (it appears in compiler diagnostics) -- pass a human-readable path so
    //! errors stay legible even when `moduleName` is an opaque id; if null, `moduleName` is used.
    //! Returns the compiled module, or nullptr on failure (diagnostics go to the engine message callback).
    asIScriptModule* CompileModuleFromSource(
        asIScriptEngine* engine, const char* moduleName, const char* source, const char* sectionName = nullptr);

    //! Get the module named `moduleName`, compiling it from `source` if absent. When
    //! `forceRecompile` is true the module is always rebuilt from `source` (used for hot reload,
    //! since a module's key is stable across reloads and would otherwise return the stale module).
    //! `sectionName` is forwarded to CompileModuleFromSource for readable diagnostics.
    //! Returns the module, or nullptr on compile failure / null engine.
    asIScriptModule* EnsureModule(
        asIScriptEngine* engine, const char* moduleName, const char* source,
        const char* sectionName, bool forceRecompile);
} // namespace AngelScript
