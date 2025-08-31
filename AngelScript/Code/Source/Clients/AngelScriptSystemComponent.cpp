
#include "AngelScriptSystemComponent.h"

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/IO/FileIO.h>

// AngelScript Headers
#include <angelscript.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Console/ILogger.h>

namespace AngelScript
{
    AZ_COMPONENT_IMPL(AngelScriptSystemComponent, "AngelScriptSystemComponent",
        AngelScriptSystemComponentTypeId);

    void AngelScriptSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AngelScriptSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AngelScriptSystemComponent>("AngelScript System", "Manages the AngelScript virtual machine and script execution environment.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AngelScriptSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AngelScriptSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    AngelScriptSystemComponent::AngelScriptSystemComponent()
    {
        if (AngelScriptInterface::Get() == nullptr)
        {
            AngelScriptInterface::Register(this);
        }
    }

    AngelScriptSystemComponent::~AngelScriptSystemComponent()
    {
        m_scriptEngine->ShutDownAndRelease();

        if (AngelScriptInterface::Get() == this)
        {
            AngelScriptInterface::Unregister(this);
        }
    }

    void AngelScriptSystemComponent::Init()
    {
        m_scriptEngine = asCreateScriptEngine();
        m_scriptContext = m_scriptEngine->CreateContext();
    }

    void AngelScriptSystemComponent::Activate()
    {
        AngelScriptRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        CreateScriptWorkspace();
        InitializeAngelScriptEngine();
    }

    void AngelScriptSystemComponent::Deactivate()
    {
        ShutdownAngelScriptEngine();

        AZ::TickBus::Handler::BusDisconnect();
        AngelScriptRequestBus::Handler::BusDisconnect();
    }

    void AngelScriptSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

    // A simple message callback function for AngelScript to route messages to the O3DE logger.
    void MessageCallback(const asSMessageInfo* msg, void* param)
    {
        (void)param;

        const char* typeStr = "ERROR";
        AZ::Debug::LogLevel level = AZ::Debug::LogLevel::Errors;
        if (msg->type == asMSGTYPE_WARNING)
        {
            typeStr = "WARN";
            level = AZ::Debug::LogLevel::Warnings;
        }
        else if (msg->type == asMSGTYPE_INFORMATION)
        {
            typeStr = "INFO";
            level = AZ::Debug::LogLevel::Info;
        }

        AZ_TracePrintf("AngelScript", "[%s] (%d, %d) %s : %s", msg->section, msg->row, msg->col, typeStr, msg->message);
    }

    asIScriptEngine* AngelScriptSystemComponent::GetScriptEngine()
    {
        return m_scriptEngine;
    }

    asIScriptContext* AngelScriptSystemComponent::CreateContext()
    {
        if (m_scriptEngine)
        {
            return m_scriptEngine->CreateContext();
        }
        AZLOG_ERROR("AngelScript", "Cannot create context: Script engine is not initialized.");
        return nullptr;
    }

    asIScriptModule* AngelScriptSystemComponent::GetModule(const AZStd::string& moduleName)
    {
        if (m_scriptEngine)
        {
            return m_scriptEngine->GetModule(moduleName.c_str());
        }
        return nullptr;
    }

    bool AngelScriptSystemComponent::ExecuteString(const AZStd::string& /*scriptCode*/, const AZStd::string& /*moduleName*/)
    {
        if (!m_scriptEngine)
        {
            AZLOG_ERROR("AngelScript", "Cannot execute string: Script engine not initialized.");
            return false;
        }

        //CScriptBuilder builder;
        //int r = builder.StartNewModule(m_scriptEngine, moduleName.c_str());
        //if (r < 0)
        //{
        //    AZLOG_ERROR("AngelScript", "Failed to start new module for ExecuteString.");
        //    return false;
        //}

        //r = builder.AddSectionFromMemory("ExecuteString", scriptCode.c_str());
        //if (r < 0)
        //{
        //    AZLOG_ERROR("AngelScript", "Failed to add script code section.");
        //    return false;
        //}

        //r = builder.BuildModule();
        //if (r < 0)
        //{
        //    AZLOG_ERROR("AngelScript", "Failed to build module from string.");
        //    return false;
        //}

        //// For simple execution, find a main function or execute directly if the API supports it.
        //// This is a simplified example. Typically you'd call a specific function.
        //asIScriptModule* mod = builder.GetModule();
        //asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
        //if (func == nullptr)
        //{
        //    AZLOG_WARN("AngelScript", "ExecuteString did not find a 'void main()' function to run.");
        //    return false;
        //}

        //asIScriptContext* ctx = CreateContext();
        //ctx->Prepare(func);
        //r = ctx->Execute();

        bool success = false;
        //bool success = (r == asEXECUTION_FINISHED);

        //ctx->Release();

        return success;
    }

    bool AngelScriptSystemComponent::RegisterGlobalFunction(const char* declaration, const void* funcPointer)
    {
        if (m_scriptEngine)
        {
            int r = m_scriptEngine->RegisterGlobalFunction(declaration, asFUNCTION(funcPointer), asCALL_CDECL);
            AZ_Assert(r >= 0, "Failed to register global function: %s", declaration);
            return r >= 0;
        }
        return false;
    }

    bool AngelScriptSystemComponent::RegisterGlobalProperty(const char* declaration, void* propertyPtr)
    {
        if (m_scriptEngine)
        {
            int r = m_scriptEngine->RegisterGlobalProperty(declaration, propertyPtr);
            AZ_Assert(r >= 0, "Failed to register global property: %s", declaration);
            return r >= 0;
        }
        return false;
    }

    void AngelScriptSystemComponent::InitializeAngelScriptEngine()
    {
        AZLOG_INFO("AngelScript", "Initializing AngelScript Engine...");
        m_scriptEngine = asCreateScriptEngine();
        if (!m_scriptEngine)
        {
            AZLOG_ERROR("AngelScript", "Failed to create AngelScript engine.");
            return;
        }

        // Set the message callback to receive information on errors in scripts.
        int r = m_scriptEngine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
        AZ_Assert(r >= 0, "Failed to set AngelScript message callback.");

        // Register standard add-ons
        //RegisterStdString(m_scriptEngine);

        // TODO: Register O3DE types and functions here.
        // For example: Registering vector types, entity manipulation functions, etc.

        AZLOG_INFO("AngelScript", "AngelScript Engine Initialized.");
    }

    void AngelScriptSystemComponent::ShutdownAngelScriptEngine()
    {
        if (m_scriptEngine)
        {
            AZLOG_INFO("AngelScript", "Shutting down AngelScript Engine...");
            m_scriptEngine->ShutDownAndRelease();
            m_scriptEngine = nullptr;
            AZLOG_INFO("AngelScript", "AngelScript Engine Shutdown.");
        }
    }

    void AngelScriptSystemComponent::CreateScriptWorkspace()
    {
        // Create a dedicated directory for AngelScript files within the project's user folder.
        // This makes it easy for users to manage their scripts.
        const char* userPath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@user@");
        if (userPath)
        {
            m_scriptWorkspacePath = userPath;
            m_scriptWorkspacePath += "/AngelScript";

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(m_scriptWorkspacePath.c_str()))
            {
                AZLOG_INFO("AngelScript", "Creating AngelScript workspace at: %s", m_scriptWorkspacePath.c_str());
                AZ::IO::FileIOBase::GetInstance()->CreatePath(m_scriptWorkspacePath.c_str());
            }
            else
            {
                AZLOG_INFO("AngelScript", "AngelScript workspace found at: %s", m_scriptWorkspacePath.c_str());
            }
        }
        else
        {
            AZLOG_ERROR("AngelScript", "Could not resolve @user@ alias to create script workspace.");
        }
    }


} // namespace AngelScript
