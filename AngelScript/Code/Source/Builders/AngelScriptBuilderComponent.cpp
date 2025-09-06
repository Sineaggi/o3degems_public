/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builders/AngelScriptBuilderComponent.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <AzToolsFramework/Fingerprinting/TypeFingerprinter.h>

namespace AngelScript
{
    void AngelScriptBuilderComponent::Activate()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext not found");

        AzToolsFramework::Fingerprinting::TypeFingerprinter fingerprinter(*serializeContext);
        AzToolsFramework::Fingerprinting::TypeCollection types = fingerprinter.GatherAllTypesForComponents();
        AzToolsFramework::Fingerprinting::TypeFingerprint allComponents = fingerprinter.GenerateFingerprintForAllTypes(types);
        AZStd::string builderAnalysisFingerprint = AZStd::string::format("%zu", allComponents);

        // Register UI Canvas Builder
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "AngelScript Builder";
        builderDescriptor.m_version = 1;
        builderDescriptor.m_analysisFingerprint = builderAnalysisFingerprint;
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.as", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = AngelScriptBuilderWorker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&AngelScriptBuilderWorker::CreateJobs, &m_angelScriptBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&AngelScriptBuilderWorker::ProcessJob, &m_angelScriptBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        m_angelScriptBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);

        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, azrtti_typeid<AngelScriptAsset>(), AngelScriptAsset::GetFileFilter());
    }

    void AngelScriptBuilderComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();

        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<AngelScriptAsset>());

        m_angelScriptBuilder.BusDisconnect();
    }

    void AngelScriptBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Need to add the AssetBuilder tag because this builder is in a Gem
            // If the tag is not set, the Asset Processor will never use this builder
            serializeContext->Class<AngelScriptBuilderComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

}
