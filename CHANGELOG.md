# Changelog

## 0.8.1 (in development)

### Fixed

- **Motorbikes:** Refuel and repair now covers them, and boats too; it only knew cars. Motorbikes also show up as vehicle markers on the map and on the ESP.
- **Refuel and repair** restores every damaged part of a vehicle (fuel tank, chassis...), not just the engine, and says when there's no vehicle within 15 m of the target.

## 0.8.0

An admin and kill log you can act on, map markers and saved locations, loadouts, messages, rank protection, a dozen new functions, and much less network traffic.

### Upgrading from 0.7.0

- **Update both sides.** 0.7.x and 0.8.x clients and servers can't talk to each other.
- **Your files keep working:** `ZomBerryPermissions.json`, `ZomBerryBans.json`, `ZomBerryConfig.json` and function keybinds. Function names are unchanged, so `zomberry.functions.<Name>` permissions still apply.
- **New permissions** are granted only by `*` or a matching wildcard:
  - **hand-picked lists** need the new nodes added on the Permissions tab (see the [permission table](README.md#permissions));
  - **wildcards grant new nodes too:** the default `moderator` group's `zomberry.map.*` now also covers object markers, teleporting other players from the map and managing saved locations. Add denies such as `-zomberry.map.teleport_players` if that's too much;
  - **groups with `*`** are protected from lower-ranked admins automatically.
- **New files on the server**, in `<profiles>/ZomBerry/`: `ZomBerryEventLog.bin` (with a `.journal`), `ZomBerryLocations.json`, `ZomBerryLoadouts.json`, `ZomBerryLoadoutOriginals.json`, `ZomBerryDeathLoadouts.json` and `ZomBerryTeleportReturns.json`.
- **New files on admins' computers:** `ZomBerryMyLoadouts.json` and `ZomBerryESP.json`.

### Added

- **Teleport here from the Logs tab:** right-click a death, a teleport, a spawn, a function run on a player or at a spot, a join or logout, a kick, a ban or a saved location to go where it happened, height included (with `zomberry.map.teleport`). Mods' commands that act at a spot can give their entry a position with `SetCallPosition`.
- **Teleport back from the Logs tab:** right-click a teleport to move its players back to where they were before it (new permission `zomberry.teleport.revert`). One player goes back at once; for several, a window lists them, and you send back everyone or only the ones you tick. Every teleport is kept: the Teleport functions, the Map tab and saved locations. Players offline, in a vehicle or protected from your rank stay pending, and players who died since are skipped. Mods can make their own teleports revertable with `RecordMove`.
- **Loadouts in the Spawn tab:** save what you wear as a loadout and try loadouts on yourself. The gear you wore before the first try is kept on the server until you restore it, also across logouts and restarts. Loadouts keep everything, wear, wetness and hotbar slots included. Your own are kept on your side; with `zomberry.loadouts.share` you can share them with the other admins. Shared loadouts can be changed by their owner, by higher-ranked admins, and by anyone with `zomberry.loadouts.manage`. Items your spawn rules don't allow, and items from mods this server doesn't have, are marked and left out; whatever was inside them is put wherever there's room. Online players are listed too: their current gear can be saved as a loadout or tried on. With `zomberry.loadouts.give`, loadouts can be put on the selected players after a confirmation; what they wore is kept, and Undo gives it back. New permissions: `zomberry.loadouts.use`, `.share`, `.manage` and `.give`; new log event type "Loadout".
- **Loot on the ESP:** items lying on the ground, with their name and distance, within their own range (10–300 m, 50 m by default).
- **Kicked and banned players see why:** back in the main menu, their game shows the reason, and for bans how long it still lasts and when it ends. This also covers banned players who try to rejoin.
- **Gear at death in the Logs tab:** right-click a death (or double-click it) for its loadout overview: what the player wore and held when they died. Try it on yourself, give it back to the player (new permission `zomberry.loadouts.restore`), or save a copy among your own loadouts. The last 5 deaths of each player are kept, across clean restarts too.
- **Toggle infinite ammo:** a new stock function that keeps the magazine of the target's weapon full, an empty one too. Weapons without a detachable magazine aren't covered, and an empty chamber still needs a reload.
- **Several players at once:** `Ctrl` + double-click and `Shift` + double-click select more than one player, and `Ctrl` + `A` over the list selects everyone but you. Functions made for it run on all of them and are logged as one event naming every player; others act on the player selected last. Commands from mods opt in with the new `multiTarget` parameter of `AddCommand`.
- **Keys in the function list:** each function's key is shown after its name, and the Bind button has no caption for functions that can't be bound.
- **Object markers on the Map tab** (#58, #61): vehicles, tents, storage, bases and dead bodies.
  - Each type has its own toggle button, icon and colour, and needs its own permission (`zomberry.map.markers.<category>`).
  - Storage and base parts are grouped per area, with a count.
  - The server keeps track of these objects as they appear and disappear, instead of scanning the map.
- **Player orientation on the map** (#61): an arrow in front of each player marker shows which way they face.

- **Protection from lower-ranked admins** (#46): groups can shield their members from functions run by lower-ranked or same-level admins.
  - `zomberry.protect.lower` and `zomberry.protect.same` block all functions; `.lower.harm` and `.same.harm` only block harmful ones.
  - Harmful stock functions: kill, bite, strip, symptoms, set health or blood, god mode toggle, and teleporting the target.
  - Superadmins ignore protections.
  - Groups with `*` are protected automatically, so a moderator can no longer kill a superadmin.
- **Developer APIs:** `AddCommand` has new optional `harmful` and `keybindable` parameters, the Permissions API has `CanTarget()`, and commands can report that they did nothing (`RefuseCall()`), only asked for confirmation (`AwaitConfirmation()`), or describe what they did in the log (`DescribeCall()`, `GetCurrentCall()`).
- **Admin event log, with a Logs tab in the menu** (needs `zomberry.logs.view`). The tab takes the Permissions tab's old place, which moved under the gear icon.
  - **View:** everyone the log mentions on the left (double-click one to see only their events), a search by text, name or Steam64 id, a type filter, and live updates you can pause.
  - **Copying:** right-click any line for **Copy this line** or **Copy the shown log**, with date, type and Steam64 ids.
  - **Events with types:** functions run on yourself, on a player or on the world (each marked harmful or not), spawns, teleports, permission changes, kicks, bans, unbans, refused joins, logins, respawns, logouts, kills, auth problems and refused attempts.
  - **Players** are kept as separate fields (name and Steam64), so the view can highlight them.
  - **Player-against-player damage** goes into a separate compact queue, with hits merged per attacker and victim. A burst ends after 3 s without hits, or as soon as the attacker is hit by another player, so a firefight is logged volley by volley.
  - **Size:** the server keeps the last 2048 events and 10486 damage records.
  - **Kept across restarts:** in `ZomBerryEventLog.bin`, plus a journal of the newest additions, so even a crash loses nothing that was already logged. A full log takes about 0.5–1.3 MB on disk, and never more than about 11 MB. Server starts and clean stops are logged too, as a new event type, "Server".
  - **Long event texts** are cut at 1024 bytes.
  - **Game names in your own language:** animals, vehicles, weapons and objects in the Logs tab show in each admin's game language. The log file keeps the server's.
  - **Deaths say why:**
    - **Killed:** by a player (weapon and distance), by an infected or an animal, run over (naming the driver), or by an explosion.
    - **Died:** by suicide, from a fall, by drowning, burning or barbed wire, or of bleeding, chemical poisoning, thirst or hunger. The last thing that hurt them is mentioned when it was recent.
    - **Forced:** respawning or logging out while unconscious or restrained.
    - **Anything else** shows the player's blood, water and energy. Damage from infected and animals still isn't logged, only damage between players.
  - **Who gets it:** admins with `zomberry.logs.view` receive everything once after joining, in small batches so the server doesn't stall, and live updates from then on.
  - **Log file:** logins, logouts and damage stay out of ZomBerry's log file. Everything else is written there too.
  - **Times** show in each admin's local time. The server keeps them in UTC.
  - **Positions** are written as map coordinates in whole metres ("X 9871 Y 2144"), here and in the log file.
  - **Function calls that did nothing** (no valid target, nothing under the cursor, a player in a vehicle) are logged as refused, with the reason. A first run that only asks for confirmation isn't logged; the confirmed run is. Commands from other mods can do the same with `RefuseCall()` and `AwaitConfirmation()`.
  - **Teleport functions are teleport events** (#71), and each one says where the moved player went, also when it's run only once.
  - **Repeated teleports are one entry:** "Teleport - Me 10 meters Forward" or "Target to Cursor" run again and again (gaps under 3 s) are logged once the run ends, with where the last one went, how often, and where the run started.
  - **Functions say what they did** (#66): one line each, in the log file and the in-game log, instead of "ran …" plus a separate detail line. Toggles say which way they switched: "switched ESP on", "entered FreeCam", "froze PlayerName", "switched god mode off for themselves". Flip, clean up, delete and teleport all name the vehicle, the count or the object, and the place. Commands from other mods can do the same with `DescribeCall()`.
- **Clean up around cursor:** removes infected, dead bodies, loose items or ruined items within 5–200 m. Built and placed things are never touched, and it asks for confirmation. The confirming run deletes what the prompt counted: things that arrived since stay, and so do items someone picked up.
- **Flip vehicle near cursor:** puts a car or boat back on its wheels.
- **Repair all gear:** like Repair item in hands, for everything the player wears and carries. Works on several selected players at once.
- **Teleport the selected player from the map:** `Ctrl` + click, with the new permission `zomberry.map.teleport_players` (rank protections apply).
- **God mode and invisibility protect gear:** nothing the player wears or carries is damaged (weapons don't wear from firing either), their body temperature stays comfortable, and walking barefoot or in ruined shoes no longer cuts them. God mode also keeps stamina full.
- **Toggle invisibility:** a new stock function. Other players don't see, hear or bump into the target (voice chat still works) and get no actions on them, infected and animals ignore them, and nothing can hurt them while it lasts. It survives logging out and respawning. Mods can check it with `PlayerBase.ZBIsInvisible()`, on the server and on clients.
- **Toggle freeze:** a new stock function that stops the selected player in place until released. It survives logging out and back in.
- **Spawning for the selected player:** the Spawn tab's target button now also offers *Player's feet* and *Player's inv.* (new permission `zomberry.spawn.player`, rank protections apply).
- **Teleporting several players at once** (#57): with several players selected, Teleport - Target to Me and Teleport - Target to Cursor place them on a ring around the spot, so they don't end up inside each other.
- **Spectate target** (#60): a new stock function that shows the selected player's view.
  - **What it follows:** where their own camera looks, up and down and free look included. In third person the camera sits over their shoulder. While they're watched, their game reports which mode they use and where they look.
  - **Anywhere on the map:** the area your game loads moves with the camera, and the camera follows a target who teleports or drives away. FreeCam works the same way.
  - **Ending it:** running it again, the target dying or leaving, or losing the permission.
- **ESP** (#59): a new stock function that draws, on the admin's own screen:
  - **players:** box, name, distance and health, and skeletons within 150 m;
  - **vehicles:** box and name;
  - **infected and animals:** a box.

  Players beyond the admin's network range still show by name and distance, streamed by the server.

  **Switching and settings** (#68): running the function switches ESP on, and running it again switches it off. It has no parameters, so it can be bound to a key. What it draws is set on an **ESP tab** under the header's gear: each player detail, far players, vehicles, infected, animals and two ranges. The settings are saved per computer in `ZomBerryESP.json`. The server only streams everyone's position and health while those settings use them.
- **Messages** (#62): a **Message** button on the players panel sends a private message to the selected player (`zomberry.messages.private`) or an announcement to everyone (`zomberry.messages.announce`). Players get a notification and the same text in chat. Messages are logged with their text, as a new event type, "Message".
- **Player info** (#64): the selected player's ping, position, time online, status (alive, unconscious, restrained, dead), vehicle and Steam64 ID show under the player list, with a **Copy ID** button. The server sends them for the selected player only, every 5 seconds. The position needs `zomberry.map.player_markers`, like the map's player markers.
- **Teleport by coordinates and saved locations** (#65) on the Map tab:
  - **Coordinates:** type X and Y, then teleport yourself or the selected player there.
  - **Saved locations:** a list shared by all admins, kept in `ZomBerryLocations.json`. Show one on the map, teleport yourself or the selected player there (height included), save your position under a name or delete one (`zomberry.map.locations.manage`). They appear on the map as their own marker type.
  - **Map filters** moved into a dropdown at the top left, which opens when you point at it.
  - Saving and deleting locations is logged as a new event type, "Location".
- **Client-side functions for mods** (#38): server code can switch a named feature on one client (`SetClientFeature`), and that client runs the handler registered for it (`RegisterClientFeature`). Features with a required permission switch off by themselves when it's removed.

### Changed

- **Functions are sorted into new categories:** Teleport, Player, Punish, Camera, World and Objects, instead of OnTarget, `==` and OnServer. Commands from other mods that still name the old categories land in Player, Punish and World, and OnServer ones are logged as server-wide.
- **Set time, Set date and Set weather start from the game's current time, date and weather** when you select them, instead of fixed values.
- **FreeCam:** your own body is hidden and silent on your screen while you're away from it, and the camera works in underground areas, going through the ground where something is built below.
- **Keybind code reworked** (#51): binding, unbinding and clearing are separate operations instead of magic values. Existing keybinds keep working.
- **Spawn target picker:** hovering over the Spawn tab's target button opens a list of places above it to pick from. Clicking the button still steps through them. The list closes when the mouse leaves.
- **Much less network traffic for the player list and the map.** Clients now tell the server what they show, and the server only sends that, only when it changed.
  - **The roster** (names, admins, what players hold) is only sent when it changes.
  - **Positions, headings, and the selected player's health** go out as about 12 bytes per player. After the first update only changes are sent, plus a full update every 10 seconds.
  - **Map markers** are only resent when their category changed.
  - **Before**, every open menu fetched the whole player list every 2–5 seconds, and every map marker every 10 seconds.
- **Every log line names players the same way**, "Name (Steam64 id)". Function, spawn, teleport, kick and ban lines used the BattlEye GUID before. Offline bans by GUID still log the GUID, the only ID they have.
- **Function and spawn requests** no longer carry the admin's ID, which the server ignored anyway (#52).
- **A close button and a settings menu in the header** (#55): the **×** in the top-right corner closes the admin menu with the mouse, so the menu can be left even when the keyboard stops reaching it. The gear icon next to it opens a dropdown with the Permissions and ESP tabs. It lists only the pages the admin may use, without gaps.

- **The log says what a permission change changed:** one line per change.
  - **Groups:** added, removed, level changes, spawn rules added or removed, and each permission that changed, in the Permissions tab's words: "zomberry.players.ban allowed -> denied" (allowed, denied or unset).
  - **Admins:** added, removed, or moved to another group.
  - **Before**, every save logged only the number of groups and admins.

### Fixed

- **Delete object near cursor picked the wrong thing:** it took the first object within 1.5 m, could hit map objects, which only vanish for players who join later, and confirmed by comparing text. It now picks the object the cursor is on (by its bounding box), only networked objects, never living players, and confirms by network ID.
- **Set date couldn't set the 31st:** days only went up to 30.
- **Set weather made almost no storm** (#48): the storm density was calculated with whole numbers, so it was 0 unless overcast and rain were both 100.
- **Set weather barely worked on servers.** The server's own weather took over again right away: a storm often brought no rain at all, and clear weather kept raining for about a minute. The chosen weather now applies at once and holds for 6 minutes.
- **FreeCam started inside the admin's head.** It now starts just behind it.
- **Aiming was off after FreeCam:** back in the body, the weapon pointed somewhere other than the view, and aiming down sights snapped to it.
- **The server crashed when a banned player tried to rejoin.**
- **The game could crash when joining a server again** after leaving one.
- **Permissions tab:** toggling a permission scrolled the list back to the top, and a custom pattern switched to unset disappeared, so it couldn't be switched back on without typing it again.
- **The Functions tab looked ready to run after reopening the menu, but wasn't** (#67). Reopening cleared the selection, but the function's name and parameters stayed, and named values turned into numbers. The selected function and its values are now kept.
- **Long value names were cut off** on the Functions tab ("Ruined items" showed as "Ruinec"): the value box is wider, with smaller text.

## 0.7.0

The big rewrite: own RPC layer and no dependencies, permission groups, kicks and bans, a rebuilt admin menu that scales with the resolution, and APIs for other mods.

### Upgrading from 0.6.x

- **Clients and servers both need 0.7.0.** A 0.6.x client gets no answer from a 0.7.0 server, and the other way round, so the menu doesn't open at all.
- **Community Framework is no longer needed.**
- **`admins.cfg` is imported** on the first start, then renamed to `admins.cfg.imported`:
  - the first ID becomes `superadmin`;
  - the others go into an `admin` group (level 90).

  From then on, admins live in `ZomBerryPermissions.json`. See [Upgrading from admins.cfg](README.md#upgrading-from-adminscfg).
- **Removed options:** `-zbryInstallMode`, the `MenuKey` config entry and the old `ZomBerry.cfg`. The menu key is set in DayZ's control settings (**Open Admin menu**), and the first admin comes from the setup token.
- **Keep:** `ZomBerryConfig.json` and function keybinds, both of which still work.

### Added

- **Permission groups** in `ZomBerryPermissions.json`. A group has:
  - permission patterns with `*`/`?` wildcards and `-` denies;
  - a level (0-100) that decides who may change whom;
  - spawn rules (class names, base classes, wildcards, denies).

  Every node is listed in the [README](README.md#permissions). Every function has its own node (`zomberry.functions.<name>`), including commands added by missions and mods.
- **In-game Permissions tab:**
  - assign players to groups;
  - edit permissions with allow/deny/unset toggles, and spawn rules;
  - create and delete groups, and set their levels;
  - preview which classes a group may spawn.
- **Setup token for the first admin.** With no admins configured, the server prints a token into its logs. The first player to enter it in-game becomes superadmin.
- **Kicks and bans** from the players panel:
  - free-text reasons, shown in chat to the player;
  - durations (1 hour to permanent), and offline bans by Steam64 ID or GUID;
  - a ban list with time left and Unban;
  - bans stored in `ZomBerryBans.json` and enforced while players connect;
  - new nodes `zomberry.players.kick`, `.ban` and `.unban`.
- **Permissions API**, `GetZomberryPermAPI()`, for other mods and missions:
  - register their own nodes, so they show in the Permissions tab;
  - check rights on the server (`HasPermission`, `GetLevel`, `CanActOn`, …) and on the client (`HasLocalPermission`, `OnLocalPermissionsChanged`).
- **Named values for function parameters.** Partial heal shows No/Yes, and Set date shows month names. Admins can type either a name or a number.
- **Inventory spawning** loads a matching magazine into the weapon in hands, and says in chat when an item doesn't fit or can't be carried.
- **Automatic refresh:** the player list updates every 5 seconds while the menu is open, every 2 seconds while the map is shown, and the ban list every 30 seconds.
- **Minimal offline mission** for testing without a server (`Examples/offlineTest.chernarusplus`).
- **Dev builds** show their commit in the menu header and logs.

### Changed

- **No dependencies.** ZomBerry has its own RPC layer on a single RPC ID (`74635`), which replaces Community Framework's RPCManager.
- **The admin menu was rebuilt** from self-contained UI components while keeping its look. Players' map markers update live.
- **The menu scales with the resolution.**
  - Captions, labels, edit boxes and list text are sized for the screen.
  - Lists switch to a matching font size.
  - The quickbar is hidden while the menu is open.
- **The players panel:**
  - Kick, Ban and Bans buttons replace the Update button;
  - health and blood share one info line;
  - the list grows when you have no kick or ban rights.
- **The server checks every admin request** against the sender's permissions.
  - The admin is always the sender, never an ID sent by the client.
  - Unknown functions and classes are refused.
  - Player positions only go to admins who may see map markers.
- **Version checks** parse major.minor.patch, so 0.10 and later compare correctly.
- **`ZomBerryConfig.json`:** a broken file is reported with the reason and default settings are used. A config without `SpawnMenuSorting` gets the default spawn groups.
- **Refuel and repair** builds its car-part list once instead of for every car.
- **Logs:** the player sync log moved to debug level 1.

### Fixed

- **The menu couldn't be closed** on current DayZ versions: the menu key and `Esc` stopped working while it was open, so players had to Alt+F4. The menu took extra input focus on top of the focus DayZ menus already take.
- **Function keybinds ran while typing in chat.** With the default `Delete` bind, that could delete the object at your cursor. They also ran in other menus.
- **Script errors when an admin without a character** (dead or respawning) used keybinds, FreeCam or teleports.
- **Stock functions:**
  - god mode now heals without a fake admin message;
  - healing works in singleplayer;
  - Induce vomiting handles a target that can't vomit right now.
- **All script compile warnings.**

### Removed

- **Installation mode** (`-zbryInstallMode`) and the hard-coded example admin ID in new `admins.cfg` files.
- **Legacy configuration:**
  - the `ZomBerry.cfg` reader;
  - the `MenuKey` entry;
  - `admins.cfg` lookup in the mission folder;
  - the 2019 move of `ZomBerryConfig.json` from the profile root.
- **The Community Framework dependency.**

## Earlier versions

Release notes for 0.6.4 and earlier are on the [GitHub releases page](https://github.com/Moondarker/ZomBerry-DayZAdminTools/releases).
