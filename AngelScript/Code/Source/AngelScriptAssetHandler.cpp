
#include "AngelScriptAssetHandler.h"
#include <AngelScript/AngelScriptBus.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

// AngelScript Headers
#include <angelscript.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/Utils.h>

#pragma optimize("", off)

namespace AngelScript
{

    AngelScriptAssetHandler::AngelScriptAssetHandler()
    {
        // The handler is typically registered by a system component (e.g., AngelScriptSystemComponent)
        // to ensure correct initialization order.
    }

    AngelScriptAssetHandler::~AngelScriptAssetHandler()
    {
        // The system component that registered this handler should also be responsible for unregistering it.
    }

    bool AngelScriptAssetHandler::CanCreateComponent(const AZ::Data::AssetId& /*assetId*/) const
    {
        // TODO-LS: Change to True when I make the ASComponent, and have this handler create that type
        return false;
    }

    void AngelScriptAssetHandler::Register()
    {
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager is not ready!");
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<AngelScriptAsset>::Uuid());
        }
        AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<AngelScriptAsset>::Uuid());
    }

    void AngelScriptAssetHandler::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect();
        if (AZ::Data::AssetManager::IsReady())
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);
        }
    }

    void AngelScriptAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(azrtti_typeid<AngelScriptAsset>());
    }

    AZ::Data::AssetPtr AngelScriptAssetHandler::CreateAsset(const AZ::Data::AssetId& /*id*/, const AZ::Data::AssetType& /*type*/)
    {
        return aznew AngelScriptAsset();
    }

    AZ::Data::AssetHandler::LoadResult AngelScriptAssetHandler::LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        //AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Asset);

        AZ::Data::AssetHandler::LoadResult Result = AZ::Data::AssetHandler::LoadResult::Error;

        AngelScriptAsset* scriptAsset = asset.GetAs<AngelScriptAsset>();
        if (!scriptAsset)
        {
            AZLOG_ERROR("AngelScript", "Failed to cast asset to AngelScriptAsset.");
            return Result;
        }

        // Deserialize the entire AngelScriptAsset object from the product file stream.
        AZ::TypeId angelScriptTypeId = AZ::AzTypeInfo<AngelScriptAsset>::Uuid();
        AZ::ObjectStream::FilterDescriptor filter(assetLoadFilterCB);
        if (!AZ::Utils::LoadObjectFromStream(*stream, nullptr, &angelScriptTypeId, filter))
        {
            AZLOG_ERROR("AngelScript", "Failed to load/deserialize AngelScriptAsset from stream for asset %s", asset.GetId().ToString<AZStd::string>().c_str());
            return Result;
        }

        // Now that the asset data is loaded into memory, load the bytecode into the AngelScript engine.
        asIScriptEngine* engine = nullptr;
        AngelScriptRequestBus::BroadcastResult(engine, &AngelScriptRequestBus::Events::GetScriptEngine);

        if (!engine)
        {
            AZLOG_ERROR("AngelScript", "Cannot load script asset %s, AngelScript engine is not available.", asset.GetId().ToString<AZStd::string>().c_str());
            // We return true because the asset data itself loaded correctly from disk. The engine might initialize later.
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        asIScriptModule* module = engine->GetModule(scriptAsset->m_moduleName.c_str(), asGM_ALWAYS_CREATE);
        if (!module)
        {
            AZLOG_ERROR("AngelScript", "Failed to create or get module '%s' for asset %s", scriptAsset->m_moduleName.c_str(), asset.GetId().ToString<AZStd::string>().c_str());
            return Result;
        }

        // Use a memory stream to load the bytecode buffer into the module.
        AZ::IO::MemoryStream byteCodeStream(scriptAsset->m_byteCode.data(), scriptAsset->m_byteCode.size());
        int r = module->LoadByteCode(reinterpret_cast<asIBinaryStream*>(&byteCodeStream));
        if (r < 0)
        {
            AZLOG_ERROR("AngelScript", "Failed to load bytecode into module '%s'. Error code: %d", scriptAsset->m_moduleName.c_str(), r);
            engine->DiscardModule(scriptAsset->m_moduleName.c_str());
            return Result;
        }

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());
        AZLOG_INFO("AngelScript", "Successfully loaded script asset '%s' into module '%s'.", assetInfo.m_relativePath.c_str(), scriptAsset->m_moduleName.c_str());

        return AZ::Data::AssetHandler::LoadResult::LoadComplete;
    }

    void AngelScriptAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        // When an asset is destroyed (e.g., unloaded), we should also remove its corresponding module from the engine.
        AngelScriptAsset* scriptAsset = static_cast<AngelScriptAsset*>(ptr);
        if (scriptAsset)
        {
            asIScriptEngine* engine = nullptr;
            AngelScriptRequestBus::BroadcastResult(engine, &AngelScriptRequestBus::Events::GetScriptEngine);
            if (engine)
            {
                engine->DiscardModule(scriptAsset->m_moduleName.c_str());
                AZLOG_INFO("AngelScript", "Discarded module '%s' on asset destruction.", scriptAsset->m_moduleName.c_str());
            }
        }
        delete ptr;
    }

    // --- AssetTypeInfoBus::Handler Implementation ---

    AZ::Data::AssetType AngelScriptAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<AngelScriptAsset>::Uuid();
    }

    const char* AngelScriptAssetHandler::GetAssetTypeDisplayName() const
    {
        return "AngelScript File";
    }

    const char* AngelScriptAssetHandler::GetGroup() const
    {
        return "Scripts";
    }

    const char* AngelScriptAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/AssetBrowser/Script_16.png";
    }

    AZ::Uuid AngelScriptAssetHandler::GetComponentTypeId() const
    {
        // Return the Uuid of the AngelScriptComponent that will use this asset.
        // This will be defined later. For now, a null Uuid is acceptable.
        return AZ::Uuid::CreateNull();
    }

    void AngelScriptAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) 
    {
        extensions.push_back("as");
    }

} // namespace AngelScript


#pragma optimize("", on)
