
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AngelScript/AngelScriptBus.h>

#include <angelscript.h>

namespace AngelScript
{
    class AngelScriptSystemComponent
        : public AZ::Component
        , protected AngelScriptRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(AngelScriptSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AngelScriptSystemComponent();
        ~AngelScriptSystemComponent();

    private:

        asIScriptEngine* m_scriptEngine = nullptr;
        asIScriptContext* m_scriptContext = nullptr;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AngelScriptRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AngelScriptRequestBus::Handler interface implementation
        asIScriptEngine* GetScriptEngine() override;
        asIScriptContext* CreateContext() override;
        asIScriptModule* GetModule(const AZStd::string& moduleName) override;
        bool ExecuteString(const AZStd::string& scriptCode, const AZStd::string& moduleName) override;
        bool RegisterGlobalFunction(const char* declaration, const void* funcPointer) override;
        bool RegisterGlobalProperty(const char* declaration, void* propertyPtr) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        /// @brief Creates and configures the AngelScript engine instance.
        void InitializeAngelScriptEngine();

        /// @brief Shuts down and cleans up the AngelScript engine.
        void ShutdownAngelScriptEngine();

        /// @brief Creates the workspace directory for AngelScript files.
        void CreateScriptWorkspace();

        // The path to the AngelScript workspace directory.
        AZStd::string m_scriptWorkspacePath;
    };

} // namespace AngelScript
