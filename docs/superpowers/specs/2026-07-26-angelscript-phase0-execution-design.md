# Design — Phase 0: AngelScript execution on an entity

*Date: 2026-07-26. Branch: `phase0-hello-world`. Follows the research dossier
(`AngelScript/docs/RESEARCH.md`) §11 Phase 0.*

## Goal

Get an AngelScript `.as` script to compile and execute on an O3DE entity: its
class is instantiated and its `OnCreate()` / `OnTick(float)` / `OnDestroy()`
lifecycle methods run. This is the last step of Phase 0; the gem already builds,
registers, and links, but nothing executes yet (context pool unimplemented,
`CScriptBuilder` commented out).

## Definition of done

- **0a — core (automated):** a gtest in `AngelScript.Tests` compiles a small
  `.as` from memory, instantiates its class, calls `OnCreate()` then
  `OnTick(dt)`, and asserts an observable side-effect (a native counter the
  script increments). A green test proves the execution core.
- **0b — demo (manual):** in the Editor, assign a `.as` asset to an
  `AngelScriptComponent` on an entity, enter game mode, and observe the
  `OnCreate` / `OnTick` log output.

## Key decisions

1. **Compile from source at runtime** (not a bytecode pipeline). The builder
   stores the `.as` *source text* in the asset; the runtime compiles it via
   `CScriptBuilder` on load. Rationale: AngelScript bytecode only loads when the
   runtime engine's registered-type configuration exactly matches the builder's.
   Phase 1 (the BehaviorContext binding walker) will churn that configuration
   constantly, so a bytecode pipeline would be brittle during development.
   Compiling from source recompiles against whatever the live engine has
   registered. This mirrors Hazelight's dev model; bytecode caching is deferred
   to a later shipping optimization.
2. **Test-first, then Editor demo.** Prove the execution core with a hermetic,
   deterministic gtest before touching Editor UI wiring; the test remains as
   regression coverage. Then wire the Inspector and do the on-entity demo.

## Components and data flow

### 1. Build wiring

Compile the vendored `scriptbuilder` add-on
(`External/angelscript_2.37.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp`) into
the gem and add `sdk/add_on/` as an include directory. `CScriptBuilder` is used
by both the builder worker (editor/builder object lib) and the runtime module
compile (runtime object lib), so the add-on source belongs in the shared file
lists so both `AngelScript.Private.Object` and `AngelScript.Editor.Private.Object`
compile it. The add-on is self-contained (`scriptbuilder.cpp` / `.h`); it does
not require `scriptstdstring` for this milestone.

### 2. Builder (`AngelScriptBuilderWorker::ProcessJob`, asset-time)

- Read the `.as` file.
- Store the **source text** into `AngelScriptAsset.m_scriptData.m_script`; set
  `m_moduleName` (filename stem) and `m_scriptData.m_debugName`.
- Output the product asset (existing `OutputObject` path).
- **Best-effort syntax check (optional nicety):** compile the source on a
  throwaway `asIScriptEngine` created locally in the job, routing compiler
  messages to job issues for early feedback in the Asset Processor. This is not
  a hard gate — the runtime compile is authoritative — and may be cut if it
  complicates the work.
- No bytecode is produced. `#include` dependency scanning is out of scope
  (single-file hello-world).

### 3. Asset handler (runtime load)

Unchanged from its current active path: `LoadAssetData` deserializes
`AngelScriptData` (the source) into the asset. The handler stays pure
serialization with no engine coupling; module compilation lives in the system
component (below).

### 4. System component — engine, modules, context pool

Owns the single `asIScriptEngine` (created once in `Activate`, already fixed).

- **Context pool.** Add `RequestContext()` and `ReturnContext(asIScriptContext*)`
  to the `AngelScriptRequests` bus interface and `AngelScriptInterface` (they
  were referenced in commented component code but never declared). Implement a
  free-list on the system component: `AZStd::vector<asIScriptContext*>` guarded
  by a mutex. `RequestContext` pops an idle context or creates one via the
  engine; `ReturnContext` calls `Unprepare()` and pushes it back; shutdown
  releases all pooled contexts. (Gameplay execution is main-thread in practice;
  the mutex is cheap insurance and keeps the pool safe if a context is ever
  requested off-thread.)
- **Module compile.** `EnsureModule(moduleName, source)` — idempotent: if the
  named module already exists, return it; otherwise `CScriptBuilder`
  `StartNewModule` -> `AddSectionFromMemory(source)` -> `BuildModule`. On failure,
  log via `AZ_Error` and return `nullptr`.

### 5. Component (per-entity execution)

Implements the flow already sketched in comments in `AngelScriptComponent.cpp`,
now that the context pool exists.

- `OnAssetReady` -> `CreateScriptObject`:
  1. `EnsureModule` from the asset's source + module name.
  2. Find the class type by the convention *class name == module name == filename
     stem*.
  3. `RequestContext`, run the type's factory, capture the returned
     `asIScriptObject*`, `AddRef` it.
  4. Cache `OnCreate` / `OnDestroy` / `OnTick` method pointers by declaration.
  5. Call `OnCreate` if present. `ReturnContext`.
  6. Connect to `TickBus` iff `OnTick` exists.
- `OnTick(dt)`: `RequestContext` -> `Prepare(onTick)` -> `SetObject` ->
  `SetArgFloat(0, dt)` -> `Execute` -> `ReturnContext`.
- `DestroyScriptObject`: call `OnDestroy` if present, `Release` the object, clear
  cached pointers, disconnect `TickBus`.

### 6. Editor asset assignment (0b)

Uncomment the Inspector `DataElement` for `m_scriptAsset` in
`AngelScriptComponent::Reflect` so a `.as` asset can be assigned to the component
in the Entity Inspector.

## Error handling

- Builder syntax errors -> job issues / job failure with messages (message
  callback already routes to logging).
- Runtime compile failure, missing class, or missing factory -> `AZ_Error`; the
  component no-ops (no script object), no crash.
- Null engine / null context -> guarded early-returns (already in place).

## Testing

A `AngelScript.Tests` fixture creates an `asIScriptEngine` (or drives the system
component through the bus) and registers one native function the script calls to
record a side-effect (e.g. increment a counter). The test compiles an inline
class defining `OnCreate` / `OnTick`, drives it through the context pool +
`EnsureModule` + instantiation + method dispatch, and asserts the counter
advanced as expected. The test is hermetic — it exercises the execution core
without the asset pipeline or the Editor.

## Out of scope (YAGNI for this milestone)

- BehaviorContext binding walker (Phase 1).
- Bytecode caching / precompiled shipping path (later optimization).
- `#include` / multi-file dependency scanning.
- Hot-reload state preservation (CSerializer add-on).
- Per-script-class component descriptors — `ScanAndRegisterScriptComponents`
  stays as-is; 0b uses the single `AngelScriptComponent` with an
  Inspector-assigned asset.
- Script `Properties` table -> Inspector fields (Phase 2).
