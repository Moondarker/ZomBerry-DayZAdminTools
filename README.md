# ZomBerry Admin Tools

A lightweight in-game admin menu for DayZ: player management, a function list (heal, teleport, weather, time and more), an object spawner and a live map. The function list is defined on the server, so missions and server-side mods can add their own commands.

ZomBerry has **no dependencies**, so it doesn't need Community Framework or any other mod. It's still in an early testing stage, so please report any issues, and mention your game and ZomBerry versions in the issue.

- [Installation](#installation)
- [Configuration](#configuration)
- [Using the menu](#using-the-menu)
- [Troubleshooting](#troubleshooting)
- [For developers](#for-developers)
- [Building from source](#building-from-source)
- [Roadmap](#roadmap)

What changed in each version is in the [changelog](CHANGELOG.md).

## Installation

ZomBerry is a client **and** server mod. Admins need it loaded on their game, and the server needs it too. The [workshop page](https://steamcommunity.com/workshop/filedetails/discussion/1582756848/1745605598717631020) has the latest release.

1. **Load the mod on both sides.** Copy the mod folder to the server and to the client (or subscribe on the workshop), then add it to `-mod=` on both, for example `-mod=@ZomBerry`.
2. **Accept the signature on the server.** Copy `Keys/Vaker.bikey` into the server's `keys` folder.
3. **Start the server with a profile folder.** Use `-profiles=<folder>`; ZomBerry keeps its configuration and logs in `<profiles>/ZomBerry/`.
4. **Become the first admin.** Start the server, take the setup token from its log, join, press `O` and enter it. See [First start](#first-start).
5. **Open the menu in-game with `O`.** Add more admins and groups on the Permissions tab, under the gear icon in the header.

Client and server must run the same major and minor version (for example 0.7.x on both). Otherwise the client refuses to open the menu and says why in chat.

**Upgrading from 0.6.x:**
- Update both sides: a 0.6.x client gets no answer from a 0.7 server, and the other way round.
- Community Framework is no longer needed.
- Your `admins.cfg` is imported on the first start (see [Upgrading from admins.cfg](#upgrading-from-adminscfg)).
- The [changelog](CHANGELOG.md#upgrading-from-06x) lists everything else.

## Configuration

### Admins and permissions

Admin rights are permission groups stored in `<profiles>/ZomBerry/ZomBerryPermissions.json`. They can be edited in that file, or in-game on the **Permissions** tab.

#### First start

With no admins configured, the server prints a large **SETUP TOKEN** banner with a token like `ABCD-EFGH-JKLM` to its script log and to `<profiles>/ZomBerry/ZomBerry_<date>.log`. Join the server and press `O`. A dialog asks for the token, and the first player to enter it becomes a `superadmin`.
- **Attempts:** each player has 5 tries, and every attempt is logged.
- **After use:** the token stops working once it has been used.

#### Groups

Each admin belongs to one group, and each group has a level, permission patterns and spawn rules. A new file starts with these two groups:

```json
{
    "Groups": [
        { "Name": "superadmin", "Level": 100, "Permissions": ["*"], "SpawnAllowed": ["*"] },
        { "Name": "moderator", "Level": 10,
          "Permissions": ["zomberry.players.list", "zomberry.players.kick", "zomberry.map.*",
                          "zomberry.functions.TPToTarget", "zomberry.functions.TPToAdmin", "zomberry.functions.HealTarget"],
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

#### Protecting admins from functions

By default, any admin may run any function they have on anyone, including higher-ranked admins. A group can shield its members with these permissions:
- **`zomberry.protect.lower`:** lower-ranked admins can't run any function on them.
- **`zomberry.protect.lower.harm`:** lower-ranked admins can't run *harmful* functions on them, but can still heal or repair them.
- **`zomberry.protect.same`** and **`zomberry.protect.same.harm`:** the same, against admins of the same level.

Harmful functions are the ones that hurt, move or work against the target:
- teleport target to cursor, and teleport target to you;
- toggle god mode;
- kill, bite and strip;
- vomiting, laughter and sneezing;
- set health and set blood.

Commands added by mods decide for themselves (see [Adding commands](#adding-commands)).
- **Superadmins ignore protections:** level 100 can always act on everyone.
- **Groups with `*`** (superadmin, and the `admin` group from an imported `admins.cfg`) are protected automatically.
- **Running a function on yourself** is always allowed.
- **Kicks and bans** keep their own stricter rule, described below.

#### Permissions

| Permission | Allows |
|---|---|
| `zomberry.players.list` | The player list, and the selected player's health, blood, ping, time online, status and Steam64 ID |
| `zomberry.players.kick` | Kicking players |
| `zomberry.players.ban` | Banning players, also offline ones by ID |
| `zomberry.players.unban` | The ban list, and unbanning |
| `zomberry.messages.private` | Private messages to the selected player |
| `zomberry.messages.announce` | Announcements to everyone online |
| `zomberry.map.view` | The Map tab |
| `zomberry.map.player_markers` | Player markers on the map |
| `zomberry.map.teleport` | Teleporting yourself with `Alt` + click on the map, to typed coordinates or to a saved location |
| `zomberry.map.teleport_players` | Teleporting the selected player with `Ctrl` + click on the map, to typed coordinates or to a saved location. Follows the protection nodes |
| `zomberry.map.locations.manage` | Saving and deleting locations on the Map tab |
| `zomberry.map.markers.<category>` | Object markers on the map: `vehicles`, `tents`, `storage`, `bases` or `bodies`. `zomberry.map.markers.*` grants all of them |
| `zomberry.spawn.ground` | Spawning at the cursor or at your feet |
| `zomberry.spawn.inventory` | Spawning into your inventory |
| `zomberry.spawn.player` | Spawning for the selected player: at their feet or into their inventory (together with `zomberry.spawn.ground` or `.inventory`). Follows the protection nodes |
| `zomberry.permissions.manage` | The Permissions tab (under the gear icon) |
| `zomberry.logs.view` | The Logs tab: the in-game admin and kill log |
| `zomberry.protect.lower`, `.lower.harm`, `.same`, `.same.harm` | Protect the group's members from functions run by lower-ranked or same-level admins, see [Protecting admins from functions](#protecting-admins-from-functions) |
| `zomberry.functions.<FunctionName>` | One function, for example `zomberry.functions.HealTarget`. Commands added by missions and mods get one too. |

Other mods and missions can add their own nodes through the [Permissions API](#permissions-api). These are listed after ZomBerry's own in the Permissions tab.

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
- **Unconscious or restrained players:** kicking them kills their character. The normal logout treats it like combat logging. See [#53](https://github.com/Moondarker/ZomBerry-DayZAdminTools/issues/53).
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
| `SpawnMenuSorting` | Client side: the filter buttons of the Spawn tab, up to three. Each one lists the classes inheriting from any of its comma-separated `BaseClassNames`. Without it, the defaults above are used |

If the file can't be read (broken JSON), the log says why, and ZomBerry runs with the default settings until it's fixed.

### Keybinds

Function keybinds are saved per client in `Documents/DayZ/ZomBerryKeybinds.bin`. By default `Delete` runs "Delete object near cursor". See [Functions](#functions) for how to change them.

Keybinds only work while no menu is open, so they don't fire while you type in chat or have the inventory open.

## Using the menu

Open and close the menu with `O`, or close it with `Esc` or the **×** in the header's top-right corner. The `O` key can be changed in DayZ's control settings as **Open Admin menu**.

- **Closing with the mouse:** the **×** always works, even while a search or text field would stop `O` from closing the menu.
- **The gear icon** next to it opens pages that don't need a tab of their own: Permissions and ESP. Only the pages you may use are listed. The dropdown closes by itself shortly after the mouse leaves it. The gear turns green while one of its pages is open, and it's hidden when you may not use any of them.

### Players

The left panel lists everyone online. Admins are green, and the item in a player's hands or their vehicle is shown next to their name.
- **Selecting a target:** double-click a player to make them the target of functions. The selected player turns orange, and below the list you see:
  - their health, blood and ping;
  - their position (only with `zomberry.map.player_markers`) and how long they've been online;
  - whether they're alive, unconscious, restrained or dead, and the vehicle they're in;
  - their Steam64 ID. **Copy ID** puts it on the clipboard.

  These details refresh every 5 seconds, and only for the selected player.
- **Refreshing:** the list updates itself every 5 seconds while the menu is open, the ban list every 30 seconds.
- **Kick and Ban:** select a player, then press **Kick** or **Ban**, enter a reason and, for bans, pick a duration. **Ban** with nobody selected bans an offline player by Steam64 ID or GUID.
- **Message:** sends a private message to the selected player, or an announcement to everyone. The **To** button switches between the two (with nobody selected, it's an announcement). Players see it as a notification at the side of the screen for 15 seconds, and in chat. Messages are up to 300 characters, and every one is logged with its text.
- **Bans:** switches the list to the ban list, showing time left. Double-click a ban to see who banned them and why, then press **Unban** to lift it. **Players** switches back.

### Functions

- **Running a function:** click it in the list, set its parameters if it has any (slider or exact value), then press **Execute** or double-click the function.
  - Functions in the "OnTarget" and "==" categories act on the selected player. If no player is selected, some of them use you as the target and the rest ask you to select a player first.
  - "Delete object near cursor" deletes what the cursor is on: items, vehicles, bodies, base parts, infected or animals. It never deletes living players or objects that belong to the map. Run it a second time on the same object to confirm.
  - "Clean up around cursor" removes one kind of thing within a radius (5–200 m): infected, dead bodies, loose items, or ruined items.
    - Bases, tents, storage, stashes and fireplaces are never touched.
    - The first run says how many it found, and running it again within 10 seconds deletes them.
  - "Flip vehicle near cursor" puts the car or boat under the cursor back on its wheels, keeping its heading. It refuses while someone is inside.
- **Binding a key:** select a function, press **Bind** (it shows `[...]`), then press the key. Functions with parameters can't be bound yet.
- **Removing a key:** select the function and press its key button twice. The first press shows `[...]` and the second removes the bind.
- **Removing all binds:** select any function without parameters, hold `A` and press the button twice.

The stock set:

| Category | Functions |
|---|---|
| OnTarget | Teleport target to cursor, teleport 10 m forward, teleport to target, teleport target to you, heal, partial heal (diseases, wounds, food and water), toggle god mode, toggle invisibility, spectate target, toggle freeze, repair item in hands, refuel and repair nearby vehicles |
| == | Kill, bite (bleeding), strip, induce vomiting, laughter or sneezing, set health, set blood |
| OnServer | FreeCam, time to day or night, set time, set date, set weather (fog, overcast, rain), delete object near cursor, clean up around cursor, flip vehicle near cursor, teleport all players to you, ESP |

**God mode:** the target takes no damage and their stamina stays full until the function is run again. Toggling it on also heals them fully. Mods that switch damage back on by themselves, such as safe zones, can end it early unless they check ZomBerry's god mode (see [God mode and other mods](#god-mode-and-other-mods)).

**Toggle invisibility:** other players don't see the target or their gear and don't hear them, apart from voice chat. Infected and animals ignore them, and nothing can hurt them until it's switched off again. It's separate from god mode. Sounds of other objects (doors, gunshots, vehicles) can still be heard, and players still bump into them. Logging out or respawning doesn't end it.

**Teleport all players to you:** run it twice within 10 seconds to confirm. Players are placed in a ring around you. Players in vehicles, and admins protected from your rank, stay where they are.

**Toggle freeze:** the selected player can't move, turn or raise their hands until you run it again. They get a chat message. Logging out doesn't escape it: a frozen player who rejoins is frozen again. It's harmful, so the protection nodes apply.

**Spectate target:** see through the selected player's eyes.
- **Camera:** it follows where their own camera looks, up and down and free look included. While they play in third person, the camera moves over their shoulder.
- **Distance:** it works anywhere on the map. What your game loads moves with the camera, so you see the people and vehicles around them, and the camera follows them when they teleport or drive off.
- **Your body** stays frozen where you left it. If you disconnect while spectating, it logs out like after a normal disconnect.
- **Stopping:** run the function again, or it ends by itself when they die or leave, or when you lose the permission.
- **Can't be combined:** you can't use it together with FreeCam.

**ESP:** draws players, vehicles, infected and animals through walls on your own screen. Running it switches ESP on, and running it again switches it off. It has no parameters, so it can be bound to a key. What it draws is set on the [ESP tab](#esp).
- **Players:** a box with name, distance, health and blood, and a skeleton within 150 m. Players beyond what your game loads (about 1 km) show as a name and distance.
- **Vehicles:** a box with the name, within 1.5 km.
- **Infected and animals:** a box, within 300 m.
- **Who sees what:** only you see it, and it stays on when the menu is closed. It switches off by itself if you lose the permission.

**FreeCam:** move with the usual movement keys and change the speed with the mouse wheel. Run the function again to return to your character. What your game loads moves with the camera, so you see players and vehicles wherever you fly. Your body stays frozen where you left it.

### Spawn

- **Picking an object:** use the filter buttons from `SpawnMenuSorting`, or the search field, which matches class names and display names. Double-click a category header to fold it, and double-click an entry to spawn it.
- **Choosing where it goes:** hover over the target button to open the list of places above it, then click one, or click the button itself to step through them. The list stays open until the mouse leaves it, and only shows the places you're allowed to use:
  - **Cursor:** where you look.
  - **Ground:** at your feet.
  - **Inventory:** into your own inventory.
    - Weapon attachments go straight onto a carried weapon with a free matching slot.
    - A matching magazine is loaded into it when the weapon is empty.
    - If nothing fits, chat tells you whether there's no room or the object can't be carried at all.
  - **Player's feet** and **Player's inv.:** the same for the player you double-clicked in the list. They get a chat message when something lands in their inventory.
- **Safe and Expert:** the header button switches between the two lists. Safe (green) hides classes without a proper display name, such as test and placeholder items. Expert (red) shows everything.

### Map

With `zomberry.map.player_markers`, players appear as markers, green for admins and blue for everyone else. A small arrow in front of each marker shows which way the player is facing. They update every 2 seconds while the map is open.

**Filters**, top left, opens a list of marker types when you point at it, and closes shortly after the mouse leaves. Each type only appears with its permission.
- **Players:** on by default.
- **Vehicles:** cars and boats, with their names.
- **Tents:** pitched tents and shelters.
- **Storage:** barrels, sea chests, wooden crates and buried stashes.
- **Bases:** fences, watchtowers and flagpoles.
- **Bodies:** dead players, with their names.
- **Locations:** saved locations, on by default.

Storage and base parts are grouped: one marker per 50 m square, with a count, such as "Base (23)". Objects in someone's hands or inventory aren't shown. Object markers refresh every 10 seconds, and one map shows up to 1,000 of them. If there are more, the hint below the map says how many were left out.
- **Teleporting:** hold `Alt` and click a spot.
- **Teleporting a player:** select them in the list, then hold `Ctrl` and click a spot.
- **Zooming out:** double-click the map.

The cursor's coordinates are shown below the map.

**Coordinates**, top centre: type X and Y as the map shows them, then press **Go** to teleport yourself there, or **Player** for the selected player. The height comes from the ground.

**Locations**, top right, opens the saved locations: places all admins share, such as a trader or an admin area.
- **Click a name** to show it on the map.
- **Go** teleports you there, **Player** teleports the selected player. You arrive exactly where the location was saved, height included, so a spot inside a building works.
- **Save here** (with `zomberry.map.locations.manage`) saves your current position under the name you typed. **X** deletes a location: click it twice.
- The list is kept in `ZomBerryLocations.json` in the server's `ZomBerry` profile folder, up to 100 locations with names of up to 40 characters.

### Logs

Admins with `zomberry.logs.view` get this tab. It shows what the server logged, newest first: functions run, spawns, teleports, messages, saved locations, kicks, bans, permission changes, refused attempts, logins, respawns and logouts, kills, damage between players, and server starts and stops. Times are in your local time.
- **Kept across restarts:** the server keeps the last 2048 events and 10486 damage records in `ZomBerryEventLog.bin` in its `ZomBerry` profile folder, with the newest additions in `ZomBerryEventLog.bin.journal`.
  - Each start logs "Server started", with how much it restored, and whether the previous run stopped without a clean shutdown. A clean shutdown logs "Server stopping".
  - Even after a crash, everything logged up to it is kept.
  - A file that can't be read is set aside as `.bad`, and the log starts empty.
  - Delete both files, with the server stopped, to clear the log.
- **Deaths** say what killed the player: another player, an infected, an animal, a vehicle, an explosion, a fall, drowning, fire, bleeding, poisoning, thirst or hunger. The damage log only covers damage between players.
- **Language:** names from the game (animals, vehicles, weapons, objects) show in your game's language. The server's log file has them in the server's language.
- **The left panel** lists everyone the log mentions, online or not. Double-click one to see only their events, or press **Show all** to go back.
- **Search** matches the text, names and Steam64 ids. **Types** filters by kind of event.
- **Live and Paused:** the header button stops the list from updating while you read it.
- **Details:** select a row to see it in full below the list, wrapped over up to three lines.

### ESP

Admins who may run the ESP function open this tab from the gear icon in the header. It sets what ESP draws on your own screen. The settings are saved on your computer, in `ZomBerryESP.json` next to `ZomBerryConfig.json`.
- **Players:** box, skeleton (within 150 m), name, distance and health, each on its own. **Far players** shows players outside your network range as labels, with name and distance.
- **Vehicles, infected and animals:** each on its own.
- **Range:** how far players and vehicles are drawn, and separately infected and animals (50–3000 m).
- **The header button** shows whether ESP is on, and switches it like the ESP function does.

Changes apply right away while ESP is on. The server only sends everyone's position and health while your settings use them (far players or health).

### Permissions

Admins with `zomberry.permissions.manage` open this tab from the gear icon in the header. It edits groups, permissions and spawn rules. See [In-game editor](#in-game-editor).

## Troubleshooting

- **The menu doesn't open:**
  - Check that both client and server load ZomBerry, and that your ID is in a group in `ZomBerryPermissions.json`.
  - The server log shows "Auth respond to admin <name>" for admins. A chat message when you press `O` tells you about a version mismatch.
  - A 0.6.x client on a 0.7 server (or the other way round) gets no answer at all, so update both sides.
  - If `ZomBerryPermissions.json` can't be read (broken JSON), the server says so in its ZomBerry log and nobody is an admin until it's fixed. ZomBerry never overwrites a broken file.
- **A tab or function is missing:** the admin's group lacks the permission for it.
- **A keybind does nothing:** keybinds don't run while chat, the inventory or any other menu is open.
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

**AddCommand(string displayName, string functionName, Class instance, string categoryName, bool targetRequired = true, ZBerryFuncParamArray functionParams = NULL, bool harmful = false)** adds a command:
- `displayName`: shown in the function list.
- `functionName`: the method to call on `instance`.
- `instance`: the object that has the method. Keep it alive for as long as the command exists.
- `categoryName`: an existing category.
- `targetRequired`: if `true`, the admin must select a player first. If `false`, the admin is the target when nobody is selected.
- `functionParams`: up to 3 integer parameters, shown as sliders with an exact-value box. A parameter can name its values: the box then shows the name, and admins can type either a name or a number. With as many names as values, each value gets its own name. With fewer names, the range is split evenly between them. The function still receives the number.
- `harmful`: set it to `true` if the command hurts, moves or works against its target. Groups with `zomberry.protect.lower.harm` or `zomberry.protect.same.harm` are then shielded from it.

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
- **`GetZomberryCmdAPI().RefuseCall(string reason)`**: call it from your command when it does nothing (no valid target, nothing to act on). The admin log then files the call as refused, with the reason, instead of as run. Tell the admin yourself with `MessagePlayer`.
- **`GetZomberryCmdAPI().AwaitConfirmation()`**: call it when this run only asked the admin to run the command again to confirm. That run isn't logged; the confirmed one is.
- **`GetZomberryCmdAPI().DescribeCall(string text)`**: what this run did, logged instead of "AdminName ran My command". Write `{actor}` for the admin and `{target}` for the selected player, e.g. `"{actor} gave {target} a medkit"` or `"{actor} switched night vision on"`. The log view highlights both names.
- **`GetZomberryCmdAPI().GetCurrentCall()`**: the running call (`ZBerryCall`): `GetFunction()`, `GetAdmin()` and `GetTarget()` (identities, the target is `NULL` when the admin selected themselves).

The three helpers above only work while your command runs. Called later, from a `CallLater` callback, they write a warning to the log and change nothing.

```c
void MyModHeal( string funcName, int adminId, int targetId, vector cursor ) {
	PlayerBase admin = ZBGetPlayerById(adminId);
	PlayerBase target = ZBGetPlayerById(targetId);
	if (!target || !target.IsAlive()) {
		ZomberryStockFunctions.MessagePlayer(admin, "Select a living player");
		GetZomberryCmdAPI().RefuseCall("Select a living player");
		return;
	}
	//...
}
```

Working examples are in [ZomBerryStockFnc.c](ZomBerry/Addons/scripts/5_Mission/ZomBerryStockFnc.c), with all stock commands, and [Examples/missionExample.chernarusplus/init.c](Examples/missionExample.chernarusplus/init.c), with commands added from a mission.

Every command gets the permission node `zomberry.functions.<functionName>` by itself, so groups can be given it in the Permissions tab.

### Client-side functions

Commands run on the server. When a command needs something on the admin's own game, such as an overlay, a camera or a sound, it can switch a **client feature** on that client. ZomBerry's ESP and spectating work this way.
- **Server:** the command (after ZomBerry checked its permission) calls `SetClientFeature`.
- **Client:** whatever was registered under the same name runs, with the arguments.

| Method | Side | Does |
|---|---|---|
| `SetClientFeature(PlayerIdentity identity, string feature, bool enabled, TIntArray args = NULL, string requiredNode = "")` | Server | Switches the feature on that player's client. With `requiredNode`, it's switched off by itself when the player loses that permission |
| `bool IsClientFeatureActive(PlayerIdentity identity, string feature)` | Server | Whether it's on for that player |
| `RegisterClientFeature(string feature, Class instance, string funcName)` | Client | What runs when the server switches it: `void funcName(bool enabled, TIntArray args)` |
| `bool IsClientFeatureEnabled(string feature)` | Client | Whether the server switched it on for this client |

```c
//Both sides load this mod. Server: a command that switches an overlay on the admin's client
void MyModOverlayCmd( string funcName, int adminId, int targetId, vector cursor ) {
	PlayerBase admin = ZBGetPlayerById(adminId);
	if (!admin) return;

	bool on = !GetZomberryCmdAPI().IsClientFeatureActive(admin.GetIdentity(), "mymod_overlay");
	GetZomberryCmdAPI().SetClientFeature(admin.GetIdentity(), "mymod_overlay", on, NULL, ZBerryPermissions.FunctionNode(funcName));
}

//Client, at startup
GetZomberryCmdAPI().RegisterClientFeature("mymod_overlay", this, "MyModOnOverlay");

void MyModOnOverlay(bool enabled, TIntArray args) {
	m_Overlay.Show(enabled);
}
```

The server only decides whether a feature is *offered*: a modified client can always draw whatever its own game knows. Keep data the player shouldn't have on the server, and send it only while the feature is on.

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
| `bool CanTarget(PlayerIdentity actor, PlayerIdentity target, bool harmful)` | The rule functions use: whether the target's group is protected from the actor's rank (`zomberry.protect.*`). Superadmins and the actor themselves are always allowed |
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

### God mode and other mods

ZomBerry's god mode and invisibility use the engine's damage switch, `SetAllowDamage(false)`. Other mods use the same switch: a safe zone, for example, turns damage off on the way in and back on on the way out. That also ends god mode for an admin who walks out of the zone.

To avoid that, check ZomBerry before turning damage back on:
- **`#define ZOMBERRY_AT`** is set while ZomBerry is loaded, so the code also compiles without it.
- **`bool PlayerBase.ZBIsGod()`** says whether the player is in god mode.
- **`bool PlayerBase.ZBIsInvisible()`** says whether the player is invisible (Toggle invisibility).

```c
void MyModLeaveSafeZone(PlayerBase player) {
#ifdef ZOMBERRY_AT
	if (player.ZBIsGod() || player.ZBIsInvisible()) return; //ZomBerry keeps damage off
#endif
	player.SetAllowDamage(true);
}
```

**`void PlayerBase.ZBSetGod(bool isGod)`** switches god mode on or off, like the stock function but without healing or a chat message.

God mode works on the **server** only. Its flag isn't sent to clients, so `ZBIsGod()` is always `false` there, and `ZBSetGod()` does nothing on a multiplayer client. Invisibility is sent to every client, so `ZBIsInvisible()` works on both sides.

### Testing without a server

[Examples/offlineTest.chernarusplus](Examples/offlineTest.chernarusplus/init.c) is a minimal offline mission: a character on the map, no loot or infected. Copy it to the game's `Missions` folder and start `DayZ_x64.exe` directly (not through BattlEye):
```
DayZ_x64.exe "-mod=@ZomBerry" "-mission=.\Missions\offlineTest.chernarusplus" -window -noPause
```

### Contributing

[AGENTS.md](AGENTS.md) covers how the code is organised, the Enforce Script and DayZ UI pitfalls this project has run into, the rules for RPCs, permissions and config files, and how changes are tested. It's written for AI coding agents, and it's just as useful for people.

### Project layout

| Path | Contents |
|---|---|
| `3_Game/ZBerryRPC.c` | ZomBerry's own RPC layer. All its traffic uses one engine RPC id, `74635`, and each message starts with an opcode from `ZBerryRPCOp` |
| `4_World/` | God mode on `PlayerBase` (`ZBIsGod`, `ZBSetGod`), and the free and spectator cameras |
| `5_Mission/core/` | Permissions (matching, groups, JSON, the Permissions API), server and client auth, request handlers, bans, spawning, version and build info |
| `5_Mission/ui/` | Admin menu components, one class per tab or panel |
| `5_Mission/layouts/` | Layouts of the menu and its components |
| `5_Mission/*.c` | Mission hooks, command API, stock functions, config, logger, keybinds |

## Building from source

Pack `ZomBerry/Addons/scripts` into `scripts.pbo` with the PBO prefix `ZomBerry\AT\scripts`, using DayZ Tools' FileBank or Addon Builder. Then sign it with your own key and ship the matching `.bikey`. The result goes into a mod folder laid out like `ZomBerry/`: `Addons/` with the PBO and its signature, and `Keys/` with the key.

`5_Mission/core/ZBerryBuildInfo.c` describes the build. As committed, it describes a plain release build, and the admin menu header shows the version. Build tooling can fill in the commit hash and mark the build as a development build, so the header shows the commit instead and the logs show both.

## Roadmap

Open ideas and known issues are tracked on [GitHub issues](https://github.com/Moondarker/ZomBerry-DayZAdminTools/issues). Among them:
- Keybinds for functions with parameters
- New functions from the community's suggestions, such as FreeCam camera modes, teleporting a player to a map spot or flipping cars ([#42](https://github.com/Moondarker/ZomBerry-DayZAdminTools/issues/42))
- Translations ([#36](https://github.com/Moondarker/ZomBerry-DayZAdminTools/issues/36))
- Client-side functions registered by other mods ([#38](https://github.com/Moondarker/ZomBerry-DayZAdminTools/issues/38))

What's done is in the [changelog](CHANGELOG.md).
