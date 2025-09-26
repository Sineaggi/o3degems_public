#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>

#include "AngelScript/AngelScriptAsset.h"


// Forward declare AngelScript types to avoid including the header here
class asIScriptObject;
class asIScriptFunction;

namespace AngelScript
{
    class AngelScriptASComponentDescriptor;

    /// @class AngelScriptComponent
    /// @brief The component that attaches to an AZ::Entity to execute AngelScript logic.
    /// This component holds an asset reference to a compiled AngelScriptAsset.
    /// When activated, it instantiates a script class and calls lifecycle functions on it.
    class AngelScriptComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AZ::Data::AssetBus::Handler
    {
    public:
        AZ_RTTI(AngelScriptComponent, "{C5971795-0B68-4580-BC56-D43643618055}", AZ::Component);

        AngelScriptComponent() = default;
        ~AngelScriptComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static AZ::ComponentDescriptor* CreateDescriptor();


    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler implementation
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        /// @brief Creates the script object instance from the loaded asset.
        void CreateScriptObject();

        /// @brief Releases the script object and associated resources.
        void DestroyScriptObject();

        /// @brief The compiled AngelScript asset that contains the logic for this component.
        AZ::Data::Asset<AngelScriptAsset> m_scriptAsset;

        // --- Script object members ---
        asIScriptObject* m_scriptObject = nullptr;

        // --- Cached script functions ---
        asIScriptFunction* m_onCreateFunction = nullptr;
        asIScriptFunction* m_onDestroyFunction = nullptr;
        asIScriptFunction* m_onTickFunction = nullptr;
    };



    class AngelScriptASComponentDescriptor :
        public AZ::ComponentDescriptorHelper<AngelScriptComponent>
    {
    public:
        AZ_CLASS_ALLOCATOR(AngelScriptASComponentDescriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(AngelScriptASComponentDescriptor, "{5977E961-D107-4533-BC20-116DC07C81F4}");

        AngelScriptASComponentDescriptor() = default;

        void Reflect(AZ::ReflectContext* reflection) const override;

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override;
        void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override;
        void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override;
        ///void GetWarnings(AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const override;


        AZStd::string m_name;
        AZ::Uuid m_uuid;


    };
    void AngelScriptASComponentDescriptor::Reflect(AZ::ReflectContext* reflection) const
    {
        AngelScriptComponent::Reflect(reflection);
    }

    void AngelScriptASComponentDescriptor::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetProvidedServices(provided);
    }

    void AngelScriptASComponentDescriptor::GetDependentServices(
        [[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetDependentServices(dependent);
    }

    void AngelScriptASComponentDescriptor::GetRequiredServices(
        AZ::ComponentDescriptor::DependencyArrayType& required, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetRequiredServices(required);
    }

    //void AngelScriptASComponentDescriptor::GetWarnings(
    //    AZ::ComponentDescriptor::StringWarningArray& warnings, const AZ::Component* instance) const
    //{
    //}



} // namespace AngelScript
