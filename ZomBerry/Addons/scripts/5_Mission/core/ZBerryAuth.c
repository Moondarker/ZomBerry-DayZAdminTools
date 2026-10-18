//Server side of admin rights: permission groups and admins from ZomBerryPermissions.json, the first-startup token,
//and the permission editing RPCs. Clients only ever get told their own rights (AuthState), the server decides everything
class ZBerryAuth {
	static const string CONFIG_FILE = "ZomBerryPermissions.json";
	static const string SUPERADMIN_GROUP = "superadmin";
	static const int MAX_TOKEN_ATTEMPTS = 5;

	protected ref ZBerryPermsConfig m_Config = new ZBerryPermsConfig;
	protected string m_ConfigPath;
	protected string m_SetupToken = ""; //Set while no admin exists
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
		ZBerryPermGroup superadmin = new ZBerryPermGroup(SUPERADMIN_GROUP);
		superadmin.Permissions.Insert("*");
		superadmin.SpawnAllowed.Insert("*");
		m_Config.Groups.Insert(superadmin);

		ZBerryPermGroup moderator = new ZBerryPermGroup("moderator");
		moderator.Permissions.Insert(ZBerryPermissions.PLAYERS_LIST);
		moderator.Permissions.Insert("zomberry.map.*");
		moderator.Permissions.Insert("zomberry.functions.TPToTarget");
		moderator.Permissions.Insert("zomberry.functions.TPToAdmin");
		moderator.Permissions.Insert("zomberry.functions.HealTarget");
		m_Config.Groups.Insert(moderator);

		TStringArray legacyIds = ReadLegacyAdmins();
		foreach (string legacyId : legacyIds) {
			if (!m_Config.FindAdmin(legacyId)) m_Config.Admins.Insert(new ZBerryAdminEntry(legacyId, "", SUPERADMIN_GROUP));
		}
		if (m_Config.Admins.Count() > 0) ZomberryBase.Log( "ZomBerryAuth", "INFO: Imported " + m_Config.Admins.Count() + " admins from admins.cfg into the " + SUPERADMIN_GROUP + " group" );

		SaveConfig();
		ZomberryBase.Log( "ZomBerryAuth", "INFO: Created " + CONFIG_FILE );
	}

	//admins.cfg of earlier versions: one id per line. The ids those versions wrote into a new file by themselves
	//(the author's own, as an example) are skipped, so nobody becomes admin on a server that never chose them
	protected TStringArray ReadLegacyAdmins() {
		TStringArray ids = new TStringArray;
		TStringArray seededIds = {"76561198038543835", "YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54="};
		TStringArray paths = {"$profile:ZomBerry\\admins.cfg", "$CurrentDir:admins.cfg"};

		foreach (string path : paths) {
			if (!FileExist(path)) continue;

			FileHandle file = OpenFile(path, FileMode.READ);
			if (file == 0) continue;

			string line;
			while (FGets(file, line) >= 0) {
				line = line.Trim();
				if (seededIds.Find(line) != -1) {
					ZomberryBase.Log( "ZomBerryAuth", "INFO: Skipped the example id earlier versions wrote into " + path + " by default" );
				} else if (line != "" && line.IndexOf("//") != 0) {
					ids.Insert(line);
				}
				line = "";
			}
			CloseFile(file);
			ZomberryBase.Log( "ZomBerryAuth", "INFO: Found legacy admins file " + path );
			break;
		}

		return ids;
	}

	protected void SaveConfig() {
		string saveError;
		if (!JsonFileLoader<ZBerryPermsConfig>.SaveFile(m_ConfigPath, m_Config, saveError)) {
			ZomberryBase.Log( "ZomBerryAuth", "ERROR: Can't save " + CONFIG_FILE + ": " + saveError );
		}
	}

	protected void GenerateSetupToken() {
		string alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; //No O/0 and I/1 to mix up
		Math.Randomize(-1);

		m_SetupToken = "";
		for (int i = 0; i < 12; ++i) {
			if (i > 0 && i % 4 == 0) m_SetupToken += "-";
			m_SetupToken += alphabet.Get(Math.RandomInt(0, alphabet.Length()));
		}

		string notice = "No ZomBerry admins configured. Join the server, press the admin menu key (O) and enter this setup token to become superadmin: " + m_SetupToken;
		Print("[ZomBerryAuth] " + notice);
		ZomberryBase.Log( "ZomBerryAuth", "WARN: " + notice );
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
		GetZBerryRPC().SendRPC( ZBerryRPCOp.AuthState, new Param4<string, int, ref TStringArray, ref TStringArray>(g_zbryVer, state, permissions, spawnRules), true, identity );
	}

	void PushAuthStateToEveryone() {
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);

		foreach (Man player : players) {
			if (player.GetIdentity()) PushAuthState(player.GetIdentity());
		}
	}

	protected bool IsServerCall(ZBerryCallType type, PlayerIdentity sender) {
		return type == ZBerryCallType.Server && GetGame().IsServer() && GetGame().IsMultiplayer() && sender;
	}

	protected string Who(PlayerIdentity sender) {
		return sender.GetName() + " (" + sender.GetPlainId() + ")";
	}

	protected void Tell(PlayerIdentity sender, string message) {
		ZomberryStockFunctions.MessagePlayer(ZBGetPlayerById(sender.GetPlayerId()), message);
	}

	void AdminAuth( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< bool, string > authInfo; //direction (always true), client version
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( authInfo ) ) {
			ZomberryBase.Log( "ZomBerryAuth", "WARN: " + Who(sender) + " - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		if (authInfo.param2 != g_zbryVer) {
			ZomberryBase.Log( "ZomBerryAuth", "WARN: " + Who(sender) + " ZomBerry version mismatch! S: v" + g_zbryVer + ", C: v" + authInfo.param2 );
		}

		ZBerryAdminEntry admin = FindAdmin(sender);
		if (admin && admin.Name != sender.GetName()) {
			admin.Name = sender.GetName();
			SaveConfig();
		}

		if (IsAdmin(sender)) {
			ZomberryBase.Log( "ZomBerryDbg", "Auth respond to admin " + Who(sender) );
		} else if (m_SetupToken != "") {
			ZomberryBase.Log( "ZomBerryAuth", "INFO: " + Who(sender) + " joined while setup is pending, the setup token is in the server script log" );
		}

		PushAuthState(sender);
	}

	void SetupToken( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<string> tokenParam;
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( tokenParam ) ) return;

		string id = sender.GetPlainId();
		if (m_SetupToken == "") {
			ZomberryBase.Log( "ZomBerryAuth", "WARN: " + Who(sender) + " sent a setup token, but setup is already done" );
			PushAuthState(sender);
			return;
		}

		int attempts = m_TokenAttempts.Get(id);
		if (attempts >= MAX_TOKEN_ATTEMPTS) {
			ZomberryBase.Log( "ZomBerryAuth", "WARN: " + Who(sender) + " sent a setup token after " + MAX_TOKEN_ATTEMPTS + " wrong attempts, ignored" );
			Tell(sender, "Too many wrong setup tokens");
			return;
		}

		string token = tokenParam.param1.Trim();
		token.ToUpper();
		if (token != m_SetupToken) {
			m_TokenAttempts.Set(id, attempts + 1);
			ZomberryBase.Log( "ZomBerryAuth", "WARN: " + Who(sender) + " sent a wrong setup token (attempt " + (attempts + 1) + "/" + MAX_TOKEN_ATTEMPTS + ")" );
			Tell(sender, "Wrong setup token");
			return;
		}

		ZBerryPermGroup superadmin = m_Config.FindGroup(SUPERADMIN_GROUP);
		if (!superadmin) {
			superadmin = new ZBerryPermGroup(SUPERADMIN_GROUP);
			superadmin.SpawnAllowed.Insert("*");
			m_Config.Groups.Insert(superadmin);
		}
		if (superadmin.Permissions.Find("*") == -1) superadmin.Permissions.Insert("*");

		m_Config.Admins.Insert(new ZBerryAdminEntry(id, sender.GetName(), SUPERADMIN_GROUP));
		SaveConfig();
		m_SetupToken = "";
		m_TokenAttempts.Clear();

		ZomberryBase.Log( "ZomBerryAuth", "INFO: " + Who(sender) + " entered the setup token and is now in the " + SUPERADMIN_GROUP + " group" );
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

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man player : players) {
			PlayerIdentity identity = player.GetIdentity();
			if (identity) snapshot.Online.Insert(new ZBerryOnlineIdentity(identity.GetPlayerId(), identity.GetPlainId(), identity.GetName()));
		}

		return snapshot;
	}

	void SendSnapshot(PlayerIdentity recipient) {
		ZBerryPermsSnapshot snapshot = BuildSnapshot();
		GetZBerryRPC().SendRPC( ZBerryRPCOp.PermsData, new Param1<ref ZBerryPermsSnapshot>(snapshot), true, recipient );
	}

	void PermsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (!IsServerCall(type, sender)) return;
		if (!HasPermission(sender, ZBerryPermissions.PERMISSIONS_MANAGE)) {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to read permissions, but IS NOT ALLOWED" );
			return;
		}

		SendSnapshot(sender);
	}

	void PermsUpdate( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryPermsSnapshot> update;
		if (!IsServerCall(type, sender)) return;
		if ( !ctx.Read( update ) || !update.param1 ) return;

		if (!HasPermission(sender, ZBerryPermissions.PERMISSIONS_MANAGE)) {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " tried to change permissions, but IS NOT ALLOWED" );
			return;
		}

		string problem = ApplyUpdate(update.param1);
		if (problem != "") {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + Who(sender) + " permission change rejected: " + problem );
			Tell(sender, "Permissions not saved: " + problem);
			SendSnapshot(sender);
			return;
		}

		ZomberryBase.Log( "ZomBerryAdmin", Who(sender) + " changed permissions: " + m_Config.Groups.Count() + " groups, " + m_Config.Admins.Count() + " admins" );
		Tell(sender, "Permissions saved");
		PushAuthStateToEveryone();
		SendSnapshot(sender);
	}

	//Checks and stores a new set of groups and admins. Returns why it was rejected, "" when saved
	string ApplyUpdate(ZBerryPermsSnapshot update) {
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

		m_Config = newConfig;
		SaveConfig();
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
