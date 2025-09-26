#pragma once

class asITypeInfo;

namespace AngelScript
{
    // Description for classes during preprocess
    struct ClassDescription
    {
        AZStd::string m_className;

        /* Angelscript name of the class that should be the super for this angelscript class. */
        AZStd::string SuperClass;

        /* Actual UClass of the native class that backs this script type. */
        ///void* CodeSuperClass = nullptr;

        /* Whether the direct superclass of this is a code class. If false, it is an angelscript class. */
        bool m_bSuperIsCodeClass = false;

        /* Whether this is a generated statics class that does not actually exist in script. */
        bool m_bIsStaticsClass = false;

        /* Whether this class is abstract. */
        bool m_bAbstract = false;

        /* Whether all instances of this class should be transient. */
        bool m_bTransient = false;

        /* Whether this class is hidden from property combo boxes */
        bool bHideDropdown = false;

        /* Indicates that references to this class default to instanced. Used to be subclasses of UComponent, but now can be any UObject */
        bool bDefaultToInstanced = false;

        /* Class can be constructed from EditInlineNew New button. */
        bool bEditInlineNew = false;

        /* Whether this class is deprecated. */
        bool bIsDeprecatedClass = false;

        /* Whether the class can be placed in levels. */
        bool bPlaceable = true;

        /* Whether this class represents a struct or not. */
        bool bIsStruct = false;

        /* Name of the config file to use. */
        AZStd::string ConfigName;

        /* Internal angelscript class this is referencing. */
        asITypeInfo* ScriptType = nullptr;

        /* Generated UClass that this class should be instanced as. */
        ////UClass* Class = nullptr;

        /* Generated UStruct that this class should be instanced as. */
        ////UStruct* Struct = nullptr;

        /* Properties we're adding into unreal for this class. */
        ////TArray<TSharedRef<FAngelscriptPropertyDesc>> Properties;

        /* Functions we're adding into unreal for this class. */
        ////TArray<TSharedRef<FAngelscriptFunctionDesc>> Methods;

        /* The name of the global variable that should be set in script to this UClass. */
        ////AZStd::string StaticClassGlobalVariableName;

        /* The code used to set default properties for this class. */
        AZStd::string DefaultsCode;

        /* Line number in the file of the class. */
        //int32 LineNumber = 1;

        /* Metadata for the class. */
        //TMap<FName, AZStd::string> Meta;

        /* Composable class */
        AZStd::string ComposeOntoClass;

        /* This will be set when the class resides in a namespace that is NOT the modules default namespace. */
        ////TOptional<AZStd::string> Namespace;

        // Find the property descriptor by name
        //TSharedPtr<FAngelscriptPropertyDesc> GetProperty(const AZStd::string& PropName)
        //{
        //    for (auto PropDesc : Properties)
        //    {
        //        if (PropName.Equals(PropDesc->PropertyName))
        //        {
        //            return PropDesc;
        //        }
        //    }
        //    return nullptr;
        //}

        //TSharedPtr<FAngelscriptPropertyDesc> GetProperty(class asCString& PropName);

        //// Find the function descriptor by name
        //TSharedPtr<FAngelscriptFunctionDesc> GetMethod(const AZStd::string& FuncName)
        //{
        //    for (auto FuncDesc : Methods)
        //    {
        //        if (FuncName.Equals(FuncDesc->FunctionName))
        //        {
        //            return FuncDesc;
        //        }
        //    }
        //    return nullptr;
        //}

        //TSharedPtr<FAngelscriptFunctionDesc> GetMethodByScriptName(const AZStd::string& FuncName)
        //{
        //    for (auto FuncDesc : Methods)
        //    {
        //        if (FuncName.Equals(FuncDesc->ScriptFunctionName))
        //        {
        //            return FuncDesc;
        //        }
        //    }
        //    return nullptr;
        //}

        //bool AreFlagsEqual(const FAngelscriptClassDesc& Other) const
        //{
        //    return bAbstract == Other.bAbstract
        //        && bTransient == Other.bTransient
        //        && bHideDropdown == Other.bHideDropdown
        //        && bDefaultToInstanced == Other.bDefaultToInstanced
        //        && bEditInlineNew == Other.bEditInlineNew
        //        && bIsDeprecatedClass == Other.bIsDeprecatedClass
        //        && bPlaceable == Other.bPlaceable
        //        ;
        //}
    };

}

