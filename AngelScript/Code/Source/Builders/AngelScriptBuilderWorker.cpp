#include "AngelScriptBuilderWorker.h"
#include "AngelScriptBuilderWorker.h"
#include "AngelScript/AngelScriptAsset.h"

// AngelScript Headers
#include <angelscript.h>

#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformDef.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Console/ILogger.h>

#pragma optimize("", off)

namespace AngelScript
{
    namespace
    {
        // A minimal message callback for the builder to report compilation errors.
        void BuilderMessageCallback(const asSMessageInfo* msg, void* /*param*/)
        {
            //auto& response = *static_cast<AssetBuilderSDK::ProcessJobResponse*>(param);
            const char* typeStr = "ERROR";
            bool isError = true;
            if (msg->type == asMSGTYPE_WARNING)
            {
                typeStr = "WARN";
                isError = false;
            }
            else if (msg->type == asMSGTYPE_INFORMATION)
            {
                typeStr = "INFO";
                isError = false;
            }

            AZStd::string logMessage = AZStd::string::format("[%s] (%s:%d,%d) %s", typeStr, msg->section, msg->row, msg->col, msg->message);

            if (isError)
            {
                AZLOG_ERROR("AngelScriptBuilder", false, logMessage.c_str());
                ///response.m_issues.emplace_back(AZ::Uuid::Create(), logMessage.c_str(), AZ::Data::AssetIssue::Severity::Error);
            }
            else
            {
                AZLOG_ERROR("AngelScriptBuilder", false, logMessage.c_str());
                //response.m_issues.emplace_back(AZ::Uuid::Create(), logMessage.c_str(), AZ::Data::AssetIssue::Severity::Warning);
            }
        }
    }

    
    void AngelScriptBuilderWorker::Deactivate()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect();
    }

    void AngelScriptBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {

            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "AngelScript Compile";
            descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            //descriptor.SetAZBusId(GetBusId());
            descriptor.m_critical = true; // Scripts are often critical dependencies.
            response.m_createJobOutputs.push_back(descriptor);
        }
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void AngelScriptBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AZ_TracePrintf("AngelScriptBuilder", "Processing job for source file %s", request.m_sourceFile.c_str());

        // 1. Read source file content
        AZStd::vector<char> fileBuffer;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("AngelScriptBuilder", false, "FileIO instance is null.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ::IO::HandleType fileHandle;
        if (fileIO->Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead, fileHandle) != AZ::IO::ResultCode::Success)
        {
            AZ_Error("AngelScriptBuilder", false, "Failed to open source file: %s", request.m_sourceFile.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ::u64 fileSize = 0;
        fileIO->Size(fileHandle, fileSize);
        fileBuffer.resize(fileSize + 1, 0); // +1 for null terminator
        fileIO->Read(fileHandle, fileBuffer.data(), fileSize);
        fileIO->Close(fileHandle);

        // 2. Initialize a temporary AngelScript engine for compilation
        asIScriptEngine* engine = asCreateScriptEngine();
        engine->SetMessageCallback(asFUNCTION(BuilderMessageCallback), &response, asCALL_CDECL);

        // 3. Compile the script using CScriptBuilder
        //CScriptBuilder builder;
        AZStd::string moduleName = request.m_sourceFile;
        AzFramework::StringFunc::Path::GetFileName(moduleName.c_str(), moduleName);

        ///builder.StartNewModule(engine, moduleName.c_str());
        //builder.AddSectionFromMemory(request.m_sourceFile.c_str(), fileBuffer.data());
        ////int r = builder.BuildModule();

        //if (r < 0)
        ////{
        //    AZ_Error("AngelScriptBuilder", false, "AngelScript compilation failed for %s.", request.m_sourceFile.c_str());
        //   engine->Release();
        //    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        //   return;
        //}

        // 4. Save the bytecode from the compiled module
        //asIScriptModule* module = builder.GetModule();
        AZ::IO::MemoryStream byteCodeStream(nullptr, 0);
        //module->SaveByteCode(&byteCodeStream);

        engine->Release();
        engine = nullptr;

        // 5. Create and serialize the AngelScriptAsset
        AngelScriptAsset asset;
        asset.m_moduleName = moduleName;
        //asset.m_byteCode.assign(reinterpret_cast<const char*>(byteCodeStream.GetData()), byteCodeStream.GetLength());

        AZStd::string destPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), "script", "asasset", destPath);

        if (!AZ::Utils::SaveObjectToFile(destPath, AZ::DataStream::ST_JSON, &asset))
        {
            AZ_Error("AngelScriptBuilder", false, "Failed to save asset product to file: %s", destPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // 6. Report the product as output
        AssetBuilderSDK::JobProduct jobProduct(destPath);
        
        ///jobProduct.m_assetId.m_guid = AZ::AzTypeInfo<AngelScriptAsset>::Uuid();
        //jobProduct.m_assetId.m_subId = 0; // Or generate a sub-ID if needed
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }



} // namespace AngelScript


#pragma optimize("", on)
