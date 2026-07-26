
set(FILES
    Source/AngelScriptModuleInterface.cpp
    Source/AngelScriptModuleInterface.h
    Source/Clients/AngelScriptSystemComponent.cpp
    Source/Clients/AngelScriptSystemComponent.h
    Source/AngelScriptAsset.cpp

    Source/AngelScriptComponent.h
    Source/AngelScriptComponent.cpp
    Source/AngelScriptAssetHandler.h
    Source/AngelScriptAssetHandler.cpp
    Source/ScriptContextPool.h
    Source/ScriptContextPool.cpp

# Preprocessor
    Source/Preprocessor/AngelScriptPreprocessor.h
    Source/Preprocessor/AngelScriptPreprocessor.cpp

# Vendored AngelScript add-ons (compiled into the gem)
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.h
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp
)

# scriptbuilder is third-party; keep it out of the gem's unity blob to avoid
# symbol collisions and warning-as-error churn.
set(SKIP_UNITY_BUILD_INCLUSION_FILES
    ../External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp
)
