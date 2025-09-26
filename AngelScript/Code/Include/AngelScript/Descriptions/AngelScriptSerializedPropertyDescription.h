#pragma once

namespace AngelScript
{

}

///**
// * Description of a script property that has been added as an unreal property.
// */
//struct FAngelscriptPropertyDesc
//{
//    /* Name of the property. */
//    FString PropertyName;
//
//    /* Literal type in script of the property. */
//    FString LiteralType;
//
//    /* Resolved type of the property. */
//    FAngelscriptTypeUsage PropertyType;
//
//    /* Metadata for the property. */
//    TMap<FName, FString> Meta;
//
//    /* Whether the property can be read in blueprint. */
//    bool bBlueprintReadable = false;
//
//    /* Whether the property can be written in blueprint. */
//    bool bBlueprintWritable = false;
//
//    /* Whether the property can be edited on defaults. */
//    bool bEditableOnDefaults = false;
//
//    /* Whether the property can be edited on instances. */
//    bool bEditableOnInstance = false;
//
//    /* Whether the property is shown in details views but cannot be changed. */
//    bool bEditConst = false;
//
//    /* Whether the property is considered a component reference. */
//    bool bInstancedReference = false;
//
//    /* Whether the property is considered a persistent reference, an object referenced by the property is duplicated like a component. */
//    bool bPersistentInstance = false;
//
//    /* Whether the property should be marked as advanced display. */
//    bool bAdvancedDisplay = false;
//
//    /* Whether the property is transient. */
//    bool bTransient = false;
//
//    /* Whether an FProperty exists in the class for this property. */
//    bool bHasUnrealProperty = false;
//
//    /* Whether the property should be replicated. */
//    bool bReplicated = false;
//
//    /* Whether the property should be skipped when replicating. */
//    bool bSkipReplication = false;
//
//    /* Whether to skip during serialization. */
//    bool bSkipSerialization = false;
//
//    /* Whether property should be serialized for save games. */
//    bool bSaveGame = false;
//
//    /* Specified replication condition. */
//    TEnumAsByte<ELifetimeCondition> ReplicationCondition = COND_None;
//
//    /* Whether we should call a function when this is replicated. */
//    bool bRepNotify = false;
//
//    /* Whether this is a config property read from ini files. */
//    bool bConfig = false;
//
//    /* Whether the property is exposed for Matinee or Sequencer to modify. */
//    bool bInterp = false;
//
//    /* Whether the property should be searchable in the Asset Registry. */
//    bool bAssetRegistrySearchable = false;
//
//    /* Whether the property should not be clearable (disallow being set to None). */
//    bool bNoClear = false;
//
//    /* Whether the property is private in angelscript. */
//    bool bIsPrivate = false;
//
//    /* Whether the property is protected in angelscript. */
//    bool bIsProtected = false;
//
//    /* Angelscript internal data for the property. */
//    int32 ScriptPropertyIndex = -1;
//    SIZE_T ScriptPropertyOffset = 0;
//
//    /* Line number in the file of the property. */
//    int32 LineNumber = 1;
//
//    bool IsDefinitionEquivalent(const FAngelscriptPropertyDesc& Other) const
//    {
//        return Other.bBlueprintReadable == bBlueprintReadable
//            && Other.bBlueprintWritable == bBlueprintWritable
//            && Other.bEditableOnDefaults == bEditableOnDefaults
//            && Other.bEditableOnInstance == bEditableOnInstance
//            && Other.bAdvancedDisplay == bAdvancedDisplay
//            && Other.bEditConst == bEditConst
//            && Other.bInstancedReference == bInstancedReference
//            && Other.bPersistentInstance == bPersistentInstance
//            && Other.bTransient == bTransient
//            && Other.bConfig == bConfig
//            && Other.bInterp == bInterp
//            && Other.bAssetRegistrySearchable == bAssetRegistrySearchable
//            && Other.bNoClear == bNoClear
//            && Other.bReplicated == bReplicated
//            && Other.ReplicationCondition == ReplicationCondition
//            && Other.bSkipReplication == bSkipReplication
//            && Other.bSkipSerialization == bSkipSerialization
//            && Other.bSaveGame == bSaveGame
//            && Other.bRepNotify == bRepNotify
//            && Other.bIsPrivate == bIsPrivate
//            && Other.bIsProtected == bIsProtected
//            ;
//    }
//};
