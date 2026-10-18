# Changelog

## 0.7.0

The big rewrite: own RPC layer and no dependencies, permission groups, kicks and bans, a rebuilt admin menu that scales with the resolution, and APIs for other mods.

### Upgrading from 0.6.x

- **Clients and servers both need 0.7.0.** A 0.6.x client gets no answer from a 0.7.0 server, and the other way round, so the menu doesn't open at all.
- **Community Framework is no longer needed.** Remove it from `-mod=` unless another mod uses it.
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
