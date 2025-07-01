
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/AngelScriptSystemComponent.h>

namespace AngelScript
{
    /// System component for AngelScript editor
    class AngelScriptEditorSystemComponent
        : public AngelScriptSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = AngelScriptSystemComponent;
    public:
        AZ_COMPONENT_DECL(AngelScriptEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        AngelScriptEditorSystemComponent();
        ~AngelScriptEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace AngelScript
