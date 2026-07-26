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
