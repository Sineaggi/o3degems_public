# o3degems_public

# AngelScript

This requires fairly advances O3DE engineering knowledge, it's VERY experimental.

To get this to work I did:

cd o3de\Gems
git submodule add https://github.com/lsemp3d/o3degems_public.git

```
cd ..
code engine.json
```
to open Visual Studio Code

Under external_subdirectories I added:

`"Gems/o3degems_public/AngelScript"`

cmake . -B build\windows

you should see:

```
  ANGELSCRIPT: 2.37.0
  FIND ANGELSCRIPT_3RDPARTY_ROOT_DIRECTORY: D:/o3de/Gems/o3degems_public/AngelScript/3rdParty/../External/angelscript_2.37.0
  FIND ANGELSCRIPT_LIB_PATH: D:/o3de/Gems/o3degems_public/AngelScript/3rdParty/../External/angelscript_2.37.0/sdk/angelscript/lib/angelscript64$<IF:$<CONFIG:Debug>,d,>.lib
  FIND ANGELSCRIPT_LIB: angelscript64$<IF:$<CONFIG:Debug>,d,>.lib
-- Finishing up configuration...
```

Of course, relative to your own paths/folder structure.  

Open the O3DE project and you should see:

<img width="435" height="230" alt="image" src="https://github.com/user-attachments/assets/f9f481ca-6ae8-4d30-9859-9e9db648475f" />

It's very basic, at this point, all it does is initialize the AngelScript system, see:

```
 void AngelScriptSystemComponent::Init()
 {
     m_scriptEngine = asCreateScriptEngine();
     m_scriptContext = m_scriptEngine->CreateContext();
 }
```
in `o3de\Gems\o3degems_public\AngelScript\Code\Source\Clients\AngelScriptSystemComponent.cpp`

Open the Angelscript project and build it for both Debug and Release configurations, in Windows this is in:

```
Gems\o3degems_public\AngelScript\External\angelscript_2.37.0\sdk\angelscript\projects\msvc2022
```

This will produce the angelscript64d.lib and angelscript64.lib files necessary for linking into O3DE.

The fun work starts here! Let's get AngelScript natively supported in O3DE!
