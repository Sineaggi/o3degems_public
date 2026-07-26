
#pragma once

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

// Forward declarations for AngelScript types to avoid including the AngelScript headers in this public interface.
class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class asIScriptFunction;

namespace AngelScript
{
    class AngelScriptRequests
    {
    public:
        AZ_RTTI(AngelScriptRequests, AngelScriptRequestsTypeId);
        virtual ~AngelScriptRequests() = default;


        /// @brief Retrieves the main AngelScript engine instance.
        /// @return A pointer to the asIScriptEngine instance.
        virtual asIScriptEngine* GetScriptEngine() = 0;

        /// @brief Creates a new script context for executing script functions.
        /// @return A pointer to the newly created asIScriptContext. The caller is responsible for releasing it.
        virtual asIScriptContext* CreateContext() = 0;

        /// @brief Returns a script module by its name.
        /// @param moduleName The name of the module to retrieve.
        /// @return A pointer to the asIScriptModule, or nullptr if not found.
        virtual asIScriptModule* GetModule(const AZStd::string& moduleName) = 0;

        /// @brief Borrow an execution context from the pool (return it with ReturnContext).
        virtual asIScriptContext* RequestContext() = 0;

        /// @brief Return a context previously obtained from RequestContext.
        virtual void ReturnContext(asIScriptContext* context) = 0;

        /// @brief Get the module named moduleName, compiling it from source if absent.
        /// @return The module, or nullptr on compile failure.
        virtual asIScriptModule* EnsureModule(const AZStd::string& moduleName,
                                              const AZStd::string& source) = 0;

        /// @brief Executes a string of AngelScript code.
        /// @param scriptCode The code to execute.
        /// @param moduleName The name of the module to execute the code in. If empty, a temporary module is used.
        /// @return True if execution was successful, false otherwise.
        virtual bool ExecuteString(const AZStd::string& scriptCode, const AZStd::string& moduleName = "") = 0;

        /// @brief Registers a C++ function with the script engine.
        /// @param declaration The function declaration string (e.g., "void MyFunc(int)").
        /// @param funcPointer The C++ function pointer.
        /// @return True if registration was successful, false otherwise.
        virtual bool RegisterGlobalFunction(const char* declaration, const void* funcPointer) = 0;

        /// @brief Registers a global property with the script engine.
        /// @param declaration The property declaration string (e.g., "int myGlobalVar").
        /// @param propertyPtr A pointer to the C++ variable.
        /// @return True if registration was successful, false otherwise.
        virtual bool RegisterGlobalProperty(const char* declaration, void* propertyPtr) = 0;
    };

    class AngelScriptBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

       
    };

    using AngelScriptRequestBus = AZ::EBus<AngelScriptRequests, AngelScriptBusTraits>;
    using AngelScriptInterface = AZ::Interface<AngelScriptRequests>;

} // namespace AngelScript
