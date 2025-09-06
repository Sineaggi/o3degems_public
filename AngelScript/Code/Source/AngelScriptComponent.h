#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include "AngelScript/AngelScriptAsset.h"

// Forward declare AngelScript types to avoid including the header here
class asIScriptObject;
class asIScriptFunction;

namespace AngelScript
{
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
        AZ_COMPONENT(AngelScriptComponent, "{C5971795-0B68-4580-BC56-D43643618055}");

        AngelScriptComponent() = default;
        ~AngelScriptComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

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

} // namespace AngelScript
