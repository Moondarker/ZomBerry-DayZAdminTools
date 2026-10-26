//Server side of admin rights: permission groups and admins from ZomBerryPermissions.json, the first-startup token,
//and the permission editing RPCs. Clients only ever get told their own rights (AuthState), the server decides everything
class ZBerryAuth {
	static const string CONFIG_FILE = "ZomBerryPermissions.json";
	static const string SUPERADMIN_GROUP = "superadmin";
	static const string ADMIN_GROUP = "admin"; //Further ids of a legacy admins.cfg
	static const int ADMIN_LEVEL = 90;
	static const int MAX_TOKEN_ATTEMPTS = 5;

	protected ref ZBerryPermsConfig m_Config = new ZBerryPermsConfig;
	protected string m_ConfigPath;
	protected string m_SetupToken = ""; //Set while no admin exists
	protected ref TStringArray m_LastChanges = new TStringArray; //What the last ApplyUpdate changed, for the log
	protected ref map<string, int> m_TokenAttempts = new map<string, int>;

	void ZBerryAuth() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.AdminAuth, "AdminAuth", this, ZBerrySPExec.Server );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SetupToken, "SetupToken", this, ZBerrySPExec.Server );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.PermsRequest, "PermsRequest", this, ZBerrySPExec.Server );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.PermsUpdate, "PermsUpdate", this, ZBerrySPExec.Server );
	}

	static bool EveryoneIsAdmin() {
		string value;
		return GetCLIParam("zbryGiveAdminRightsToEveryone", value) && value == "true";
	}

	//Server start
	void LoadAdmins() {
		m_ConfigPath = ZomberryBase.GetConfig().GetConfigDir() + CONFIG_FILE;

		if (FileExist(m_ConfigPath)) {
			ZBerryPermsConfig loaded;
			string loadError;
			if (!JsonFileLoader<ZBerryPermsConfig>.LoadFile(m_ConfigPath, loaded, loadError) || !loaded) {
				ZomberryBase.Log( "ZomBerryAuth", "ERROR: " + CONFIG_FILE + " can't be read, nobody is an admin until it's fixed: " + loadError );
				return; //Never overwrite a file the owner may want to fix by hand
			}

			m_Config = loaded;
			m_Config.Sanitize();

			ZBerryPermGroup superadmin = m_Config.FindGroup(SUPERADMIN_GROUP);
			if (superadmin) superadmin.Level = ZBerryPermGroup.SUPERADMIN_LEVEL; //Always the top, also in files without levels
			ZomberryBase.Log( "ZomBerryAuth", "INFO: Loaded " + m_Config.Groups.Count() + " groups and " + m_Config.Admins.Count() + " admins from " + CONFIG_FILE );
		} else {
			CreateDefaultConfig();
		}

		if (EveryoneIsAdmin()) {
			ZomberryBase.Log( "ZomBerryAuth", "WARN: WARNING! EVERY PLAYER JOINED TO THIS SERVER WILL HAVE FULL ADMIN RIGHTS DUE TO '-zbryGiveAdminRightsToEveryone=true' LAUNCH PARAMETER!" );
		}

		if (m_Config.Admins.Count() == 0) GenerateSetupToken();
	}

	//First start: example groups, plus admins imported from a legacy admins.cfg if there is one
	protected void CreateDefaultConfig() {
		m_Config.Groups.Insert(NewFullGroup(SUPERADMIN_GROUP, ZBerryPermGroup.SUPERADMIN_LEVEL));

		ZBerryPermGroup moderator = new ZBerryPermGroup("moderator", 10);
		moderator.Permissions.Insert(ZBerryPermissions.PLAYERS_LIST);
		moderator.Permissions.Insert(ZBerryPermissions.PLAYERS_KICK);
		moderator.Permissions.Insert("zomberry.map.*");
		moderator.Permissions.Insert("zomberry.functions.TPToTarget");
		moderator.Permissions.Insert("zomberry.functions.TPToAdmin");
		moderator.Permissions.Insert("zomberry.functions.HealTarget");
		m_Config.Groups.Insert(moderator);

		string legacyPath;
		TStringArray legacyIds = ReadLegacyAdmins(legacyPath);
		ImportLegacyAdmins(legacyIds);

		if (!SaveConfig()) return;
		ZomberryBase.Log( "ZomBerryAuth", "INFO: Created " + CONFIG_FILE );

		//Imported once: keep the old file around, but out of the way
		if (legacyPath != "" && CopyFile(legacyPath, legacyPath + ".imported")) {
			DeleteFile(legacyPath);
			ZomberryBase.Log( "ZomBerryAuth", "INFO: Renamed " + legacyPath + " to admins.cfg.imported, " + CONFIG_FILE + " is used from now on" );
		}
	}

	protected ZBerryPermGroup NewFullGroup(string name, int level) {
		ZBerryPermGroup group = new ZBerryPermGroup(name, level);
		group.Permissions.Insert("*");
		group.SpawnAllowed.Insert("*");
		return group;
	}

	//The first id becomes superadmin, the others admins with every permission. Admins can't change the
	//superadmin or their own group: the admin group ranks below superadmin and not below itself
	protected void ImportLegacyAdmins(TStringArray ids) {
		for (int i = 0; i < ids.Count(); ++i) {
			if (m_Config.FindAdmin(ids[i])) continue;

			string groupName = SUPERADMIN_GROUP;
			if (i > 0) {
				groupName = ADMIN_GROUP;
				if (!m_Config.FindGroup(ADMIN_GROUP)) m_Config.Groups.Insert(NewFullGroup(ADMIN_GROUP, ADMIN_LEVEL));
			}

			m_Config.Admins.Insert(new ZBerryAdminEntry(ids[i], "", groupName));
		}

		if (ids.Count() > 0) ZomberryBase.Log( "ZomBerryAuth", "INFO: Imported " + m_Config.Admins.Count() + " admins from admins.cfg: the first into " + SUPERADMIN_GROUP + ", the others into " + ADMIN_GROUP );
	}

	//admins.cfg of earlier versions: one id per line. The ids those versions wrote into a new file by themselves
	//(the author's own, as an example) are skipped, so nobody becomes admin on a server that never chose them
	protected TStringArray ReadLegacyAdmins(out string foundPath) {
		TStringArray ids = new TStringArray;
		TStringArray seededIds = {"76561198038543835", "YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54="};
		TStringArray paths = {"$profile:ZomBerry\\admins.cfg", "$CurrentDir:admins.cfg"};
		foundPath = "";

		foreach (string path : paths) {
			if (!FileExist(path)) continue;

			FileHandle file = OpenFile(path, FileMode.READ);
			if (file == 0) continue;

			string line;
			while (FGets(file, line) >= 0) {
				line = line.Trim();
				if (seededIds.Find(line) != -1) {
					ZomberryBase.Log( "ZomBerryAuth", "INFO: Skipped the example id earlier versions wrote into " + path + " by default" );
				} else if (line != "" && line.IndexOf("//") != 0 && ids.Find(line) == -1) {
					ids.Insert(line);
				}
				line = "";
			}
			CloseFile(file);
			foundPath = path;
			ZomberryBase.Log( "ZomBerryAuth", "INFO: Found legacy admins file " + path );
			break;
		}

		return ids;
	}

	protected bool SaveConfig() {
		string saveError;
		if (JsonFileLoader<ZBerryPermsConfig>.SaveFile(m_ConfigPath, m_Config, saveError)) return true;

		ZomberryBase.Log( "ZomBerryAuth", "ERROR: Can't save " + CONFIG_FILE + ": " + saveError );
		return false;
	}

	protected void GenerateSetupToken() {
		string alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; //No O/0 and I/1 to mix up
		Math.Randomize(-1);

		m_SetupToken = "";
		for (int i = 0; i < 12; ++i) {
			if (i > 0 && i % 4 == 0) m_SetupToken += "-";
			m_SetupToken += alphabet.Get(Math.RandomInt(0, alphabet.Length()));
		}

		//Hard to miss in both logs: banner, instructions, the token, a closing line
		TStringArray banner = {" _____                 ____                       ", "|__  /___  _ __ ___   | __ )  ___ _ __ _ __ _   _ ", "  / // _ \\| '_ ` _ \\  |  _ \\ / _ \\ '__| '__| | | |", " / /| (_) | | | | | | | |_) |  __/ |  | |  | |_| |", "/____\\___/|_| |_| |_| |____/ \\___|_|  |_|   \\__, |", "                                            |___/ "};
		banner.Insert("");
		banner.Insert("No ZomBerry admins are configured yet. Join the server, press the admin menu key (O)");
		banner.Insert("and enter this setup token to become superadmin:");
		banner.Insert("");
		banner.Insert("        SETUP TOKEN:   " + m_SetupToken);
		banner.Insert("==================================================================================");

		bool scriptLogToo = !ZomberryBase.GetLogger().IsUsingScriptLog();
		foreach (string line : banner) {
			if (scriptLogToo) Print("[ZomBerryAuth] " + line);
			ZomberryBase.Log( "ZomBerryAuth", line );
		}
	}

	protected ZBerryAdminEntry FindAdmin(PlayerIdentity identity) {
		if (!identity) return NULL;

		ZBerryAdminEntry admin = m_Config.FindAdmin(identity.GetPlainId());
		if (!admin) admin = m_Config.FindAdmin(identity.GetId()); //Legacy BattlEye GUID entries
		return admin;
	}

	protected ZBerryPermGroup FindGroupOf(PlayerIdentity identity) {
		ZBerryAdminEntry admin = FindAdmin(identity);
		if (!admin) return NULL;

		return m_Config.FindGroup(admin.Group);
	}

	//Rank of the identity's group, -1 for non-admins
	int GetLevel(PlayerIdentity identity) {
		if (EveryoneIsAdmin()) return ZBerryPermGroup.SUPERADMIN_LEVEL;

		ZBerryPermGroup group = FindGroupOf(identity);
		if (!group) return -1;
		return group.Level;
	}

	//Group name, "" for non-admins
	string GetGroupName(PlayerIdentity identity) {
		if (EveryoneIsAdmin()) return SUPERADMIN_GROUP;

		ZBerryPermGroup group = FindGroupOf(identity);
		if (!group) return "";
		return group.Name;
	}

	//Rank of whoever has this id (Steam64 or GUID) in the config, -1 for non-admins
	int GetLevelOfId(string id) {
		if (id == "") return -1;

		ZBerryAdminEntry admin = m_Config.FindAdmin(id);
		if (!admin) return -1;

		ZBerryPermGroup group = m_Config.FindGroup(admin.Group);
		if (!group) return -1;
		return group.Level;
	}

	//Functions: may an admin of actorLevel run a (harmful) function on a target of targetLevel, whose group grants
	//targetPermissions? Targets shield themselves with zomberry.protect.* nodes, level 100 ignores those
	static bool CanTargetLevels(int actorLevel, int targetLevel, TStringArray targetPermissions, bool harmful) {
		if (targetLevel < 0 || actorLevel >= ZBerryPermGroup.SUPERADMIN_LEVEL || targetLevel < actorLevel) return true;

		string allNode = ZBerryPermissions.PROTECT_LOWER;
		string harmNode = ZBerryPermissions.PROTECT_LOWER_HARM;
		if (targetLevel == actorLevel) {
			allNode = ZBerryPermissions.PROTECT_SAME;
			harmNode = ZBerryPermissions.PROTECT_SAME_HARM;
		}

		if (ZBerryPermissions.IsGranted(targetPermissions, allNode)) return false;
		return !harmful || !ZBerryPermissions.IsGranted(targetPermissions, harmNode);
	}

	bool CanTarget(PlayerIdentity actor, PlayerIdentity target, bool harmful) {
		if (!GetGame().IsMultiplayer() || !actor || !target) return true;
		if (actor.GetPlainId() == target.GetPlainId()) return true;

		TStringArray targetPermissions = GetPermissions(target);
		return CanTargetLevels(GetLevel(actor), GetLevel(target), targetPermissions, harmful);
	}

	//Kicks and bans: admins can only act on admins ranked below them, superadmins on everyone
	static bool CanActOnLevel(int actorLevel, int targetLevel) {
		if (targetLevel < 0) return true;

		return actorLevel >= ZBerryPermGroup.SUPERADMIN_LEVEL || targetLevel < actorLevel;
	}

	//The patterns this identity has, empty for non-admins
	TStringArray GetPermissions(PlayerIdentity identity) {
		TStringArray result = new TStringArray;
		if (EveryoneIsAdmin()) {
			result.Insert("*");
			return result;
		}

		ZBerryPermGroup group = FindGroupOf(identity);
		if (group) result.InsertAll(group.Permissions);
		return result;
	}

	TStringArray GetSpawnRules(PlayerIdentity identity) {
		TStringArray result = new TStringArray;
		if (EveryoneIsAdmin()) {
			result.Insert("*");
			return result;
		}

		ZBerryPermGroup group = FindGroupOf(identity);
		if (group) result.InsertAll(group.SpawnAllowed);
		return result;
	}

	//In a group that exists. Singleplayer and clients ask GetZBerryClientAuth() instead
	bool IsAdmin(PlayerIdentity identity) {
		if (!GetGame().IsMultiplayer()) return true;
		if (EveryoneIsAdmin()) return true;

		return FindGroupOf(identity) != NULL;
	}

	bool HasPermission(PlayerIdentity identity, string node) {
		if (!GetGame().IsMultiplayer()) return true;

		TStringArray permissions = GetPermissions(identity);
		return ZBerryPermissions.IsGranted(permissions, node);
	}

	bool CanSpawn(PlayerIdentity identity, string className) {
		if (!GetGame().IsMultiplayer()) return true;

		TStringArray rules = GetSpawnRules(identity);
		return ZBerryPermissions.IsSpawnAllowed(rules, className);
	}

	//Tell one client what it may do
	void PushAuthState(PlayerIdentity identity) {
		if (!identity) return;

		int state = ZBerryAuthState.NONE;
		if (IsAdmin(identity)) {
			state = ZBerryAuthState.ADMIN;
		} else if (m_SetupToken != "") {
			state = ZBerryAuthState.SETUP_PENDING;
		}

		TStringArray permissions = GetPermissions(identity);
		TStringArray spawnRules = GetSpawnRules(identity);
		GetZBerryEventLog().StartSync(identity); //Only if they may read the log and aren't synced yet
		GetZBerryRPC().SendRPC( ZBerryRPCOp.AuthState, new Param4<string, int, ref TStringArray, ref TStringArray>(g_zbryVer, state, permissions, spawnRules), true, identity );
	}

	void PushAuthStateToEveryone() {
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man player : players) {
			if (ZBGetIdentity(player)) PushAuthState(ZBGetIdentity(player));
		}
		GetZBerryClientFeatures().Revalidate(this); //Rights may be gone: switch off what they allowed
	}

	protected bool IsServerCall(ZBerryCallType type, PlayerIdentity sender) {
		return type == ZBerryCallType.Server && GetGame().IsServer() && GetGame().IsMultiplayer() && sender;
	}

	protected string Who(PlayerIdentity sender) {
		return ZBDescribePlayer(sender);
	}

	protected void Tell(PlayerIdentity sender, string message) {
		ZomberryStockFunctions.MessagePlayer(ZBGetPlayerById(sender.GetPlayerId()), message);
	}

	void AdminAuth( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< bool, string > authInfo; //direction (always true), client version
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( authInfo ) ) {
			ZomberryBase.LogEvent( ZBerryEventType.AUTH, sender, NULL, "{actor} couldn't authenticate, their ZomBerry is too old (server: v" + g_zbryVer + ")" );
			return;
		}

		if (authInfo.param2 != g_zbryVer) {
			ZomberryBase.LogEvent( ZBerryEventType.AUTH, sender, NULL, "{actor} has another ZomBerry version: v" + authInfo.param2 + " (server: v" + g_zbryVer + ")" );
		}

		ZBerryAdminEntry admin = FindAdmin(sender);
		if (admin && admin.Name != sender.GetName()) {
			admin.Name = sender.GetName();
			SaveConfig();
		}

		if (IsAdmin(sender)) {
			ZomberryBase.Log( "ZomBerryDbg", "Auth respond to admin " + Who(sender) );
		} else if (m_SetupToken != "") {
			ZomberryBase.LogEvent( ZBerryEventType.AUTH, sender, NULL, "{actor} joined while setup is pending, look for the SETUP TOKEN banner in the server's logs" );
		}

		PushAuthState(sender);
	}

	void SetupToken( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<string> tokenParam;
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( tokenParam ) ) return;

		string id = sender.GetPlainId();
		if (m_SetupToken == "") {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} sent a setup token, but setup is already done" );
			PushAuthState(sender);
			return;
		}

		int attempts = m_TokenAttempts.Get(id);
		if (attempts >= MAX_TOKEN_ATTEMPTS) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} sent a setup token after " + MAX_TOKEN_ATTEMPTS + " wrong attempts, ignored" );
			Tell(sender, "Too many wrong setup tokens");
			return;
		}

		string token = tokenParam.param1.Trim();
		token.ToUpper();
		if (token != m_SetupToken) {
			m_TokenAttempts.Set(id, attempts + 1);
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} sent a wrong setup token (attempt " + (attempts + 1) + "/" + MAX_TOKEN_ATTEMPTS + ")" );
			Tell(sender, "Wrong setup token");
			return;
		}

		ZBerryPermGroup superadmin = m_Config.FindGroup(SUPERADMIN_GROUP);
		if (!superadmin) {
			superadmin = NewFullGroup(SUPERADMIN_GROUP, ZBerryPermGroup.SUPERADMIN_LEVEL);
			m_Config.Groups.Insert(superadmin);
		}
		if (superadmin.Permissions.Find("*") == -1) superadmin.Permissions.Insert("*");
		superadmin.Level = ZBerryPermGroup.SUPERADMIN_LEVEL;

		m_Config.Admins.Insert(new ZBerryAdminEntry(id, sender.GetName(), SUPERADMIN_GROUP));
		SaveConfig();
		m_SetupToken = "";
		m_TokenAttempts.Clear();

		ZomberryBase.LogEvent( ZBerryEventType.AUTH, sender, NULL, "{actor} entered the setup token and is now in the " + SUPERADMIN_GROUP + " group" );
		Tell(sender, "Setup done, you are a superadmin now");
		PushAuthStateToEveryone();
	}

	ZBerryPermsSnapshot BuildSnapshot() {
		ZBerryPermsSnapshot snapshot = new ZBerryPermsSnapshot;
		snapshot.Groups = m_Config.Groups;
		snapshot.Admins = m_Config.Admins;
		snapshot.KnownNodes.InsertAll(ZBerryPermissions.GetFixedNodes());

		ZBerryCategoryArray categories = GetZomberryCmdAPI().GetList();
		foreach (ZBerryCategory category : categories) {
			ZBerryFunctionArray functions = category.GetAll();
			foreach (ZBerryFunction cmdFunc : functions) {
				string node = ZBerryPermissions.FunctionNode(cmdFunc.GetName());
				if (snapshot.KnownNodes.Find(node) == -1) snapshot.KnownNodes.Insert(node);
			}
		}

		//Nodes of other mods and missions last, after all of ZomBerry's own
		snapshot.KnownNodes.InsertAll(GetZomberryPermAPI().GetRegisteredNodes());

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man player : players) {
			PlayerIdentity identity = ZBGetIdentity(player);
			if (identity) snapshot.Online.Insert(new ZBerryOnlineIdentity(identity.GetPlayerId(), identity.GetPlainId(), identity.GetName()));
		}

		return snapshot;
	}

	void SendSnapshot(PlayerIdentity recipient) {
		ZBerryPermsSnapshot snapshot = BuildSnapshot();
		snapshot.EditorLevel = GetLevel(recipient);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.PermsData, new Param1<ref ZBerryPermsSnapshot>(snapshot), true, recipient );
	}

	void PermsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!IsServerCall(type, sender)) return;
		if (!HasPermission(sender, ZBerryPermissions.PERMISSIONS_MANAGE)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to read permissions without " + ZBerryPermissions.PERMISSIONS_MANAGE );
			return;
		}

		SendSnapshot(sender);
	}

	void PermsUpdate( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryPermsSnapshot> update;
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( update ) || !update.param1 ) return;

		if (!HasPermission(sender, ZBerryPermissions.PERMISSIONS_MANAGE)) {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to change permissions without " + ZBerryPermissions.PERMISSIONS_MANAGE );
			return;
		}

		string problem = ApplyUpdate(update.param1, GetLevel(sender));
		if (problem != "") {
			ZomberryBase.LogEvent( ZBerryEventType.REFUSED, sender, NULL, "{actor} tried to change permissions, rejected: " + problem );
			Tell(sender, "Permissions not saved: " + problem);
			SendSnapshot(sender);
			return;
		}

		if (m_LastChanges.Count() == 0) ZomberryBase.LogEvent( ZBerryEventType.PERMISSIONS, sender, NULL, "{actor} saved permissions without changes" );
		foreach (string change : m_LastChanges) {
			ZomberryBase.LogEvent( ZBerryEventType.PERMISSIONS, sender, NULL, "{actor} changed permissions: " + change );
		}
		Tell(sender, "Permissions saved");
		PushAuthStateToEveryone();
		SendSnapshot(sender);
	}

	//Checks and stores a new set of groups and admins made by a manager of editorLevel. Returns why it was
	//rejected, "" when saved. Below the superadmin level, groups ranked at or above the editor's own level and the
	//admins in them must stay exactly as they were, and nothing may be added at or above that level
	string ApplyUpdate(ZBerryPermsSnapshot update, int editorLevel = 100) {
		if (!update.Groups || !update.Admins) return "incomplete data";

		ZBerryPermsConfig newConfig = new ZBerryPermsConfig;
		foreach (ZBerryPermGroup group : update.Groups) {
			if (!group) continue;
			group.Sanitize();
			group.Name = group.Name.Trim();
			if (group.Name == "") return "a group has no name";
			if (newConfig.FindGroup(group.Name)) return "group " + group.Name + " exists twice";
			newConfig.Groups.Insert(group);
		}

		bool anyManager = false;
		foreach (ZBerryAdminEntry admin : update.Admins) {
			if (!admin || admin.Id == "") continue;
			if (newConfig.FindAdmin(admin.Id)) return "admin " + admin.Id + " is listed twice";

			ZBerryPermGroup adminGroup = newConfig.FindGroup(admin.Group);
			if (!adminGroup) return "admin " + admin.Name + " is in unknown group " + admin.Group;
			if (ZBerryPermissions.IsGranted(adminGroup.Permissions, ZBerryPermissions.PERMISSIONS_MANAGE)) anyManager = true;

			newConfig.Admins.Insert(admin);
		}

		//Nobody could fix the permissions in-game anymore
		if (!anyManager) return "no admin would be left with " + ZBerryPermissions.PERMISSIONS_MANAGE;

		string hierarchyProblem = CheckHierarchy(m_Config, newConfig, editorLevel);
		if (hierarchyProblem != "") return hierarchyProblem;

		m_LastChanges = DescribeChanges(m_Config, newConfig);
		m_Config = newConfig;
		SaveConfig();
		return "";
	}

	//One line per change between two configs, for the log: groups added, removed or edited (level, permissions,
	//spawn rules), admins added, removed or moved to another group
	static TStringArray DescribeChanges(ZBerryPermsConfig oldConfig, ZBerryPermsConfig newConfig) {
		TStringArray changes = new TStringArray;

		foreach (ZBerryPermGroup oldGroup : oldConfig.Groups) {
			if (!newConfig.FindGroup(oldGroup.Name)) changes.Insert("removed group " + oldGroup.Name);
		}

		foreach (ZBerryPermGroup newGroup : newConfig.Groups) {
			ZBerryPermGroup before = oldConfig.FindGroup(newGroup.Name);
			if (!before) {
				changes.Insert("added group " + newGroup.Name + " (level " + newGroup.Level + ", permissions: " + DescribeList(newGroup.Permissions) + ", spawn rules: " + DescribeList(newGroup.SpawnAllowed) + ")");
				continue;
			}

			if (before.Level != newGroup.Level) changes.Insert("group " + newGroup.Name + ": level " + before.Level + " -> " + newGroup.Level);
			DescribePermissionChanges(newGroup.Name, before.Permissions, newGroup.Permissions, changes);
			string spawnRules = DescribeListChange(before.SpawnAllowed, newGroup.SpawnAllowed);
			if (spawnRules != "") changes.Insert("group " + newGroup.Name + ": spawn rules " + spawnRules);
		}

		foreach (ZBerryAdminEntry oldAdmin : oldConfig.Admins) {
			if (!newConfig.FindAdmin(oldAdmin.Id)) changes.Insert("removed admin " + DescribeAdmin(oldAdmin) + " from group " + oldAdmin.Group);
		}

		foreach (ZBerryAdminEntry newAdmin : newConfig.Admins) {
			ZBerryAdminEntry adminBefore = oldConfig.FindAdmin(newAdmin.Id);
			if (!adminBefore) {
				changes.Insert("added admin " + DescribeAdmin(newAdmin) + " to group " + newAdmin.Group);
			} else if (adminBefore.Group != newAdmin.Group) {
				changes.Insert("moved admin " + DescribeAdmin(newAdmin) + " from group " + adminBefore.Group + " to " + newAdmin.Group);
			}
		}

		return changes;
	}

	protected static string DescribeAdmin(ZBerryAdminEntry admin) {
		if (admin.Name == "") return admin.Id;
		return admin.Name + " (" + admin.Id + ")";
	}

	protected static string DescribeList(TStringArray items) {
		if (items.Count() == 0) return "none";
		return ZBerryPermissions.JoinList(items);
	}

	//"added a, b; removed c", "" when nothing changed. Spawn rules can start with "-" themselves, so no +/- signs
	protected static string DescribeListChange(TStringArray before, TStringArray after) {
		TStringArray added = new TStringArray;
		TStringArray removed = new TStringArray;
		foreach (string item : after) {
			if (before.Find(item) == -1) added.Insert(item);
		}
		foreach (string oldItem : before) {
			if (after.Find(oldItem) == -1) removed.Insert(oldItem);
		}

		string result = "";
		if (added.Count() > 0) result = "added " + ZBerryPermissions.JoinList(added);
		if (removed.Count() > 0) {
			if (result != "") result += "; ";
			result += "removed " + ZBerryPermissions.JoinList(removed);
		}
		return result;
	}

	//One line per node that changed, in the Permissions tab's three states: "group admin: zomberry.players.ban allowed -> denied"
	protected static void DescribePermissionChanges(string groupName, TStringArray before, TStringArray after, TStringArray changes) {
		TStringArray nodes = new TStringArray;
		AddPermissionNodes(before, nodes);
		AddPermissionNodes(after, nodes);

		foreach (string node : nodes) {
			string stateBefore = DescribeNodeState(before, node);
			string stateAfter = DescribeNodeState(after, node);
			if (stateBefore != stateAfter) changes.Insert("group " + groupName + ": " + node + " " + stateBefore + " -> " + stateAfter);
		}
	}

	protected static void AddPermissionNodes(TStringArray entries, TStringArray nodes) {
		foreach (string entry : entries) {
			string node = entry;
			if (node.IndexOf("-") == 0) node = node.Substring(1, node.Length() - 1);
			if (nodes.Find(node) == -1) nodes.Insert(node);
		}
	}

	//A deny always wins over a grant, as in ZBerryPermissions
	protected static string DescribeNodeState(TStringArray entries, string node) {
		if (entries.Find("-" + node) != -1) return "denied";
		if (entries.Find(node) != -1) return "allowed";
		return "unset";
	}

	protected string CheckHierarchy(ZBerryPermsConfig oldConfig, ZBerryPermsConfig newConfig, int editorLevel) {
		if (editorLevel >= ZBerryPermGroup.SUPERADMIN_LEVEL) return "";

		foreach (ZBerryPermGroup oldGroup : oldConfig.Groups) {
			if (oldGroup.IsEditableBy(editorLevel)) continue;
			if (!oldGroup.IsSameAs(newConfig.FindGroup(oldGroup.Name))) return "your group can't change group " + oldGroup.Name;
		}

		foreach (ZBerryPermGroup newGroup : newConfig.Groups) {
			ZBerryPermGroup existing = oldConfig.FindGroup(newGroup.Name);
			if (existing && !existing.IsEditableBy(editorLevel)) continue; //Checked above, unchanged
			if (!newGroup.IsEditableBy(editorLevel)) return "group " + newGroup.Name + " must have a level below " + editorLevel;
		}

		foreach (ZBerryAdminEntry oldAdmin : oldConfig.Admins) {
			ZBerryPermGroup oldAdminGroup = oldConfig.FindGroup(oldAdmin.Group);
			if (!oldAdminGroup || oldAdminGroup.IsEditableBy(editorLevel)) continue;

			ZBerryAdminEntry kept = newConfig.FindAdmin(oldAdmin.Id);
			if (!kept || kept.Group != oldAdmin.Group) return "your group can't change admins of group " + oldAdmin.Group;
		}

		foreach (ZBerryAdminEntry newAdmin : newConfig.Admins) {
			ZBerryPermGroup newAdminGroup = newConfig.FindGroup(newAdmin.Group);
			if (newAdminGroup.IsEditableBy(editorLevel)) continue;

			ZBerryAdminEntry before = oldConfig.FindAdmin(newAdmin.Id);
			if (!before || before.Group != newAdmin.Group) return "your group can't add admins to group " + newAdmin.Group;
		}

		return "";
	}

	//For tests and tooling
	ZBerryPermsConfig GetConfigData() {
		return m_Config;
	}

	string GetSetupToken() {
		return m_SetupToken;
	}
};

//What AuthState tells a client
class ZBerryAuthState {
	static const int NONE = 0;
	static const int ADMIN = 1;
	static const int SETUP_PENDING = 2;
};
