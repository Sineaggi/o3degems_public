
#include <AzCore/Serialization/SerializeContext.h>
#include "AngelScriptEditorSystemComponent.h"

#include <AngelScript/AngelScriptTypeIds.h>

#include <AngelScript/AngelScriptAsset.h>
#include <AngelScriptAssetHandler.h>

namespace AngelScript
{
    AZ_COMPONENT_IMPL(AngelScriptEditorSystemComponent, "AngelScriptEditorSystemComponent",
        AngelScriptEditorSystemComponentTypeId, BaseSystemComponent);

    void AngelScriptEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AngelScriptEditorSystemComponent, AngelScriptSystemComponent>()
                ->Version(0);
        }
    }

    AngelScriptEditorSystemComponent::AngelScriptEditorSystemComponent() = default;

    AngelScriptEditorSystemComponent::~AngelScriptEditorSystemComponent() = default;

    void AngelScriptEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("AngelScriptEditorService"));
    }

    void AngelScriptEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("AngelScriptEditorService"));
    }

    void AngelScriptEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void AngelScriptEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void AngelScriptEditorSystemComponent::Activate()
    {
        AngelScriptSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void AngelScriptEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AngelScriptSystemComponent::Deactivate();
    }

} // namespace AngelScript
