#include "AngelScriptComponent.h"
#include <AzCore/Math/Crc.h>
//#include <AngelScript/AngelScriptBus.h>
//
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
//
//// AngelScript Headers
//#include <angelscript.h>
//#include <scriptbuilder/scriptbuilder.h>


namespace AngelScript
{
    void AngelScriptComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptComponent, AZ::Component>()
                ->Version(1)
                //->Field("ScriptAsset", &AngelScriptComponent::m_scriptAsset)
                ;

            if (AZ::EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<AngelScriptComponent>("AngelScript", "Executes an AngelScript file on an entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Script.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Script.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    //->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_scriptAsset, "Script", "The AngelScript asset to execute.")
                    //->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree"))
                    ;
            }
        }
    }

    void AngelScriptComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // This component requires the main system component to be active.
        required.push_back(AZ_CRC_CE("AngelScriptService"));
    }

    void AngelScriptComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
    {
    }

    void AngelScriptComponent::Activate()
    {
        if (m_scriptAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_scriptAsset.GetId());
            m_scriptAsset.QueueLoad();
        }

    }

    void AngelScriptComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        DestroyScriptObject();
    }

    void AngelScriptComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_scriptObject && m_onTickFunction)
        {
            //asIScriptContext* ctx = nullptr;
            //AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
            //if (ctx)
            //{
            //    ctx->Prepare(m_onTickFunction);
            //    ctx->SetObject(m_scriptObject);
            //    ctx->SetArgFloat(0, deltaTime);
            //    ctx->Execute();
            //    AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
            //}
        }
    }

    void AngelScriptComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        CreateScriptObject();
    }

    void AngelScriptComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Re-create the script object with the new version of the script.
        OnAssetReady(asset);
    }

    void AngelScriptComponent::CreateScriptObject()
    {
        DestroyScriptObject(); // Clean up any existing object first.

        if (!m_scriptAsset.IsReady())
        {
            return;
        }

        asIScriptEngine* engine = nullptr;
        AngelScriptRequestBus::BroadcastResult(engine, &AngelScriptRequestBus::Events::GetScriptEngine);
        if (!engine)
        {
            AZ_Error("AngelScript", false, "Cannot create script object for entity %s, AngelScript engine not available.", GetEntityId().ToString().c_str());
            return;
        }

        asIScriptModule* module = engine->GetModule(m_scriptAsset.Get()->m_moduleName.c_str());
        if (!module)
        {
            AZ_Error("AngelScript", false, "Module '%s' not found for asset %s", m_scriptAsset.Get()->m_moduleName.c_str(), m_scriptAsset.GetId().ToString<AZStd::string>().c_str());
            return;
        }

        // Convention: The class name inside the script must match the module name (filename without extension).
        AZStd::string className = m_scriptAsset.Get()->m_moduleName;
        AZStd::string::size_type dotPos = className.rfind('.');
        if (dotPos != AZStd::string::npos)
        {
            className = className.substr(0, dotPos);
        }

        asITypeInfo* type = module->GetTypeInfoByDecl(className.c_str());
        if (!type)
        {
            AZ_Error("AngelScript", false, "Class '%s' not found in module '%s'. The class name must match the filename.", className.c_str(), module->GetName());
            return;
        }

        // Create the object instance.
        asIScriptContext* ctx = nullptr;
        //TODO-LS: implement ReturnContext
        // AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
        if (!ctx) return;

        ctx->Prepare(type->GetFactoryByIndex(0));
        if (ctx->Execute() == asEXECUTION_FINISHED)
        {
            m_scriptObject = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
            m_scriptObject->AddRef();
        }
        //TODO-LS: implement ReturnContext
        ///AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);

        if (!m_scriptObject)
        {
            AZ_Error("AngelScript", false, "Failed to instantiate script object for class '%s'", className.c_str());
            return;
        }

        // Cache function pointers for lifecycle methods.
        m_onCreateFunction = type->GetMethodByDecl("void OnCreate()");
        m_onDestroyFunction = type->GetMethodByDecl("void OnDestroy()");
        m_onTickFunction = type->GetMethodByDecl("void OnTick(float)");

        // Call OnCreate if it exists.
        if (m_onCreateFunction)
        {
            //TODO-LS: implement ReturnContext
            ///AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
            if (ctx)
            {
                ctx->Prepare(m_onCreateFunction);
                ctx->SetObject(m_scriptObject);
                ctx->Execute();
                //TODO-LS: implement ReturnContext
                // AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
            }
        }

        // If an OnTick function exists, connect to the TickBus.
        if (m_onTickFunction)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void AngelScriptComponent::DestroyScriptObject()
    {
        if (m_scriptObject)
        {
            // Call OnDestroy if it exists.
            if (m_onDestroyFunction)
            {
                asIScriptContext* ctx = nullptr;
                //TODO-LS: implement ReturnContext
                //AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
                if (ctx)
                {
                    ctx->Prepare(m_onDestroyFunction);
                    ctx->SetObject(m_scriptObject);
                    ctx->Execute();
                    //TODO-LS: implement ReturnContext
                    // AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
                }
            }
            m_scriptObject->Release();
            m_scriptObject = nullptr;
        }
        m_onCreateFunction = nullptr;
        m_onDestroyFunction = nullptr;
        m_onTickFunction = nullptr;
    }

} // namespace AngelScript
