#pragma once

#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AngelScript
{
    class AngelScriptAsset;

    class AngelScriptData
    {
    public:
        AZ_CLASS_ALLOCATOR(AngelScriptData, AZ::SystemAllocator);
        AZ_TYPE_INFO(AngelScriptData, "{44058DC7-ABDE-4562-95B8-5DACB4CB9699}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_debugName;
        AZStd::vector<AZ::Data::Asset<AngelScriptAsset>> m_dependencies;
        AZStd::vector<char> m_script;

        AZ::IO::MemoryStream CreateScriptReadStream();
        AZ::IO::ByteContainerStream<AZStd::vector<char>> CreateScriptWriteStream();
        const char* GetDebugName();
        const AZStd::vector<char>& GetScriptBuffer();
    };


    /// @class AngelScriptAsset
    /// @brief This asset contains the compiled bytecode for an AngelScript file.
    /// The asset is the product of the AngelScriptBuilderWorker and is loaded at runtime by the AngelScriptAssetHandler.
    class AngelScriptAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(AngelScriptAsset, "{D22E5C25-0F7F-4922-BE53-ABFF264B9FF5}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(AngelScriptAsset, AZ::SystemAllocator, 0);

        AngelScriptAsset() = default;
        ~AngelScriptAsset() override = default;

        static constexpr AZ::u32 AssetSubId = 1;

        AngelScriptData m_scriptData;
        /// @brief The compiled bytecode of the AngelScript file.
        //AZStd::vector<char> m_byteCode;

        /// @brief The name of the module this script should be loaded into.
        /// This is typically derived from the asset's file path.
        AZStd::string m_moduleName;

        static void Reflect(AZ::ReflectContext* context)
        {
            AngelScriptData::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AngelScriptAsset, AZ::Data::AssetData>()
                    ->Version(1)
                    ->Field("m_scriptData", &AngelScriptAsset::m_scriptData)
                    ->Field("moduleName", &AngelScriptAsset::m_moduleName);
            }
        }

        static const char* GetFileFilter()
        {
            return "*.as";
        }
    };

} // namespace AngelScript

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<AZ::Data::Asset<AngelScript::AngelScriptAsset>>
    {
        using argument_type = AZ::Uuid;
        using result_type = size_t;
        size_t operator()(const AZ::Data::Asset<AngelScript::AngelScriptAsset>& asset) const
        {
            return asset.GetId().m_guid.GetHash();
        }
    };
}

