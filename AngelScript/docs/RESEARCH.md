# AngelScript for O3DE — Research & Implementation Dossier

*Compiled 2026-07-26. Research: 6 parallel agents over the O3DE codebase (development branch, `f6e9f8c3d1`) and the web, plus a hands-on review of lsemp3d's AngelScript gem branch. Intended as the reference document for continuing this project on a Windows development machine.*

> Source of record: Claude artifact `angelscript-o3de-research.md` — https://claude.ai/code/artifact/8cda62f2-914a-4aad-9b05-aab6abd8a130

---

## Contents

1. [Executive summary](#1-executive-summary)
2. [O3DE scripting today: how deep Lua really goes](#2-o3de-scripting-today-how-deep-lua-really-goes)
3. [The C++ boundary: what no script language can do (yet)](#3-the-c-boundary)
4. [The binding architecture a new language plugs into](#4-the-binding-architecture)
5. [In-tree precedents: Python, Script Canvas, ScriptEvents](#5-in-tree-precedents)
6. [AngelScript: the language and runtime](#6-angelscript-the-language-and-runtime)
7. [Hazelight's UE-AngelScript: the design template](#7-hazelights-ue-angelscript)
8. [Alternatives considered](#8-alternatives-considered)
9. [O3DE community context](#9-o3de-community-context)
10. [lsemp3d's AngelScript gem: state assessment](#10-lsemp3ds-angelscript-gem)
11. [Implementation roadmap](#11-implementation-roadmap)
12. [Windows migration notes](#12-windows-migration-notes)
13. [Key references](#13-key-references)

---

## 1. Executive summary

Adding AngelScript to O3DE as a gameplay scripting language is **feasible as a community gem with no engine fork**. O3DE's `BehaviorContext` is a language-agnostic runtime reflection database — the exact thing Hazelight had to mine out of Unreal's UObject system to build their production-proven UE-AngelScript fork. Anything already exposed to Lua and Script Canvas (~554 reflecting files across ~52 gems) can be auto-exposed to AngelScript with zero per-class binding work.

Key conclusions:

- **Lua's integration is deep but the language is the ceiling**: dynamic typing kills IDE tooling and compile-time safety; interpreted Lua 5.4 (no JIT in O3DE) limits performance. The C++/script *boundary* is a reflection boundary, not a Lua boundary — a better language doesn't move it, but makes the scriptable side far more usable.
- **AngelScript is a strong fit**: statically typed, C++-like syntax, zlib license, and a registration API (generic calling convention + per-registration auxiliary pointer) that is almost point-for-point what a reflection-driven binder needs. Proven at AAA scale (Split Fiction: 1.7M+ lines, 16k script files, PC/PS5/XSX).
- **Performance model** (per Hazelight): interpreted VM during development (with hot reload), precompiled bytecode in packaged builds, and an optional transpile-to-C++ path for shipping that "approaches native C++ performance." Interpreted AS is roughly 1.5–2× faster than interpreted Lua.
- **A head start exists**: O3DE maintainer lsemp3d has an experimental AngelScript gem (foundations only — asset pipeline scaffolding, engine init; no compilation or execution yet). It's Windows-first, which aligns with the planned move to a Windows dev machine. Roughly a day of focused work separates it from "hello world script executing on an entity"; the real milestone after that is the BehaviorContext binding walker.
- **Community positioning**: every language proposal since 2021 (C#, Rust) died unanswered; O3DE's stated mechanism for new languages is exactly "a BehaviorContext gem." The current community energy (hot-reload discussion #19214, Luau issue #19085, the O3DESharp C# gem) makes this well-timed.

---

## 2. O3DE scripting today: how deep Lua really goes

O3DE embeds **vanilla Lua 5.4.4** (not LuaJIT) in `AZ::ScriptContext` (`Code/Framework/AzCore/AzCore/Script/ScriptContext.cpp`, ~6,160 lines — the reference implementation for any new language backend). All binding is generic and driven by BehaviorContext metadata; there is no per-class glue code anywhere.

**Runtime integration:**

- `ScriptSystemComponent` owns contexts, calls `BindTo(behaviorContext)` on activate, steps the GC incrementally per tick, and replaces Lua's `require()` with an asset-system-backed hook so modules load and hot-reload through the AssetManager.
- Scripts attach to entities via `AzFramework::ScriptComponent`: the script returns a table; a per-entity instance table is created (prototype OO), `self.entityId` injected, `OnActivate`/`OnDeactivate` called.
- **Editor properties**: a script's `Properties` table is inspected by `ScriptEditorComponent` and surfaced as typed, attributed Inspector fields (min/max/sliders from Lua attribute tables), with per-entity overrides that survive script edits.
- **Asset pipeline**: `LuaBuilderWorker` (`Gems/LmbrCentral/Code/Source/Builders/LuaBuilder/`) compiles `.lua` → `.luac` bytecode with `require()` dependency scanning. Hot reload is end-to-end: save file → Asset Processor recompiles → all entities re-activate with new script, preserving editor-tweaked properties.
- **EBus, both directions**: send (`Bus.Broadcast.Foo`, `Bus.Event.Foo(id, ...)`, queued variants) and receive (`Bus.Connect(table, id)` via `LuaEBusHandler` + generic hooks), including `TickBus` for per-frame `OnTick`.
- **API surface** = BehaviorContext's reach: ~554 reflecting .cpp files across ~52 gems — Atom component buses (materials, lights, post-FX), PhysX scene queries, EMotionFX anim params, LyShine UI, input, spawning (`SpawnableScriptMediator`), navigation, audio, full math library. Script Canvas compiles to Lua and consumes the *same* reflection, so their API surfaces are near-identical.
- **Tooling**: standalone Lua IDE (`Code/Tools/LuaIDE`) with breakpoints, watches, and completion driven by the reflected class catalog; remote debugging via `ScriptDebugAgent` (`AzFramework/Script/ScriptRemoteDebugging.cpp`) over a RemoteTools protocol.

---

## 3. The C++ boundary

Where a gameplay programmer **must** drop into C++ today — these limits belong to the reflection architecture, not to Lua, and apply equally to any new language until addressed:

| Capability | Why C++ is required | Notes |
|---|---|---|
| New component types | Scripts ride inside the single `ScriptComponent`; no script path to `AZ_COMPONENT`, provided/required services, dependency sorting | lsemp3d's gem sketches per-script-class descriptors — a genuine improvement opportunity |
| Native (typed) EBuses | EBuses are C++ templates; script can only bind already-reflected buses | ScriptEvents gem covers script-world events (dynamic dispatch, no typed C++ handler) |
| Perf-critical inner loops | Marshaling per boundary crossing + interpreted VM | AngelScript narrows but doesn't eliminate this |
| Atom render features | FeatureProcessors, custom passes, shaders/SRGs have no BehaviorContext surface | Only component-level render buses are reflected |
| Multiplayer replication | Auto-components are XML→Jinja→C++ codegen; net properties/RPCs/prediction are generated C++ | Script can only observe session state |
| Asset builders | AssetBuilderSDK is C++ (or Python via PythonAssetBuilder) | |
| Editor automation | Python-only, partitioned by `Script::Attributes::Scope` (Launcher vs Automation) | |
| Any unreflected API | Exposing a new engine API to script always means writing BehaviorContext reflection in C++ | The permanent boundary |

---

## 4. The binding architecture

`AZ::BehaviorContext` (`Code/Framework/AzCore/AzCore/RTTI/BehaviorContext.h`) is explicitly documented as serving "different scripting systems (i.e. Lua, Visual Script, etc.)" and contains no Lua types. The data model a new backend consumes:

- **`BehaviorClass`** — full class descriptor: allocator hooks, constructors/destructor/cloner, methods, properties, base classes, attributes, wrapped-type unwrapper (smart pointers).
- **`BehaviorMethod`** — abstract callable: `Call(span<BehaviorArgument>, result)`, arity/default-arg info, overload chains.
- **`BehaviorProperty`** — getter/setter method pairs.
- **`BehaviorEBus` / `BehaviorEBusEventSender`** — per-event broadcast/event/queue methods + handler factories.
- **`BehaviorEBusHandler`** — the key bridge for receiving events: `InstallGenericHook(eventName, hook, userData)` with a deliberately language-neutral C callback signature.
- **Enumeration**: public registries `m_classes`, `m_methods`, `m_properties`, `m_ebuses`; live updates via `BehaviorContextEvents` bus (so gems loaded after VM creation still bind).
- **Attribute contract** (`ScriptContextAttributes.h`): `Ignore`, `Storage` (ScriptOwn/RuntimeOwn/Value — the GC contract), `Operator`, `Scope` (Launcher/Automation/Common), `ExcludeFrom`, `Module`, `Alias`, `ReaderWriterOverride`. A gameplay language filters on `Launcher`/`Common` (as Script Canvas does); Python filters the mirror image.

### The 12-item implementation checklist for a new language gem

1. **VM context class** (≈ `ScriptContext`): lifecycle, `BindTo()`, execute, GC control. *Small.*
2. **Binding walker**: enumerate registries, honor attributes, subscribe to `BehaviorContextBus`. *Medium (~1–2 KLOC).*
3. **Method-dispatch thunk** (≈ `LuaScriptCaller`): script args → `BehaviorArgument[]` → `BehaviorMethod::Call` → result; default args, `this`, overloads, temp-value destruction. *Medium-high; correctness-critical.*
4. **Type marshaling**: primitives, enums, strings, object wrappers keyed by typeId, RTTI casts, containers (analogue of `AzStdOnDemandReflection`). *The biggest single chunk.*
5. **Ownership/GC integration** honoring `Storage` attribute + `Operator` metamethods. *Medium; subtle bugs live here.*
6. **EBus support**: senders (method bindings) + handlers (`m_createHandler` → `InstallGenericHook` → script closures; mirror `LuaEBusHandler`). *Medium (~1 KLOC).*
7. **Asset type + runtime AssetHandler** (≈ `ScriptAsset`/`ScriptSystemComponent`) with reload handling and a module/import resolver. *Medium.*
8. **Asset builder** for `*.as` (copy `LuaBuilder`): CreateJobs w/ dependency parse, ProcessJob w/ compile + bytecode product. *Small-medium (~500 LOC).*
9. **Entity component + editor component** (≈ `ScriptComponent`/`ScriptEditorComponent`): per-entity instance, Inspector-editable script properties. *Medium.*
10. **Gem packaging** per `Templates/DefaultGem`. *Boilerplate.*
11. *(Deferrable)* **Debugging**: analogue of `ScriptContextDebug` + hook the existing `ScriptDebugAgent` remote protocol. *Large.*
12. *(Deferrable)* **Cross-system interop**: ScriptEvents (mostly free — see §5), `Scope`/`Module`/`Alias` handling.

Total scale: the Lua reference is ~10 KLOC in AzCore plus builder/component/tooling. Items 3–5 dominate.

---

## 5. In-tree precedents

**EditorPythonBindings** (`Gems/EditorPythonBindings`) — proof the whole binding can live in a gem without touching AzCore. Embeds CPython 3.10 via pybind11; *nothing* hand-bound — `PythonReflectionComponent` walks BehaviorContext at runtime; one generic `PythonProxyObject` wraps any `BehaviorObject`; one generic `PythonProxyBus` handles EBuses; `PythonMarshalComponent` is an extensible per-TypeId marshaling bus. Editor-only by construction (`PAL_TRAIT_BUILD_HOST_TOOLS` guard; no Clients/Servers targets) and by attribute (`Scope::Automation` filter). Also exports symbol stubs for IDE autocomplete — a polish item to copy.

**Script Canvas** (`Gems/ScriptCanvas`) — not a graph interpreter: it parses graphs into a language-independent IR (`Grammar::AbstractCodeModel` — "for easier translation into C++, Lua, or whatever else") and **compiles to Lua** in the Asset Processor, running on the stock AzCore VM. Its node palette is generated from BehaviorContext with `Scope::Launcher` filtering. Proves BehaviorContext is sufficient to express a complete gameplay language.

**ScriptEvents** (`Gems/ScriptEvents`) — script-defined event buses, language-agnostic by design: a definition (asset or procedural) is turned into a **real `AZ::BehaviorEBus` synthesized at runtime and injected into the global BehaviorContext**, tagged `RuntimeEBusAttribute`. Any language that walks BehaviorContext gets send/receive/define of script events *for free*. Do not invent a parallel event system.

**C#/Mono/.NET in-tree**: none — confirmed by exhaustive grep.

---

## 6. AngelScript: the language and runtime

- **Version/health**: 2.38.0 stable (Aug 2025), 2.39.0 WIP with commits through July 2026. ~1 feature release/year since 2003. Single maintainer (Andreas Jönsson) — a remarkably steady bus-factor-of-one. Official repo moved to GitHub in 2025: `github.com/anjo76/angelscript`. **License: zlib.**
- **Platforms**: Windows/Linux/macOS/iOS/Android/BSD + consoles (Hazelight ships PS5/XSX; Nintendo shipped Paper Mario TTYD on Switch with it). Native calling conventions on 28+ platform/arch combos; where absent, the portable generic calling convention works everywhere (including WASM under Emscripten).
- **JIT**: none production-grade. Stock runtime is a bytecode VM; JIT interface redesigned in 2.37. BlindMindStudios JIT is dead (x86, targets 2.31); `angelsea` (2025, bytecode→C→MIR) is promising but experimental. **Plan around interpreter performance** — which still benchmarks ~1.5–2× faster than interpreted Lua thanks to static typing.
- **Memory**: refcounting for deterministic destruction + incremental generational GC only for cycles; `asOBJ_NOCOUNT` lets the engine own lifetimes entirely — very relevant for entity/component handles. No stop-the-world tracing GC (see Unity cautionary tale, §8).
- **Threading**: engine thread-safe; contexts can execute in parallel; module compilation serialized.
- **Binding API — the reflection-DB fit (the decisive question)**: registration is programmatic against `asIScriptEngine` with string declarations (`RegisterObjectType/Behaviour/Method/Property`, `RegisterGlobalFunction`, funcdefs, interfaces, templates). The **generic calling convention** (`asCALL_GENERIC`) + per-registration **auxiliary void\*** is exactly the shape a BehaviorContext-driven binder needs: one universal trampoline per `BehaviorMethod`, with the `BehaviorMethod*` stashed as auxiliary — structurally identical to how O3DE's Lua binding drives `lua_CFunction` closures. Using generic-only also neutralizes AngelScript's main weakness (per-platform assembly thunk fragility). Caveats: generate valid AS declaration strings from reflection (name sanitization, type mapping); two-pass registration (types, then members); classify each `BehaviorClass` as `asOBJ_VALUE` vs `asOBJ_REF`/`asOBJ_NOCOUNT` (maps well onto the `Storage` attribute); startup registration cost is real but proven tolerable at UE scale.
- **Ergonomics**: static typing, classes/interfaces/single inheritance, mixins, object handles (`@`), funcdefs (typed delegates — natural EBus match), property accessors, try/catch, lambdas, named args, `foreach`/variadics/templates (2.38). Context suspend/resume enables coroutines. Hot reload is app-driven; the CSerializer add-on preserves state across reloads.
- **Tooling**: `angel-lsp` (VS Code language server, active since 2024, engine-agnostic); Hazelight's `vscode-unreal-angelscript` (full LSP + debug adapter — UE-specific but the architectural model to copy); core library ships debugging hooks (line callbacks, stack/variable inspection) + a CDebugger add-on.
- **Shipped games (vanilla AS)**: Amnesia/SOMA (Frictional HPL), Overgrowth, Nightdive KEX remasters, Paper Mario TTYD (Switch), SpellForce 3, Dustforce, SuperTuxKart, Urho3D engine.
- **Honest weaknesses**: one-person upstream; no mature JIT; small hiring pool/community (one GameDev.net forum — though the author answers personally); young engine-agnostic tooling; startup registration cost; refcount cycle discipline for binder authors.

---

## 7. Hazelight's UE-AngelScript

The single most important precedent (https://angelscript.hazelight.se/). Five pillars, with O3DE translations:

1. **Auto-bind everything from engine reflection at startup.** The plugin walks all UObject reflection data — every BlueprintType UCLASS/USTRUCT/UFUNCTION/UPROPERTY/UENUM — thousands of types, zero hand-written bindings, opt-out via metadata tags. Guiding principle: *"If it can be used from Blueprint, it should be usable from Angelscript."* → O3DE: walk BehaviorContext with `Scope::Launcher` filtering; principle becomes *"if Lua/Script Canvas can use it, AngelScript can."*
2. **Delegate script-object lifetimes to the engine.** Script classes *are* UObjects in Unreal's GC; AngelScript's own refcounting is bypassed for objects. → O3DE: honor the `Storage` attribute; lean on `asOBJ_NOCOUNT` where the engine owns lifetime.
3. **Hot reload in-editor, and during play for non-structural edits** (structural changes need a PIE restart, never an editor restart).
4. **VS Code LSP + debug adapter that connects to the running editor** as the source of truth — completions and diagnostics come from the real reflected API, breakpoints hit in the live session. A JetBrains plugin now exists too.
5. **Shipping path**: precompiled bytecode cache (skip parse/compile at startup) + optional **script-to-C++ transpilation** hooked through the AS JIT interface — "approaches native C++ performance," always "significantly better than Blueprint."

**Performance answer to "is 1.7M lines interpreted?"**: in development, yes — VM execution (their fork optimized the execution layer). In shipping, bytecode-cached VM at minimum, transpiled-to-native optionally. Good enough to ship It Takes Two, Split Fiction, THE FINALS, ARC Raiders, Talos Principle 2, Titan Quest II, Gothic 1 Remake.

**Their stated rationale for AngelScript over Lua/Blueprint**: Blueprint spaghetti unmaintainable at scale; C++ iteration too slow; wanted one *text* language shared by programmers and technical designers; static typing is what makes the LSP tooling and the C++ transpile path possible. Note: theirs is a *customized* AS requiring an engine fork — **O3DE's gem system means we can avoid that cost entirely**, which was Hazelight's biggest.

Status: Epic never adopted it (their answer is Verse/UE6); the fork remains third-party and keeps gaining studio adoption anyway.

---

## 8. Alternatives considered

| Option | Verdict |
|---|---|
| **Luau** (Roblox typed Lua, MIT, maintained native codegen; Alan Wake 2, Warframe) | Strongest rival. Open O3DE issue #19085 proposes swapping it in for Lua 5.4; would reuse the existing binding path nearly wholesale. Downside: typing is bolt-on — needs generated `.d.luau` defs from reflection to make tooling useful; AngelScript's typed registration gives that natively. |
| **C# (.NET hosting)** | An active third-party gem already exists: **WatchDogStudios/O3DESharp** (Jan 2026–, .NET 9 via Coral fork, hot reload, BehaviorContext reflection, Windows/Linux; very active as of July 2026). Watch it. Heavier runtime, tracing-GC tradeoffs, console/AOT complexity — but best IDE/hiring story. |
| **Unity's C#** (cautionary tale) | Boehm GC hitches spawned an entire culture of allocation avoidance; incremental GC only mitigates. Argument for AngelScript's deterministic refcounting. IL2CPP validates the "interpret in dev, AOT for shipping" pipeline (= Hazelight's transpiler). |
| **Godot's layering** (structural template) | `ScriptLanguage`/`Script`/`ScriptInstance` trio + machine-readable `extension_api.json` reflection dump. Steal the dump idea: emit BehaviorContext as JSON for offline tooling/LSP stubs/docs. |
| **Squirrel** | Valve VScript pedigree, but stagnant upstream; niche now better filled by Luau/AS. |
| **Wren** | Elegant, maintenance mode, no typed registration surface, near-zero shipped games. No. |
| **QuickJS/JS** | quickjs-ng is alive; TS tooling is tempting via generated `.d.ts` — but GC pauses + object-model impedance make it a middling gameplay fit. |
| **Haxe** | Compile-to-target model doesn't fit runtime reflection binding or hot reload; Armory3D is moribund. No. |
| **ActionScript** (original question) | Dead platform (Flash EOL 2020); its spiritual successors are Haxe (see above) and — in "C-family language for gameplay scripting" spirit — AngelScript itself. |

**Prior art in the CryEngine→Lumberyard→O3DE lineage**: none — this lineage has always been Lua (+ Script Canvas). An AngelScript gem is genuinely novel here, but O3DE is *better* positioned than Unreal was, because BehaviorContext is precisely the walk-and-bind surface Hazelight had to build against UObject internals.

---

## 9. O3DE community context

- **Official position** (unchanged since 2021): Script Canvas + Lua ship in core; new languages are community gems over BehaviorContext (stated plainly in discussion [#7916](https://github.com/o3de/o3de/discussions/7916)).
- **Dead proposals**: C# ([rfcs#41](https://github.com/o3de/rfcs/discussions/41), [o3de#2044](https://github.com/o3de/o3de/discussions/2044) — still open, zero maintainer engagement), Rust ([#2040](https://github.com/o3de/o3de/discussions/2040), [rfcs#44](https://github.com/o3de/rfcs/discussions/44)). No WASM-scripting prior art. No scripting roadmap RFC exists.
- **Live energy**:
  - [Issue #19085](https://github.com/o3de/o3de/issues/19085) (Jun 2025): swap Lua → **Luau**. Open, unanswered.
  - [Discussion #19214](https://github.com/o3de/o3de/discussions/19214) (Aug 2025–): hot reloading. Maintainers say C++ hot reload is now very hard; thread pivoted to scripting languages as the answer, naming **AngelScript** and pointing at lsemp3d's gem. Multiple companies expressed demand.
  - **O3DESharp** (Jan 2026–): first credible third-party C# runtime gem, actively developed.
- **No deprecation plans** for Lua or Script Canvas; both received optimizations in 2025.
- **Project health 2025–26**: Linux Foundation project, releasing on cadence (25.05, 25.10, 26.05 with new particle system); ~59 active contributors/quarter, downloads +187% YoY; effort weighted toward robotics/simulation and stability. Implication: upstream won't build this — a community gem is the expected and proven route (remote gem registry exists for distribution). No credible fork/schism found.

---

## 10. lsemp3d's AngelScript gem

Repo: https://github.com/lsemp3d/o3degems_public, branch `angelscript`. 13 commits, last activity 2025-09-07. ~2,400 lines of gem code + vendored AngelScript **2.37.0** SDK (includes the add-ons we need: `scriptbuilder`, `debugger`, `serializer`, `scriptstdstring`, `contextmgr`). Cited approvingly in upstream discussion #19214. README: *"VERY experimental… No actual AS compilation yet, still setting up the foundations."* — accurate.

### What's genuinely there

- **Proper O3DE gem structure**: `.API`/`.Private.Object`/module target layout, Clients/Servers/Tools/Builders aliases, PAL files for Windows/Mac/Linux/iOS/Android (Win/Mac/Linux marked supported), test scaffolding.
- **Engine lifecycle**: `asCreateScriptEngine()`, message callback → O3DE logging, ~13 engine properties driven from the Settings Registry (`Registry/angelscript_config.setreg`).
- **Asset plumbing**: `AngelScriptAsset` type + AssetHandler + registered `*.as` builder producing a product asset; asset-catalog scanning; a preprocessor stub.
- **A notable design sketch** — `ScanAndRegisterScriptComponents()`: enumerates script classes and registers **each AngelScript class as its own `ComponentDescriptor`** with a name-derived stable UUID → script classes would appear as first-class components in the Add Component menu. This goes *beyond* Lua's single-ScriptComponent model and is worth keeping.

### What's stubbed

- All `CScriptBuilder` compilation code commented out — builder emits an asset with a module name and **no bytecode**.
- Context pool (`RequestContext`/`ReturnContext`) never implemented → `CreateScriptObject()` always early-returns → **nothing can execute**.
- Runtime module loading (`LoadScript`) commented out; Inspector `DataElement` for assigning the script asset commented out.
- **Zero BehaviorContext binding**; the `Descriptions/*.h` headers are placeholders.
- Component lifecycle conventions are sketched and ready: `void OnCreate()`, `void OnTick(float)`, `void OnDestroy()` method lookup by declaration, class-name-matches-filename convention.

### Bugs to fix (platform-independent)

1. **Double engine creation**: `Init()` creates engine+context, then `Activate()→InitializeAngelScriptEngine()` creates a second engine — first is leaked.
2. **Shutdown crash**: `Deactivate()` nulls `m_scriptEngine`; destructor then calls `m_scriptEngine->ShutDownAndRelease()` unconditionally → null deref.
3. **Self-contradictory services**: `AngelScriptComponent` *provides*, *requires*, and declares *incompatible* the same `AngelScriptService` CRC (copy-paste error) — as written it can't coexist with the system component.
4. `AZLOG_ERROR("AngelScriptBuilder", false, msg)` — wrong signature; likely a compile error.
5. Uses deprecated single-arg `AZ_CRC` in places (minor).

### Portability notes (moot on Windows, relevant later)

- `#pragma optimize("", off/on)` is MSVC-only (fine on Windows; clang warns).
- `3rdParty/FindAngelScript.cmake` hardcodes `angelscript64$<IF:$<CONFIG:Debug>,d,>.lib` — **works as-is on Windows**; Mac/Linux need `libangelscript.a`. Long-term cleanest fix: compile the ~40 SDK sources directly into the gem as a static `ly_add_target`, eliminating the prebuilt-lib step on every platform.

---

## 11. Implementation roadmap

### Phase 0 — Get lsemp3d's gem running (≈ a focused day)

1. Fork the repo (so fixes can go upstream as PRs); add as submodule under `o3de\Gems`.
2. Build the vendored AS 2.37 lib (on Windows: open `External\angelscript_2.37.0\sdk\angelscript\projects\msvc2022`, build Debug + Release → `angelscript64d.lib`/`angelscript64.lib`) — or do the static-target vendoring now.
3. Fix bugs 1–5 above.
4. Wire the `scriptbuilder` add-on into the file lists; implement `ProcessJob` → `StartNewModule`/`AddSectionFromMemory`/`BuildModule` → `SaveByteCode` into the asset; runtime side loads via `LoadByteCode`.
5. Implement the context pool (`RequestContext`/`ReturnContext` — simple free-list on the system component).
6. Uncomment the script-asset `DataElement`; verify the sketched `OnCreate`/`OnTick`/`OnDestroy` flow.
7. Register the gem (`engine.json` `external_subdirectories` or `o3de register`), enable in a test project, run a hello-world `.as` on an entity.
8. *(Consider)* upgrade vendored AS 2.37.0 → 2.38.0.

### Phase 1 — BehaviorContext binding walker (the real project)

- `BindTo(BehaviorContext*)`: two-pass registration (types, then members) filtered on `Scope::Launcher`/`Common`, honoring `Ignore`/`ExcludeFrom`/`Alias`/`Module`.
- One generic trampoline per `BehaviorMethod` via `asCALL_GENERIC` with the `BehaviorMethod*` as auxiliary; marshal through `BehaviorArgument` (mirror `LuaScriptCaller`).
- Type classification: `BehaviorClass` + `Storage` attribute → `asOBJ_VALUE` / `asOBJ_REF` / `asOBJ_NOCOUNT`.
- EBus senders (method bindings) and handlers (`m_createHandler` → `InstallGenericHook` → funcdef/delegate dispatch into script methods).
- Subscribe to `BehaviorContextBus` for late-loaded gems.
- Milestone: AngelScript reaches Lua parity (math, transforms, physics queries, input, EBus both directions) with static typing.

### Phase 2 — Editor experience

- Inspector-exposed script properties (mine the class's serializable properties; mirror `ScriptEditorComponent`'s dynamic edit-data or Script Canvas's `BuildVariableOverrides`).
- Per-script-class component descriptors (finish lsemp3d's sketch).
- Hot reload with state preservation (CSerializer add-on).
- ScriptEvents interop (mostly free via BehaviorContext).

### Phase 3 — Tooling & shipping

- Emit a machine-readable BehaviorContext dump (Godot `extension_api.json` model) → feed `angel-lsp` for completions; write a debug adapter against the existing `ScriptDebugAgent` remote protocol (Hazelight's "editor as source of truth" model).
- Precompiled bytecode already covered by the asset pipeline; investigate `angelsea` JIT or a Hazelight-style transpile path much later.

---

## 12. Windows migration notes

The move to Windows **removes obstacles** for this project:

- lsemp3d's branch is Windows-first: the README workflow (git submodule into `o3de\Gems`, edit `engine.json`, `cmake . -B build\windows`, build the AS SDK via the bundled MSVC 2022 solution) was written for exactly this setup, and `FindAngelScript.cmake` works unmodified on Windows.
- The MSVC-only pragmas compile clean; the clang portability fixes drop out of the critical path (do them eventually for CI/other platforms).
- O3DE's primary developer platform is Windows (Editor, Asset Processor, and most contributors); Hazelight's tooling precedents are Windows-native too.

Setup checklist for the new machine:

1. Prereqs: VS 2022 (C++ workload), CMake ≥3.22, Git (+ LFS), Windows SDK.
2. Clone o3de; `python\get_python.bat`; register engine (`scripts\o3de.bat register --this-engine`).
3. Fork + submodule the gem: `cd o3de\Gems && git submodule add <your-fork> o3degems_public`.
4. Add `"Gems/o3degems_public/AngelScript"` to `engine.json` `external_subdirectories`.
5. Build AS SDK libs (Debug + Release) from `External\angelscript_2.37.0\sdk\angelscript\projects\msvc2022`.
6. `cmake . -B build\windows` — expect the `ANGELSCRIPT: 2.37.0` / `FIND ANGELSCRIPT_*` configure output from the README.
7. Create/register a minimal test project; enable the AngelScript gem in it; build Editor + Asset Processor.
8. Start at Phase 0, step 3 (bug fixes).

Nothing from the Mac session needs migrating besides this document — no code was modified; the clone lives in a session scratchpad.

---

## 13. Key references

**O3DE code (paths in this repo)**

- `Code/Framework/AzCore/AzCore/RTTI/BehaviorContext.h` — the reflection DB
- `Code/Framework/AzCore/AzCore/Script/ScriptContext.cpp` — Lua backend (reference implementation)
- `Code/Framework/AzCore/AzCore/Script/ScriptContextAttributes.h` — the attribute contract
- `Code/Framework/AzFramework/AzFramework/Script/ScriptComponent.*` + `AzToolsFramework/ToolsComponents/ScriptEditorComponent.*` — entity component pattern
- `Gems/LmbrCentral/Code/Source/Builders/LuaBuilder/` — asset builder template
- `Gems/EditorPythonBindings/` — gem-hosted language integration template
- `Gems/ScriptCanvas/`, `Gems/ScriptEvents/` — IR/translation and cross-language events
- `Code/Framework/AzFramework/AzFramework/Script/ScriptRemoteDebugging.cpp` + `Code/Tools/LuaIDE/` — debugging infrastructure

**External**

- AngelScript: https://angelcode.com/angelscript/ · GitHub: https://github.com/anjo76/angelscript · Generic convention: https://angelcode.com/angelscript/sdk/docs/manual/doc_generic.html
- Hazelight UE-AngelScript: https://angelscript.hazelight.se/ · Auto-bindings: https://angelscript.hazelight.se/cpp-bindings/automatic-bindings/ · Precompiled/transpile: https://angelscript.hazelight.se/cpp-bindings/precompiled-data/ · VS Code ext: https://github.com/Hazelight/vscode-unreal-angelscript
- lsemp3d's gem: https://github.com/lsemp3d/o3degems_public/tree/angelscript
- angel-lsp: https://github.com/sashi0034/angel-lsp · angelsea JIT: https://github.com/asumagic/angelsea
- O3DE threads: hot reload https://github.com/o3de/o3de/discussions/19214 · Luau https://github.com/o3de/o3de/issues/19085 · policy answer https://github.com/o3de/o3de/discussions/7916
- O3DESharp (C# gem): https://github.com/WatchDogStudios/O3DESharp
