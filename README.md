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
4. **Become the first admin.** Start the server, take the setup token from its log, join, press `O` and enter it. See [First start](#first-start).
5. **Open the menu in-game with `O`.** Add more admins and groups on the Permissions tab.

Client and server must run the same major and minor version (for example 0.7.x on both). Otherwise the client refuses to open the menu and says why in chat.

## Configuration

### Admins and permissions

Admin rights are permission groups stored in `<profiles>/ZomBerry/ZomBerryPermissions.json`. They can be edited in that file, or in-game on the **Permissions** tab.

#### First start

With no admins configured, the server prints a large **SETUP TOKEN** banner with a token like `ABCD-EFGH-JKLM` to its script log and to `<profiles>/ZomBerry/ZomBerry_<date>.log`. Join the server and press `O`. A dialog asks for the token, and the first player to enter it becomes a `superadmin`.
- **Attempts:** each player has 5 tries, and every attempt is logged.
- **After use:** the token stops working once it has been used.

#### Groups

Each admin belongs to one group, and each group has a level, permission patterns and spawn rules:

```json
{
    "Groups": [
        { "Name": "superadmin", "Level": 100, "Permissions": ["*"], "SpawnAllowed": ["*"] },
        { "Name": "moderator", "Level": 10,
          "Permissions": ["zomberry.players.list", "zomberry.map.*", "zomberry.functions.TPToTarget", "zomberry.functions.HealTarget"],
          "SpawnAllowed": [] }
    ],
    "Admins": [
        { "Id": "76561198000000000", "Name": "Somebody", "Group": "superadmin" }
    ]
}
```

- **`Id`:** the player's Steam64 ID. A BattlEye GUID works too.
- **`Name`:** only for display. ZomBerry updates it when the admin joins.

#### Levels

`Level` (0 to 100) decides who may change whom. An admin who may manage permissions can only change groups ranked **below** their own: their permissions, spawn rules, level, or deleting them. They can also only assign or remove admins who are in such groups, and only into such groups. Groups at their own level or above are read-only for them.

The `superadmin` group is always level 100, and level 100 may change everything, including itself. So a level 90 admin can manage moderators, but not fellow level 90 admins and not superadmins.

#### Permissions

| Permission | Allows |
|---|---|
| `zomberry.players.list` | The player list, with health and blood |
| `zomberry.players.kick` | Kicking players |
| `zomberry.players.ban` | Banning players, also offline ones by ID |
| `zomberry.players.unban` | The ban list, and unbanning |
| `zomberry.map.view` | The Map tab |
| `zomberry.map.player_markers` | Player markers on the map |
| `zomberry.map.teleport` | Teleporting yourself with `Alt` + click on the map |
| `zomberry.spawn.ground` | Spawning at the cursor or at your feet |
| `zomberry.spawn.inventory` | Spawning into your inventory |
| `zomberry.permissions.manage` | The Permissions tab |
| `zomberry.functions.<FunctionName>` | One function, for example `zomberry.functions.HealTarget`. Commands added by missions and mods get one too. |

How patterns match:
- **Wildcards:** `*` and `?` work anywhere. `zomberry.map.*` grants every map permission, and `*` alone grants everything.
- **Denying:** a leading `-` denies, and a deny always wins. `["*", "-zomberry.permissions.*"]` grants everything except managing permissions.
- **Opening the menu:** any member of a group can open it. The Functions tab only lists the functions that admin may run, and tabs without the matching permission are hidden. The server checks every request again, so a modified client gains nothing.

#### Spawn rules

`SpawnAllowed` decides which classes a group may spawn. A rule can be:
- **A class name** (`AKM`).
- **A base class** (`Weapon_Base`): every class inheriting from it matches.
- **A wildcard** (`Mag_*`).
- **A deny** (`-Mag_STANAG*`), which always wins.

The Spawn tab only lists the classes the admin may spawn. When rules hide anything, the end of the list says how many classes are allowed.

#### In-game editor

The **Permissions** tab needs `zomberry.permissions.manage`. While it's open:
- **The left panel** lists online players plus offline admins.
- **Assigning a group:** double-click an entry, then press the **Group** button to cycle through the groups or "none".
- **Editing a group:** pick it on the left. Double-click a permission to switch it between allowed `[x]`, denied `[-]` and unset `[ ]`; `(x)` means a wildcard already grants it.
  - **Your own patterns:** add them, such as `zomberry.functions.TP*`, with the field below the list.
  - **Spawn rules:** go in the bottom field, comma-separated. **Preview** swaps the permission list for the classes those rules allow, with a count. Press it again, or double-click the list, to go back.
- **Groups:** create or delete them with the buttons below the groups list. The level of the edited group is in the field next to "Groups". New groups start one level below your own, and groups you can't change are greyed out.
- **Saving:** press **Save** in the header. Nothing changes on the server before that. The server refuses changes that would leave no admin able to manage permissions.

#### Upgrading from admins.cfg

On the first start of 0.7.0, an existing `admins.cfg` (in `<profiles>/ZomBerry/` or the server root) is imported:
- **The first ID** becomes a `superadmin` (level 100).
- **All other IDs** go into an `admin` group (level 90). They have every permission, but can't change superadmins or their own group.

The file is then renamed to `admins.cfg.imported` and isn't used anymore. Earlier versions wrote one example ID into new `admins.cfg` files by themselves; that ID is skipped.

For a local test server only, `-zbryGiveAdminRightsToEveryone=true` gives every player `*`.

### Kicks and bans

Bans are ZomBerry's own list, stored in `<profiles>/ZomBerry/ZomBerryBans.json`. They don't touch BattlEye's `ban.txt`.
- **Who can be kicked or banned:** levels apply here too. Admins can only kick or ban admins ranked **below** them, and superadmins can kick or ban everyone. Nobody can kick or ban themselves. This also applies to offline bans of an admin's ID.
- **Reasons:** a kicked or banned player who is online gets the reason in chat, then is disconnected two seconds later. Their character is saved and logged out as on a normal disconnect.
- **Rejoining:** banned players are refused while they connect, before their character loads. The game can't show them a reason at that point, so they only see a disconnect. The server's ZomBerry log names them and the reason.
- **Durations:** 1 hour, 1 day, 7 days, 30 days or permanent. Expired bans are removed on their own.
- **The file:** entries have `Id` (Steam64), `Guid` (BattlEye GUID), `Name`, `Reason`, `BannedBy`, `Since` and `Until` (seconds since 1970, UTC; `0` means permanent). Either ID is enough for a ban to match. If the file can't be read, nobody is banned until it's fixed, and ZomBerry never overwrites it.

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
    ]
}
```

| Key | Meaning |
|---|---|
| `DebugLevel` | `0` normal, `1` adds frequent events (like the player list sync), `2` also dumps the whole function list and vehicle repair details |
| `UseScriptLog` | `0` writes ZomBerry's own log files to `<profiles>/ZomBerry/`, `1` writes to the game's script log instead |
| `SpawnMenuSorting` | Client side: the filter buttons of the Spawn tab, up to three. Each one lists the classes inheriting from any of its comma-separated `BaseClassNames` |

### Keybinds

Function keybinds are saved per client in `Documents/DayZ/ZomBerryKeybinds.bin`. By default `Delete` runs "Delete object near cursor". See [Functions](#functions) for how to change them.

## Using the menu

Open and close the menu with `O`, or close it with `Esc`. The `O` key can be changed in DayZ's control settings as **Open Admin menu**.

### Players

The left panel lists everyone online. Admins are green, and the item in a player's hands or their vehicle is shown next to their name.
- **Selecting a target:** double-click a player to make them the target of functions. The selected player turns orange, and their health and blood are shown below the list.
- **Refreshing:** the list updates itself every 5 seconds while the menu is open, the ban list every 30 seconds.
- **Kick and Ban:** select a player, then press **Kick** or **Ban**, enter a reason and, for bans, pick a duration. **Ban** with nobody selected bans an offline player by Steam64 ID or GUID.
- **Bans:** switches the list to the ban list, showing time left. Double-click a ban to see who banned them and why, then press **Unban** to lift it. **Players** switches back.

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

### Permissions

Admins with `zomberry.permissions.manage` get this tab, which edits groups, permissions and spawn rules. See [In-game editor](#in-game-editor).

## Troubleshooting

- **The menu doesn't open:**
  - Check that both client and server load ZomBerry, and that your ID is in a group in `ZomBerryPermissions.json`.
  - The server log shows "Auth respond to admin <name>" for admins. A chat message when you press `O` tells you about a version mismatch.
  - If `ZomBerryPermissions.json` can't be read (broken JSON), the server says so in its ZomBerry log and nobody is an admin until it's fixed. ZomBerry never overwrites a broken file.
- **A tab or function is missing:** the admin's group lacks the permission for it.
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

Every command gets the permission node `zomberry.functions.<functionName>` by itself, so groups can be given it in the Permissions tab.

### Permissions API

Mods and missions can use ZomBerry's admin groups for their own features, so server owners manage everything in one place, instead of keeping a separate admin list per mod. `GetZomberryPermAPI()` works like `GetZomberryCmdAPI()`, and the code is in [ZBerryPermAPI.c](ZomBerry/Addons/scripts/5_Mission/core/ZBerryPermAPI.c).

**Nodes** are dot-separated and lower case, with your mod's name first: `mymod.vip.teleport`.
- **Registering:** `RegisterNode` makes a node show up in the Permissions tab, where it can be given to groups like any built-in one.
- **Reserved prefix:** `zomberry.*` belongs to ZomBerry, so nodes starting with it are refused.
- **Granting:** groups grant nodes with the usual patterns. `mymod.*` grants every node of your mod, superadmins' `*` grants everything, and `-mymod.vip.*` denies.

```c
//Server, at startup (the Permissions tab gets its node list from the server)
GetZomberryPermAPI().RegisterNode("mymod.vip.teleport");
```

**Server side**, for requests from players (in singleplayer, the local player has every permission):

| Method | Returns |
|---|---|
| `bool HasPermission(PlayerIdentity identity, string node)` | Whether the player's group grants the node |
| `bool IsAdmin(PlayerIdentity identity)` | Whether the player is in any ZomBerry group |
| `int GetLevel(PlayerIdentity identity)` | Group level, 0 to 100 (100 is superadmin), or -1 for players who aren't admins |
| `string GetGroup(PlayerIdentity identity)` | Group name, or `""` |
| `bool CanActOn(PlayerIdentity actor, PlayerIdentity target)` | ZomBerry's rank rule, the one kicks and bans use: admins can only act on lower-ranked admins, and superadmins on everyone. Players who aren't admins and the actor themselves are always allowed |
| `bool CanSpawn(PlayerIdentity identity, string className)` | Whether the player's spawn rules allow the class |

```c
void MyModTeleportRequest(PlayerIdentity sender, vector position) {
	if (!GetZomberryPermAPI().HasPermission(sender, "mymod.vip.teleport")) return;
	...
}
```

**Client side**, to show or hide your own UI. The server sends each client its own permissions, and these answer from that copy. They're only for the UI: always check again on the server.

| Method | Returns |
|---|---|
| `bool HasLocalPermission(string node)` | Whether the local player's group grants the node |
| `bool IsLocalAdmin()` | Whether the local player is a ZomBerry admin |
| `ScriptInvoker OnLocalPermissionsChanged` | Called, without arguments, whenever the server sends the local player new rights (after joining, and when groups or permissions change) |

```c
void MyModInitUI() {
	GetZomberryPermAPI().OnLocalPermissionsChanged.Insert(MyModUpdateButtons);
	MyModUpdateButtons();
}

void MyModUpdateButtons() {
	m_TeleportButton.Show(GetZomberryPermAPI().HasLocalPermission("mymod.vip.teleport"));
}
```

The example mission registers `missionexample.vip_loadout` and gives members of a group with that permission a map on spawn.

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
| `5_Mission/core/` | Permissions (matching, groups, JSON, the Permissions API), server and client auth, request handlers, bans, spawning, version and build info |
| `5_Mission/ui/` | Admin menu components, one class per tab or panel |
| `5_Mission/layouts/` | Layouts of the menu and its components |
| `5_Mission/*.c` | Mission hooks, command API, stock functions, config, logger, keybinds |

## Building from source

Pack `ZomBerry/Addons/scripts` into `scripts.pbo` with the PBO prefix `ZomBerry\AT\scripts`, using DayZ Tools' FileBank or Addon Builder. Then sign it with your own key and ship the matching `.bikey`. The result goes into a mod folder laid out like `ZomBerry/`: `Addons/`, `Keys/` and `Config/`.

`5_Mission/core/ZBerryBuildInfo.c` describes the build. As committed, it describes a plain release build, and the admin menu header shows the version. Build tooling can fill in the commit hash and mark the build as a development build, so the header shows the commit instead and the logs show both.

## Planned features

- Keybinds for functions with parameters
- New functions in the default kit

Done in 0.7.0:
- A Permissions API, so other mods and missions can use ZomBerry's groups for their own features
- Kicks and bans from the players panel, with reasons, durations and offline bans by ID
- Permission groups with fine-grained, wildcard permissions and spawn rules, editable in JSON and in-game
- A setup token for the first admin, replacing installation mode and manual file editing
- No dependencies: ZomBerry's own RPC layer replaces Community Framework
- The admin menu was rebuilt from self-contained UI components, and the map updates live
- Server-side checks on every admin request, and more robust stock functions
- Inventory spawning loads magazines into the weapon in hands and explains when an item doesn't fit
- Function parameters can have named values (Partial heal shows No/Yes, Set date shows month names)

Earlier:
- The commands list is completely server-side, so it can be extended from missions and server-side mods
