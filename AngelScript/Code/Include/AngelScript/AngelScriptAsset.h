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
        AZ_RTTI(AngelScriptAsset, "{BD76A288-FAF4-44C1-A572-31877BCC504B}", AZ::Data::AssetData);
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
    };

} // namespace AngelScript
