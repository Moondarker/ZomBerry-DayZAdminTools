# AGENTS.md

Notes for AI coding agents and human contributors working on ZomBerry Admin Tools. The [README](README.md) covers what the mod does and how to use it. This file covers how to change it without breaking it.

## The project

ZomBerry is a DayZ admin tools mod written in **Enforce Script**, DayZ's C#-like scripting language. It runs on both sides:
- **Server:** checks every request and does the work.
- **Client:** shows the admin menu.

It has **no dependencies on other mods**, and that's deliberate: don't add Community Framework or similar.

All code lives in `ZomBerry/Addons/scripts`, packed into one PBO with the prefix `ZomBerry\AT\scripts`:

| Path | Contents |
|---|---|
| `config.cpp` | CfgPatches/CfgMods: script modules, inputs, the mod version |
| `Common/` | `#define ZOMBERRY_AT`, visible to every module. Other mods use it to check whether ZomBerry is loaded |
| `3_Game/ZBerryRPC.c` | Own RPC layer: one engine RPC ID (`74635`), every message starts with an opcode (`ZBerryRPCOp`) |
| `4_World/` | `PlayerBase` god mode (`ZBIsGod`/`ZBSetGod`, used by other mods), FreeCam and spectator cameras |
| `5_Mission/ZomBerry.c` | `ZomberryBase`, the `MissionServer`/`MissionGameplay` hooks, menu and dialog opening |
| `5_Mission/core/` | Server auth and permissions (`ZBerryAuth`, `ZBerryPermissions`), client auth (`ZBerryClientAuth`), request handlers (`ZBerryServerRPC`), bans, spawning, version and build info, the Permissions API |
| `5_Mission/ui/` | Admin menu components: one class per tab, panel or dialog |
| `5_Mission/layouts/` | `.layout` files of the menu and its components |
| `5_Mission/*.c` | Command API (`GetZomberryCmdAPI`), stock functions, config, logger, keybinds, helpers |

`Examples/` has a mission that adds commands and a permission node, and a minimal offline mission for testing without a server.

## Ground rules

- **The server decides, the client displays.**
  - Every request handler on the server checks the sender's permission (`ZBerryServerRPC.CanRun`, `ZBerryAuth.HasPermission`).
  - Client-side permission checks (`GetZBerryClientAuth().Has`) only hide UI.
  - Never trust IDs sent by the client: the admin is always the RPC sender.
- **Only ever append RPC opcodes.**
  - Client and server must agree on `ZBerryRPCOp`, so never renumber or reuse an opcode.
  - Changing the params of an existing opcode breaks compatibility: client and server must share major.minor (`ZBerryVersion.IsCompatible`), so that needs a minor version bump.
  - Retired opcodes are listed above the enum, and their numbers are never reused.
- **World state goes through the streams, not new request/response pairs.**
  - **Server side:** `ZBerryStreamServer` (`5_Mission/core/ZBerryStreamServer.c`) keeps what each admin's client subscribed to. It pushes from one timer, checks permissions on every send, and only sends what changed: roster on change, player state as deltas, marker categories by signature.
  - **Client side:** `ZBerryStreamClient` (`GetZBerryStreams()`) keeps the latest data and raises `OnPlayersChanged`/`OnMarkersChanged`. The UI says what it shows (`SetListShown`, `SetMapShown`, `SetMarkerMask`, `SetSelected`).
  - **Extending:** new synced data (ESP, spectating, other mods) extends these classes instead of adding another polling RPC, so bandwidth stays low.
  - **Entities to track:** register them in `ZBerryEntityRegistry` (`4_World`) from their `EEInit`/`EEDelete`. It works on both sides: on a client it holds what the network bubble holds.
- **Admins act only on lower ranks** for kicks and bans (`ZBerryAuth.CanActOnLevel`, the Permissions API's `CanActOn`).
  - Functions use `ZBerryAuth.CanTarget` instead: groups shield their members with `zomberry.protect.*`, and superadmins ignore that. Mark commands that hurt or move their target as `harmful` in `AddCommand`.
- **New permission nodes** go into `ZBerryPermissions` as a constant, into `GetFixedNodes()` (so the Permissions tab lists them) and into the README's permission table. Nodes are lower case and dot-separated, and `zomberry.*` is ZomBerry's own.
- **Never overwrite a config file that failed to load.**
  - Load JSON with `JsonFileLoader<T>.LoadFile(path, data, error)` and log `error`.
  - On failure, keep running with safe defaults, but don't save over the file.
  - Don't use the deprecated `JsonLoadFile`/`JsonSaveFile`: they give no reason.
- **Keep public APIs stable.** Other mods and missions use:
  - `GetZomberryCmdAPI()`, `GetZomberryPermAPI()`, `ZBGetPlayerById()` and `ZomberryStockFunctions.MessagePlayer()`;
  - `#define ZOMBERRY_AT` with `PlayerBase.ZBIsGod()` and `ZBSetGod()`, so mods that switch damage themselves (safe zones) can leave god mode alone. God mode has to stay a server-side flag on `PlayerBase` that goes together with `SetAllowDamage`.
- **Log through `ZomberryBase.Log(module, text)`**, with a `ZomBerry<Area>` module name and an `INFO:`, `WARN:` or `ERROR:` prefix. Log every admin action on the server with the admin's name and ID. Chatty events go through `ZomberryBase.DebugLog(level, ...)`.

## Enforce Script pitfalls

These cost real debugging time on this project:
- **`func` is a reserved word.** Don't use it as a name.
- **Temporaries passed to weak references get freed.** A `new` array passed straight into a `Param` constructor or a method argument may be gone by the time it's used. Hold it in a local first (see `ZBerryServerRPC.RunFunction`).
- **Mixed arithmetic isn't plain integer maths.** Expressions mixing array elements, method results and literals can give unexpected results. Use typed `float` locals and `Math.Floor`/`Math.Ceil` (see `ZBerryFuncParam.GetValueName`). Plain `int / int` is integer division, so write `100.0` when you want a fraction.
- **Strings:** `string.Trim()` returns a copy, but `ToLower()`, `ToUpper()` and `Replace()` change the string in place.
- **You can't call a method on `new X()` in the same expression.** Assign it first.
- **A `modded class` must live in the same script module (`3_Game`/`4_World`/`5_Mission`) as the class it mods.** Fields of a modded class should be declared in the file that uses them: the order modded classes in different files are compiled in isn't something to rely on.
- **Load layouts by their full literal path** (`"ZomBerry/AT/scripts/5_Mission/layouts/X.layout"`). With the path built at runtime, the engine once returned the wrong layout tree.
- **Use typed locals for `out` parameters**, and initialise them, as in `ZBerryTime`.

## DayZ UI rules

The menu has to look right from 800x600 to 4K. The engine scales **exact** layout sizes and positions by screen height ÷ 1080, but not everything follows:
- **Text:** use `TextWidget`s with `"exact text" 1` and `"exact text size" N`. These scale.
- **Buttons:** button text never scales. Leave the button's own text empty, and put a child `TextWidget` inside it with `ignorepointer 1`, centred, exact size. Set captions with `ZBerryUIUtils.SetButtonText`/`SetButtonTextColor`.
- **Edit boxes:** use `font "gui/fonts/sdf_MetronLight24"` with `"exact text" 0`. The text then follows the box height.
- **Text listboxes:** their text never scales, and scripts can't change their font. Each listbox has hidden copies in other font sizes (`Name_F12`, `_F16`, `_F22`, `_F28`), and `ZBerryScaledList` shows the one closest to the current scale. New listboxes need the same copies.
- **Scripted layout:** `Widget.SetPos`/`SetSize` take **physical pixels**. Convert layout units with the width of a known exact-size parent (see `ZBerryUIPlayersPanel.LayoutRows`, `ZBerryUIPermsTab.LayoutRows`). Layouts can't pin a widget to both the top and bottom of its parent, so lay such rows out from script.
- **Components:** each one extends `ZBerryUIComponent` (tabs extend `ZBerryUITab`). It loads its own layout and routes its widgets' events to itself with `BindWidget`, which calls `SetHandler`.
- **Closing the menu:** while a component needs keyboard input, it must stop the menu key from closing the menu (`BlocksClose`, `ZomberryMenu.GetCloseClearance`).

## Testing

There's no unit test runner, since Enforce Script only runs inside the game or a server. Test in this order:
1. **Compile check:** start a DayZ server with `-mod=` pointing at the packed mod, and read its script log (`script_*.log` in the profile folder).
   - Any `SCRIPT (E)` line is a failure.
   - Check that ZomBerry logs "Starting Server side".
   - Client-only code compiles on the server too, as `5_Mission` is shared.
2. **Server-side tests:** a throwaway test mod that's never shipped. It holds a `modded class MissionServer` whose `OnInit` schedules a test method (`CallLater`). That method checks results and prints `PASS`/`FAIL` lines into the script log. This works well for pure logic: permission matching, time maths, JSON save/load, version parsing.
3. **Singleplayer:** `Examples/offlineTest.chernarusplus`, started directly without BattlEye. In singleplayer the local player has every permission.
4. **In-game on a server:** anything involving the UI, other players (kicks, bans, targets) or scaling at several resolutions.

Say which of these a change has been through. Don't claim in-game behaviour that wasn't tested in-game.

## Building and releasing

- **Build:** pack `ZomBerry/Addons/scripts` with the prefix `ZomBerry\AT\scripts` (DayZ Tools' FileBank or Addon Builder), then sign the PBO.
- **Signing keys:** never commit private keys (`.biprivatekey`) or local tooling. `Tools/` is git-ignored for that reason.
- **Build info:** `5_Mission/core/ZBerryBuildInfo.c` is committed as a plain release build. Build tooling may stamp a commit hash into a *copy* of it for dev builds. Never commit a stamped version.
- **The version lives in two places:** `g_zbryVer` in `5_Mission/ZomBerry.c` and `version` in `config.cpp`. Bump both, and add a section to [CHANGELOG.md](CHANGELOG.md).

## Git workflow

- **Branches:**
  - `development` is the integration branch;
  - `master` gets releases;
  - work happens on topic branches (`feature/...`, `docs/...`, bugfix branches), merged into `development` by fast-forward.
- **One commit per logical change.**
  - Commit messages have a short imperative subject.
  - The body explains *why* and what the user-visible effect is.
- **Behaviour changes** also update the README, and the changelog when they matter to server owners or admins.
- **Line endings:** keep each file's existing line endings. The repository stores LF, and working copies may use CRLF.
- **Code style:**
  - follow the surrounding code: tabs, braces on the same line;
  - `//Comments` without a space after the slashes, explaining *why*;
  - `m_` prefixes for members;
  - new classes prefixed with `ZBerry`.
