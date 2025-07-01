
#include "AngelScriptSystemComponent.h"

#include <AngelScript/AngelScriptTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>


namespace AngelScript
{
    AZ_COMPONENT_IMPL(AngelScriptSystemComponent, "AngelScriptSystemComponent",
        AngelScriptSystemComponentTypeId);

    void AngelScriptSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptSystemComponent, AZ::Component>()
                ->Version(0)
                ;
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
    }

    void AngelScriptSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AngelScriptRequestBus::Handler::BusDisconnect();
    }

    void AngelScriptSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace AngelScript
