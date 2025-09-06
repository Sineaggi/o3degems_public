#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace AngelScript
{
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

        /// @brief The compiled bytecode of the AngelScript file.
        AZStd::vector<char> m_byteCode;

        /// @brief The name of the module this script should be loaded into.
        /// This is typically derived from the asset's file path.
        AZStd::string m_moduleName;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AngelScriptAsset, AZ::Data::AssetData>()
                    ->Version(1)
                    ->Field("byteCode", &AngelScriptAsset::m_byteCode)
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
