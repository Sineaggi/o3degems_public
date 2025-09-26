
class ASComponentTest
{

    
};


/*
COMPONENT("My Component", "A cool component from AngelScript")
ATTRIBUTES(AooearsInAddComponentMenu="Game", AutoExpand=true, Visibility=ShowChildrenOnly)
class MyComponent
{

// Use a more UE-like syntax for reflection?

    PROPERTY(Name="My Float Value", "A cool float value", Min=0.0f, Max=1.0f, Suffix="cm", ChangeHandler=OnFloatChange, UIHandler=Slider)
    float m_floatValue;


    void OnFloatChange()
    {
        Log("Float Value Changed"); // todo, verify how to do formatting
    }

}
*/

/*
This I would expect to result in code like this:

class ASMyComponent : public ASComponent
{
    AZ_COMPONENT(ASMyComponent, "{4DC25339-2361-4E00-BF69-D3B5C7C40CB1}", ASComponent);

    static void Reflect(AZ::ReflectContext* context)
    {
        ASComponent::Reflect(context);

        if (AZ::SerializeContext * serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ASMyComponent, ASComponent>()
                ->Field("m_floatValue", &ASMyComponent::m_floatValue)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StarsComponentConfig>("My Component", "A cool component from AngelScript")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Game") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ASMyComponent::m_floatValue, "My Float Value", "A cool float value")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "cm")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            }
        }
    }
};


then the next question is...

can I register this component into the engine dynamically? currently we use Descriptors, I believe we can, but I'll need to figure out how, maybe with Nick's help



*/