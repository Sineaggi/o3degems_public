
#pragma once

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AngelScript/AngelScriptBus.h>

#include <AngelScriptAssetHandler.h>

#include <Builders/AngelScriptBuilderWorker.h>

#include <angelscript.h>

namespace AngelScript
{
    class AngelScriptSystemComponent
        : public AZ::Component
        , protected AngelScriptRequestBus::Handler
        , public AZ::TickBus::Handler
        , public AZ::AssetTypeInfoBus::Handler

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

        ////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler overrides
         //! this is the same type Id (uuid) as your AssetData-derived class's RTTI type.
        AZ::Data::AssetType GetAssetType() const override;

        //! Retrieve the friendly name for the asset type.
        const char* GetAssetTypeDisplayName() const override;

        //! This is the group or category that this kind of asset appears under for filtering and displaying in the browser.
        const char* GetGroup() const override;

        //!  You can implement this to apply a specific icon to all assets of your type instead of using built in heuristics
        const char* GetBrowserIcon() const override;

        //!  you can return the kind of component best suited to spawn on an entity if this kind of asset is dragged
        //!  to the viewport or to the component entity area.
        AZ::Uuid GetComponentTypeId() const override;

        //! Retrieve file extensions for the asset type.
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        //! Determines if a component can be created from the asset type
        //! This will be called before attempting to create a component from an asset (drag&drop, etc)
        //! You can use this to filter by subIds or do your own validation here if needed
        bool CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const override;
        //////////////////////////////////////////////////////////////////////////

    private:
        /// @brief Creates and configures the AngelScript engine instance.
        void InitializeAngelScriptEngine();

        /// @brief Shuts down and cleans up the AngelScript engine.
        void ShutdownAngelScriptEngine();

        /// @brief Creates the workspace directory for AngelScript files.
        void CreateScriptWorkspace();

        // The path to the AngelScript workspace directory.
        AZStd::string m_scriptWorkspacePath;

        AZStd::unique_ptr<AngelScriptAssetHandler> m_angelSriptAssetHandler;

        AngelScriptBuilderWorker m_angelScriptBuilderWorker;

        void RegisterBuilder();

    };

} // namespace AngelScript
