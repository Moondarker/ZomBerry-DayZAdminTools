# ZomBerry Admin Tools

A lightweight in-game admin menu for DayZ: player management, a function list (heal, teleport, weather, time and more), an object spawner and a live map. The function list is defined on the server, so missions and server-side mods can add their own commands.

ZomBerry has **no dependencies**, so it doesn't need Community Framework or any other mod. It's still in an early testing stage, so please report any issues, and mention your game and ZomBerry versions in the issue.

- [Installation](#installation)
- [Configuration](#configuration)
- [Using the menu](#using-the-menu)
- [Troubleshooting](#troubleshooting)
- [For developers](#for-developers)
- [Building from source](#building-from-source)
- [Planned features](#planned-features)

## Installation

ZomBerry is a client **and** server mod. Admins need it loaded on their game, and the server needs it too. The [workshop page](https://steamcommunity.com/workshop/filedetails/discussion/1582756848/1745605598717631020) has the latest release.

1. **Load the mod on both sides.** Copy the mod folder to the server and to the client (or subscribe on the workshop), then add it to `-mod=` on both, for example `-mod=@ZomBerry`.
2. **Accept the signature on the server.** Copy `Keys/Vaker.bikey` into the server's `keys` folder.
3. **Start the server with a profile folder.** Use `-profiles=<folder>`; ZomBerry keeps its configuration and logs in `<profiles>/ZomBerry/`.
4. **Add your admins.** Put one ID per line in `<profiles>/ZomBerry/admins.cfg`, see [Admins](#admins).
5. **Open the menu in-game with `O`.**

Client and server must run the same major and minor version (for example 0.7.x on both). Otherwise the client refuses to open the menu and says why in chat.

## Configuration

### Admins

`admins.cfg` is a plain text file with one ID per line. Both formats work:
- **Steam64 ID**, for example `76561198000000000`.
- **BattlEye GUID**, the hashed ID shown in the server's logs.

The server looks for it in this order and uses the first one it finds:

1. `<profiles>/ZomBerry/admins.cfg` (recommended)
2. `admins.cfg` in the server root folder
3. `ZomBerry/Config/admins.cfg` inside the mod folder

If none exists on first start, ZomBerry creates one in `<profiles>/ZomBerry/`. Check its contents and replace them with your own IDs.

### Installation mode

If you can't get admin rights working, start the server once with `-zbryInstallMode=true`. Every connecting player then gets a reduced menu with guided steps:
- list the admins ZomBerry found,
- check where `admins.cfg` is,
- create it in the right place,
- add **your** ID to it.

The player list and spawning are disabled while installation mode is on. Remove the launch option afterwards.

For a local test server only, `-zbryGiveAdminRightsToEveryone=true` makes every player an admin.

### ZomBerryConfig.json

Created with default values on first start:
- **Server:** in `<profiles>/ZomBerry/`.
- **Clients:** in `Documents/DayZ/`.

```json
{
    "DebugLevel": 0,
    "UseScriptLog": 0,
    "SpawnMenuSorting": [
        { "CategoryName": "Items", "BaseClassNames": "Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base" },
        { "CategoryName": "Objects", "BaseClassNames": "Transport,House" },
        { "CategoryName": "AI", "BaseClassNames": "DZ_LightAI" }
    ],
    "PermissionGroups": []
}
```

| Key | Meaning |
|---|---|
| `DebugLevel` | `0` normal, `1` adds frequent events (like the player list sync), `2` also dumps the whole function list and vehicle repair details |
| `UseScriptLog` | `0` writes ZomBerry's own log files to `<profiles>/ZomBerry/`, `1` writes to the game's script log instead |
| `SpawnMenuSorting` | Client side: the filter buttons of the Spawn tab, up to three. Each one lists the classes inheriting from any of its comma-separated `BaseClassNames` |
| `PermissionGroups` | Reserved for the upcoming permissions system, currently unused |

### Keybinds

Function keybinds are saved per client in `Documents/DayZ/ZomBerryKeybinds.bin`. By default `Delete` runs "Delete object near cursor". See [Functions](#functions) for how to change them.

## Using the menu

Open and close the menu with `O`, or close it with `Esc`. The `O` key can be changed in DayZ's control settings as **Open Admin menu**.

### Players

The left panel lists everyone online. Admins are green, and the item in a player's hands or their vehicle is shown next to their name.
- **Selecting a target:** double-click a player to make them the target of functions. The selected player turns orange, and their health and blood are shown below the list.
- **Refreshing:** use the refresh button to update the list.

### Functions

- **Running a function:** click it in the list, set its parameters if it has any (slider or exact value), then press **Execute** or double-click the function.
  - Functions in the "OnTarget" and "==" categories act on the selected player. If no player is selected, some of them use you as the target and the rest ask you to select a player first.
  - "Delete object near cursor" asks for confirmation: run it a second time on the same object.
- **Binding a key:** select a function, press **Bind** (it shows `[...]`), then press the key. Functions with parameters can't be bound yet.
- **Removing a key:** select the function and press its key button twice. The first press shows `[...]` and the second removes the bind.
- **Removing all binds:** select any function without parameters, hold `A` and press the button twice.

The stock set:

| Category | Functions |
|---|---|
| OnTarget | Teleport target to cursor, teleport 10 m forward, teleport to target, teleport target to you, heal, partial heal (diseases, wounds, food and water), toggle god mode, repair item in hands, refuel and repair nearby vehicles |
| == | Kill, bite (bleeding), strip, induce vomiting, laughter or sneezing, set health, set blood |
| OnServer | FreeCam, time to day or night, set time, set date, set weather (fog, overcast, rain), delete object near cursor |

**FreeCam:** move with the usual movement keys and change the speed with the mouse wheel. Run the function again to return to your character.

### Spawn

- **Picking an object:** use the filter buttons from `SpawnMenuSorting`, or the search field, which matches class names and display names. Double-click a category header to fold it, and double-click an entry to spawn it.
- **Choosing where it goes:** the target button switches between three places:
  - **Cursor:** where you look.
  - **Ground:** at your feet.
  - **Inventory:** into your own inventory.
    - Weapon attachments go straight onto a carried weapon with a free matching slot.
    - A matching magazine is loaded into it when the weapon is empty.
    - If nothing fits, chat tells you whether there's no room or the object can't be carried at all.
- **Safe and Expert:** the header button switches between the two lists. Safe (green) hides classes without a proper display name, such as test and placeholder items. Expert (red) shows everything.

### Map

Players appear as markers, green for admins and blue for everyone else. They update every 2 seconds while the map is open.
- **Teleporting:** hold `Alt` and click a spot.
- **Zooming out:** double-click the map.

The cursor's coordinates are shown below the map.

## Troubleshooting

- **The menu doesn't open:**
  - Check that both client and server load ZomBerry, and that your ID is in `admins.cfg`.
  - The server log shows "Auth respond to admin <name>" for admins. A message in chat when you press `O` tells you about a version mismatch or a broken `admins.cfg`.
- **Logs:**
  - **Server:** `<profiles>/ZomBerry/ZomBerry_<date>.log`, and the server's script log.
  - **Client:** `ZomBerry/` in the game's profile folder (`%LocalAppData%/DayZ` by default).
- **Every admin action is logged on the server** with the admin's name and ID.

## For developers

### Adding commands

The function list lives on the server and is sent to admins when they open the menu. So missions and server-side mods can register their own commands without any client-side changes.

**AddCategory(string categoryName, int color)** adds a category. The color is ARGB, so `0xFF00FF00` is opaque green.
```c
GetZomberryCmdAPI().AddCategory("MyMod", COLOR_RED);
```

**AddCommand(string displayName, string functionName, Class instance, string categoryName, bool targetRequired = true, ZBerryFuncParamArray functionParams = NULL)** adds a command:
- `displayName`: shown in the function list.
- `functionName`: the method to call on `instance`.
- `instance`: the object that has the method. Keep it alive for as long as the command exists.
- `categoryName`: an existing category.
- `targetRequired`: if `true`, the admin must select a player first. If `false`, the admin is the target when nobody is selected.
- `functionParams`: up to 3 integer parameters, shown as sliders with an exact-value box. A parameter can name its values: the box then shows the name, and admins can type either a name or a number. With as many names as values, each value gets its own name. With fewer names, the range is split evenly between them. The function still receives the number.

```c
GetZomberryCmdAPI().AddCommand("Induce sneeze", "MyModSneezeTarget", this, "MyMod", true);

GetZomberryCmdAPI().AddCommand("Set time", "MyModSetTime", this, "MyMod", false, {
	new ZBerryFuncParam("Hour", {0, 23, 12,}),   //Name, {min, max, default}
	new ZBerryFuncParam("Minute", {0, 59, 0,}),
});

GetZomberryCmdAPI().AddCommand("Toggle lights", "MyModLights", this, "MyMod", false, {
	new ZBerryFuncParam("Lights", {0, 1, 1,}, {"Off", "On"}),   //Optional value names
});
```

The method is called on the server, so it must have this signature:
```c
void MyModSetTime( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
	//funcName: the function name, adminId/targetId: session ids (targetId == adminId when no player was selected),
	//cursor: where the admin's cursor hits the ground, fValues: always 3 values, unused ones are 0
	PlayerBase admin = ZBGetPlayerById(adminId);
	ZomberryStockFunctions.MessagePlayer(admin, "Time set to " + fValues[0] + ":" + fValues[1]);
}
```
Commands without parameters may leave out the `fValues` argument.

Helpers:
- **`ZBGetPlayerById(int sessionId)`** returns the `PlayerBase` of a session id, or `NULL` if that player is gone.
- **`ZomberryStockFunctions.MessagePlayer(PlayerBase player, string message)`** shows a `[ZomBerry]` chat message to one player.

Working examples are in [ZomBerryStockFnc.c](ZomBerry/Addons/scripts/5_Mission/ZomBerryStockFnc.c), with all stock commands, and [Examples/missionExample.chernarusplus/init.c](Examples/missionExample.chernarusplus/init.c), with commands added from a mission.

### Testing without a server

[Examples/offlineTest.chernarusplus](Examples/offlineTest.chernarusplus/init.c) is a minimal offline mission: a character on the map, no loot or infected. Copy it to the game's `Missions` folder and start `DayZ_x64.exe` directly (not through BattlEye):
```
DayZ_x64.exe "-mod=@ZomBerry" "-mission=.\Missions\offlineTest.chernarusplus" -window -noPause
```

### Project layout

| Path | Contents |
|---|---|
| `3_Game/ZBerryRPC.c` | ZomBerry's own RPC layer. All its traffic uses one engine RPC id, `74635`, and each message starts with an opcode from `ZBerryRPCOp` |
| `4_World/` | God mode on `PlayerBase`, and the free and spectator cameras |
| `5_Mission/core/` | Server side: auth and admin list, request handlers, installation mode, spawning, version and build info |
| `5_Mission/ui/` | Admin menu components, one class per tab or panel |
| `5_Mission/layouts/` | Layouts of the menu and its components |
| `5_Mission/*.c` | Mission hooks, command API, stock functions, config, logger, keybinds |

## Building from source

Pack `ZomBerry/Addons/scripts` into `scripts.pbo` with the PBO prefix `ZomBerry\AT\scripts`, using DayZ Tools' FileBank or Addon Builder. Then sign it with your own key and ship the matching `.bikey`. The result goes into a mod folder laid out like `ZomBerry/`: `Addons/`, `Keys/` and `Config/`.

`5_Mission/core/ZBerryBuildInfo.c` describes the build. As committed, it describes a plain release build, and the admin menu header shows the version. Build tooling can fill in the commit hash and mark the build as a development build, so the header shows the commit instead and the logs show both.

## Planned features

- A permissions system (per-admin access to functions)
- Keybinds for functions with parameters
- New functions in the default kit

Done in 0.7.0:
- No dependencies: ZomBerry's own RPC layer replaces Community Framework
- The admin menu was rebuilt from self-contained UI components, and the map updates live
- Server-side checks on every admin request, and more robust stock functions
- Inventory spawning loads magazines into the weapon in hands and explains when an item doesn't fit
- Function parameters can have named values (Partial heal shows No/Yes, Set date shows month names)

Earlier:
- The commands list is completely server-side, so it can be extended from missions and server-side mods
