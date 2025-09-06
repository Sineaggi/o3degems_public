#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include "AngelScript/AngelScriptAsset.h"

namespace AngelScript
{
    /// @class AngelScriptAssetHandler
    /// @brief Manages the loading and lifecycle of AngelScriptAsset instances.
    /// This handler is responsible for taking the raw asset data from disk (the compiled bytecode)
    /// and creating a usable AngelScriptAsset object from it. It also handles hot-reloading.
    class AngelScriptAssetHandler
        : public AZ::Data::AssetHandler
        , public AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AngelScriptAssetHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(AngelScriptAssetHandler, "{EC94F51A-F2E2-4AB3-B1C4-7AFEC324A131}", AZ::Data::AssetHandler);

        AngelScriptAssetHandler();
        ~AngelScriptAssetHandler() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetHandler overrides
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        LoadResult LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
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

        /// @brief Registers this asset handler with the asset manager.
        void Register();

        /// @brief Unregisters this asset handler.
        void Unregister();

        // Inherited via AssetHandler
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;
    };

} // namespace AngelScript
