#include "AngelScriptComponent.h"
#include <AzCore/Math/Crc.h>
#include <AzCore/Component/TickBus.h>
#include <AngelScript/AngelScriptBus.h>
//
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
//
//// AngelScript Headers
#include <angelscript.h>
//#include <scriptbuilder/scriptbuilder.h>


namespace AngelScript
{
    void AngelScriptComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_scriptAsset", &AngelScriptComponent::m_scriptAsset)
                ;

            if (AZ::EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<AngelScriptComponent>("AngelScript", "Executes an AngelScript file on an entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/LuaScript.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/LuaScript.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AngelScriptComponent::m_scriptAsset, "Script", "The AngelScript asset to execute.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                    ;
            }
        }
    }

    void AngelScriptComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // Intentionally empty: the AngelScriptService is provided by AngelScriptSystemComponent,
        // not by per-entity script components. Multiple AngelScriptComponents may coexist on
        // different entities, so this component must not provide (or be incompatible with) the service.
    }

    void AngelScriptComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Intentionally empty: see GetProvidedServices. Declaring AngelScriptService incompatible here
        // would prevent this component from coexisting with the system component that provides it.
    }

    void AngelScriptComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // Intentionally empty. AngelScriptService is a SYSTEM-level service provided by
        // AngelScriptSystemComponent (a required system component). An entity component's required
        // services are resolved only against other components on the SAME entity, never against
        // system components -- so requiring it here would disable this component on every normal
        // entity ("missing a required component service"). The script system is reached at runtime
        // via AngelScriptRequestBus, which the system component services, so no dependency is needed.
    }

    void AngelScriptComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
    {
    }

    AZ::ComponentDescriptor* AngelScriptComponent::CreateDescriptor()
    {
        return aznew AngelScriptASComponentDescriptor();
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

    void AngelScriptComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        if (m_scriptObject && m_onTickFunction)
        {
            asIScriptContext* ctx = nullptr;
            AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
            if (ctx)
            {
                ctx->Prepare(m_onTickFunction);
                ctx->SetObject(m_scriptObject);
                ctx->SetArgFloat(0, deltaTime);
                ctx->Execute();
                AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
            }
        }
    }

    void AngelScriptComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_scriptAsset = asset;
        CreateScriptObject(false);
    }

    void AngelScriptComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Reload: rebuild the module from the new source. The module key (the AssetId) is stable
        // across reloads, so we must force a recompile rather than reuse the cached module.
        m_scriptAsset = asset;
        CreateScriptObject(true);
    }

    void AngelScriptComponent::CreateScriptObject(bool forceRecompile)
    {
        DestroyScriptObject(); // Clean up any existing object first.

        if (!m_scriptAsset.IsReady())
        {
            return;
        }

        // Key the engine module on the asset's AssetId -- unique and stable, so two scripts that
        // share a class name (same filename stem in different folders) don't collide into one
        // module. The class name is the filename stem (the authoring convention); the readable
        // source path is passed as the section name so compiler diagnostics stay legible.
        const AngelScriptAsset* scriptAsset = m_scriptAsset.Get();
        const AZStd::vector<char>& scriptBuffer = scriptAsset->m_scriptData.m_script;
        const AZStd::string source(scriptBuffer.data(), scriptBuffer.size());
        const AZStd::string moduleKey = m_scriptAsset.GetId().ToString<AZStd::string>();

        AZStd::string className = scriptAsset->m_moduleName;
        AZStd::string::size_type dotPos = className.rfind('.');
        if (dotPos != AZStd::string::npos)
        {
            className = className.substr(0, dotPos);
        }

        const AZStd::string& sectionName =
            scriptAsset->m_scriptData.m_debugName.empty() ? className : scriptAsset->m_scriptData.m_debugName;

        asIScriptModule* module = nullptr;
        AngelScriptRequestBus::BroadcastResult(
            module, &AngelScriptRequestBus::Events::EnsureModule, moduleKey, source, sectionName, forceRecompile);
        if (!module)
        {
            AZ_Error("AngelScript", false, "Failed to compile script '%s' for entity %s",
                sectionName.c_str(), GetEntityId().ToString().c_str());
            return;
        }

        asITypeInfo* type = module->GetTypeInfoByDecl(className.c_str());
        if (!type)
        {
            AZ_Error("AngelScript", false, "Class '%s' not found in script '%s'.", className.c_str(), sectionName.c_str());
            return;
        }

        // Instantiate via the type's factory.
        asIScriptContext* ctx = nullptr;
        AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
        if (!ctx)
        {
            AZ_Error("AngelScript", false, "No script context available to instantiate '%s'.", className.c_str());
            return;
        }

        ctx->Prepare(type->GetFactoryByIndex(0));
        if (ctx->Execute() == asEXECUTION_FINISHED)
        {
            m_scriptObject = *static_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
            m_scriptObject->AddRef();
        }
        AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);

        if (!m_scriptObject)
        {
            AZ_Error("AngelScript", false, "Failed to instantiate script object for class '%s'", className.c_str());
            return;
        }

        // Cache lifecycle methods.
        m_onCreateFunction = type->GetMethodByDecl("void OnCreate()");
        m_onDestroyFunction = type->GetMethodByDecl("void OnDestroy()");
        m_onTickFunction = type->GetMethodByDecl("void OnTick(float)");

        if (m_onCreateFunction)
        {
            asIScriptContext* createCtx = nullptr;
            AngelScriptRequestBus::BroadcastResult(createCtx, &AngelScriptRequestBus::Events::RequestContext);
            if (createCtx)
            {
                createCtx->Prepare(m_onCreateFunction);
                createCtx->SetObject(m_scriptObject);
                createCtx->Execute();
                AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, createCtx);
            }
        }

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
                AngelScriptRequestBus::BroadcastResult(ctx, &AngelScriptRequestBus::Events::RequestContext);
                if (ctx)
                {
                    ctx->Prepare(m_onDestroyFunction);
                    ctx->SetObject(m_scriptObject);
                    ctx->Execute();
                    AngelScriptRequestBus::Broadcast(&AngelScriptRequestBus::Events::ReturnContext, ctx);
                }
            }
            m_scriptObject->Release();
            m_scriptObject = nullptr;
        }
        m_onCreateFunction = nullptr;
        m_onDestroyFunction = nullptr;
        m_onTickFunction = nullptr;

        AZ::TickBus::Handler::BusDisconnect();
    }

    // --- AngelScriptASComponentDescriptor ---
    // Defined here (not in the header) so the header can be included by multiple translation units
    // without producing multiple-definition link errors.

    void AngelScriptASComponentDescriptor::Reflect(AZ::ReflectContext* reflection) const
    {
        AngelScriptComponent::Reflect(reflection);
    }

    void AngelScriptASComponentDescriptor::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetProvidedServices(provided);
    }

    void AngelScriptASComponentDescriptor::GetDependentServices(
        AZ::ComponentDescriptor::DependencyArrayType& dependent, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetDependentServices(dependent);
    }

    void AngelScriptASComponentDescriptor::GetRequiredServices(
        AZ::ComponentDescriptor::DependencyArrayType& required, [[maybe_unused]] const AZ::Component* instance) const
    {
        AngelScriptComponent::GetRequiredServices(required);
    }

} // namespace AngelScript
