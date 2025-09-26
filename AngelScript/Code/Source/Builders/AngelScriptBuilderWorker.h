#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AngelScript/AngelScriptAsset.h>
#include <Preprocessor/AngelScriptPreprocessor.h>

namespace AngelScript
{
    /// @class AngelScriptBuilderWorker
    /// @brief This class is the Asset Processor's worker for compiling .as source files.
    /// It listens for asset build requests, compiles the source script into bytecode,
    /// and emits a runtime AngelScriptAsset as the product.
    class AngelScriptBuilderWorker
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(AngelScriptBuilderWorker, "{01234567-89AB-CDEF-0123-456789ABCDEF}");

        AngelScriptBuilderWorker() = default;
        ~AngelScriptBuilderWorker() override = default;

        /// @brief Connects to the AssetBuilderCommandBus and registers the builder descriptor.
        void Activate()
        {
            AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(GetUUID());
        }


        /// @brief Disconnects from the bus.
        void Deactivate();

        //////////////////////////////////////////////////////////////////////////
        // AssetBuilderSDK::AssetBuilderCommandBus::Handler overrides
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;
        //////////////////////////////////////////////////////////////////////////

        // AssetBuilderSDK::AssetBuilderCommandBus::Handler overrides for job cancellation (optional)
        // void Cancel() override { m_isShuttingDown = true; } 
        // AZStd::atomic_bool m_isShuttingDown{ false };

        int GetVersionNumber() const { return 1; }

        static AZ::Uuid GetUUID()
        {
            return AZ::Uuid::CreateString("{9D5F3A39-0214-4D67-9307-92DFA90F830B}");
        }

        const char* GetFingerprintString() const
        {
            if (m_fingerprintString.empty())
            {
                // compute it the first time
                const AZStd::string runtimeAssetTypeId = azrtti_typeid<AngelScriptAsset>().ToString<AZStd::string>();
                m_fingerprintString = AZStd::string::format("%i%s", GetVersionNumber(), runtimeAssetTypeId.c_str());
            }
            return m_fingerprintString.c_str();
        }

        mutable AngelScriptPreprocessor m_preprocessor;

        mutable AZStd::string m_fingerprintString;

        void ShutDown() override {}
    };
} // namespace AngelScript
