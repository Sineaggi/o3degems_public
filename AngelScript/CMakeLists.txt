
o3de_gem_setup("AngelScript")

set(ANGELSCRIPT_CUR_VER "2.38.0")

ly_add_external_target_path(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty)

message("ANGELSCRIPT: " ${ANGELSCRIPT_CUR_VER})

add_subdirectory(Code)
