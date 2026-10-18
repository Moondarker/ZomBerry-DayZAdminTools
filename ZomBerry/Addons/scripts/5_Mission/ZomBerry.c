static string g_zbryVer = "0.6.4";

class ZomberryBase {
	protected string remoteZbryVer = g_zbryVer;
	protected bool isAdmin = false;
	protected int instStage = 1;
	protected autoptr TStringArray adminList = new TStringArray;
	ref ZomberryStockFunctions m_ZomberryStockFunctions;
	static ref ZomberryConfig m_ZomberryConfig;
	static ref ZomberryLogger m_ZomberryLogger;
	static ref ZomberryKeybinds m_ZomberryKeybinds;

	void ZomberryBase() {
		m_ZomberryStockFunctions = new ZomberryStockFunctions;
		string tmpStr = "";

		if (!GetConfig().IsDefaultIO()) {
			GetLogger().SwitchToCustomIO();
		}

		if (GetCLIParam("zbryInstallMode", tmpStr) && GetGame().IsMultiplayer()) {
			if (tmpStr == "true") {
				GetZomberryCmdAPI().AddCategory("ZomBerry Installation Mode", COLOR_RED);
				GetZomberryCmdAPI().AddCommand("List of admins in file", "InstMode", this, "ZomBerry Installation Mode", false, {
					new ZBerryFuncParam("Step", {0, 0, 0,}),
				});
				GetZomberryCmdAPI().AddCommand("Step 1: Check if admins.cfg was found", "InstMode", this, "ZomBerry Installation Mode", false, {
					new ZBerryFuncParam("Step", {1, 1, 1,}),
				});
			}
		} else {
			if (GetGame().IsServer() || !GetGame().IsMultiplayer()) m_ZomberryStockFunctions.Init();
		}

		if (GetConfig().GetDebugLvl() >= 2) {
			GetZomberryCmdAPI().Debug();
		}

		GetZBerryRPC().AddRPC( ZBerryRPCOp.AdminAuth, "AdminAuth", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncPlayersRequest, "SyncPlayersRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctionsRequest, "SyncFunctionsRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.ExecuteCommand, "ExecuteCommand", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.SpawnObject, "SpawnObject", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.MapTeleport, "MapTeleport", this, ZBerrySPExec.Client );
	}

	static ref ZomberryConfig GetConfig() {
		if ( !m_ZomberryConfig ) {
			m_ZomberryConfig = new ZomberryConfig;
		}

		return m_ZomberryConfig;
	}

	static ref ZomberryLogger GetLogger() {
		if ( !m_ZomberryLogger ) {
			m_ZomberryLogger = new ZomberryLogger;
		}

		return m_ZomberryLogger;
	}

	static ref ZomberryKeybinds GetKeyBindsMgr() {
		if ( !m_ZomberryKeybinds ) {
			m_ZomberryKeybinds = new ZomberryKeybinds;
		}

		return m_ZomberryKeybinds;
	}

	static void Log( string module, string toLog ) {
		GetLogger().Log( module, toLog );
	}

	static void DebugLog( int dLvl, string module, string toLog ) {
		GetLogger().DebugLog( dLvl, module, toLog );
	}

	void OnClientReady() {
		Log( "ZomBerryDbg", "Auth Request sent" );

		GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( true, g_zbryVer ), true, NULL );
	}

	void OnServerReady() {
		adminList = GetConfig().ConfigureAdmins();
	}

	string GetRemoteVersion() {
		return remoteZbryVer;
	}

	bool IsAdmin(PlayerIdentity plyIdent = NULL) {
		string tmpStr = "";
		if (!GetGame().IsMultiplayer() || GetGame().IsClient()) return isAdmin;
		if ((adminList.Find(plyIdent.GetId()) != -1) || (adminList.Find(plyIdent.GetPlainId()) != -1)) return true;

		if (GetCLIParam("zbryGiveAdminRightsToEveryone", tmpStr) || GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") return true;
		}
		return false;
	}

	void InstMode( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		int step = fValues[0];
		string admPath = "";
		TStringArray outMessages = {};
		PlayerBase adminPly = ZBGetPlayerById(adminId);

		if (!GetCLIParam("zbryInstallMode", admPath)) {
			if (admPath != "true") return;
		}

		switch (step) {
			case 0:
				outMessages.Insert("List of admins in admins.cfg (if exists):");
				outMessages.InsertAll(GetConfig().ConfigureAdmins());
				break;

			case 1:
				admPath = GetConfig().InstFindAdmins();
				if (admPath == "") {
					outMessages.Insert("admins.cfg was not found");
				} else {
					if (admPath == "$CurrentDir:\\ZomBerry\\Config\\") outMessages.Insert("admins.cfg was found at default mod location, please try to create it in proper location!");
					outMessages.Insert("admins.cfg was found at: " + admPath);
				}

				if (instStage == 1) {
					GetZomberryCmdAPI().AddCommand("Step 2: Try to create admins.cfg in proper location", "InstMode", this, "ZomBerry Installation Mode", false, {
						new ZBerryFuncParam("Step", {2, 2, 2,}),
					});
					if (admPath != "" && admPath != "$CurrentDir:\\ZomBerry\\Config\\") {
						GetZomberryCmdAPI().AddCommand("Step 3: Try to add your BIGUID into file", "InstMode", this, "ZomBerry Installation Mode", false, {
							new ZBerryFuncParam("Step", {3, 3, 3,}),
						});
						++instStage;
					}
					++instStage;
					SyncFunctionsRequest( ZBerryCallType.Server, NULL, adminPly.GetIdentity(), NULL );
				}
				break;

			case 2:
				string stageTwoResult = GetConfig().InstCreateAdmins();
				outMessages.Insert("admins.cfg create result: " + stageTwoResult);

				if (instStage == 2 && (stageTwoResult == "OK" || stageTwoResult == "AlreadyExists")) {
					GetZomberryCmdAPI().AddCommand("Step 3: Try to add your BIGUID into file", "InstMode", this, "ZomBerry Installation Mode", false, {
						new ZBerryFuncParam("Step", {3, 3, 3,}),
					});
					++instStage;
					SyncFunctionsRequest( ZBerryCallType.Server, NULL, adminPly.GetIdentity(), NULL );
				}
				break;

			case 3:
				if (GetConfig().InstInsertAdmin(adminPly.GetIdentity().GetId())) {
					outMessages.Insert("Your BIGUID was successfully added into admins.cfg!");
				} else {
					outMessages.Insert("Can't add your BIGUID into admins.cfg");
				}
				break;
		}

		for (int i = 0; i < outMessages.Count(); i++) {
			Param1<string> param = new Param1<string>( "[ZomBerry]: " + outMessages[i] );
			GetGame().RPCSingleParam(adminPly, ERPCs.RPC_USER_ACTION_MESSAGE, param, true, adminPly.GetIdentity());
		}
	}

	void AdminAuth( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< bool, string > authInfo; //direction (true = to server), version (only in client dir)
		if ( !ctx.Read( authInfo ) ) {
			Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		if ( type == ZBerryCallType.Server ) {
			if (IsAdmin(sender)) {
				GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( false, g_zbryVer ), true, sender );
				Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
				if (authInfo.param2 != g_zbryVer) {
					Log( "ZomBerryAT", "WARN: Admin " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! S: v" + g_zbryVer + ", C: v" + authInfo.param2 + ", clientside may not start!");
				}
			} else {
				if (adminList.Count() <= 1) GetZBerryRPC().SendRPC( ZBerryRPCOp.AdminAuth, new Param2< bool, string >( false, g_zbryVer + "CFGFailed" ), true, sender );
				Log( "ZomBerryDbg", "Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!authInfo.param1 || GetGame().IsMultiplayer()) {
				Log( "ZomBerryDbg", "Auth Respond received" );
				remoteZbryVer = authInfo.param2;
				if (authInfo.param2.Substring(0, 3) != g_zbryVer.Substring(0, 3)) {
					Log( "ZomBerryAT", "ERROR: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2 + ", clientside won't start!");
				} else {
					if (authInfo.param2 != g_zbryVer) Log( "ZomBerryAT", "WARN: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2);
					if (!remoteZbryVer.Contains("CFGFailed")) isAdmin = true;
				}
			} else {
				Log( "ZomBerryDbg", "Auth ignored, singleplayer" );
				isAdmin = true;
			}
			GetConfig();
		}
	}

	void SyncPlayersRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		ref ZBerryPlayer plyData;
		ref ZBerryPlayerArray playerListS = new ZBerryPlayerArray;
		array<Man> players = new array<Man>;
		PlayerBase player;
		int plyId;
		string plyName;
		bool plyAdmin;

		if ( type == ZBerryCallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender)) {
				if (GetCLIParam("zbryInstallMode", plyName)) {
					if (plyName == "true") {
						playerListS.Insert(new ZBerryPlayer(3, "ZomBerry", false, Vector(0,0,0), 0, 0));
						playerListS.Insert(new ZBerryPlayer(3, "Installation Mode", false, Vector(0,0,0), 0, 0));
						playerListS.Insert(new ZBerryPlayer(3, "Active", true, Vector(0,0,0), 0, 0));
						playerListS.Insert(new ZBerryPlayer(3, "No real players displayed!", false, Vector(0,0,0), 0, 0));
						GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncPlayers, new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, sender );

						return;
					}
				}

				GetGame().GetPlayers(players);

				for (int i = 0; i < players.Count(); ++i) {
					player = PlayerBase.Cast(players.Get(i));
					if (!player || !player.GetIdentity()) continue; //Connecting/disconnecting players have no identity

					plyId = player.GetIdentity().GetPlayerId();
					plyName = player.GetIdentity().GetName();
					plyAdmin = IsAdmin(player.GetIdentity());

					if (player.GetItemInHands() && !player.GetCommand_Vehicle()) {plyName += (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}
					if (player.GetCommand_Vehicle()) {plyName += (" [" + player.GetCommand_Vehicle().GetTransport().GetDisplayName() + "]")}

					plyData = new ZBerryPlayer(plyId, plyName, plyAdmin, player.GetPosition(), player.GetHealth(), player.GetHealth("", "Blood"));
					playerListS.Insert(plyData);
				}

				GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncPlayers, new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, sender );
				DebugLog( 1, "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync"); //Every 2 seconds while the map is open

			} else {
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync denied (not an admin)" );
			}
		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			plyData = new ZBerryPlayer(0, "Player" + plyName, true, player.GetPosition(), player.GetHealth(), player.GetHealth("", "Blood"));

			playerListS.Insert(plyData);
			GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncPlayers, new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, NULL );
			Log( "ZomBerryDbg", "Player list sync singleplayer");
		}
	}

	void SyncFunctionsRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		ref ZBerryCategoryArray catList;

		if ( type == ZBerryCallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender)) {
				catList = GetZomberryCmdAPI().GetList();

				GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctions, new Param1<ref ZBerryCategoryArray> (catList), true, sender );
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync");

			} else {
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync denied (not an admin)" );
			}
		} else {
			catList = GetZomberryCmdAPI().GetList();

			GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncFunctions, new Param1<ref ZBerryCategoryArray> (catList), true, NULL );
			Log( "ZomBerryDbg", "Function list sync singleplayer");
		}
	}

	void ExecuteCommand( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param5< int, int, int, vector, autoptr TIntArray > funcParam; //Function id, Admin Id, Target Id, Admin cursor, Function params
		if ( !ctx.Read( funcParam ) ) return;

		//Everything in funcParam comes from the client: check the sender first, then validate before use
		ZBerryFunction cmdFunc = GetZomberryCmdAPI().GetFunc(funcParam.param1);
		int adminId = 0;
		int targetId = funcParam.param3;

		if ( type == ZBerryCallType.Server && GetGame().IsServer() ) {
			if (!IsAdmin(sender)) {
				Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute function #" + funcParam.param1 + " but IS NOT ADMIN");
				return;
			}
			if (!cmdFunc) {
				Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute unknown function #" + funcParam.param1);
				return;
			}

			adminId = sender.GetPlayerId(); //Never trust the admin id sent by the client
			RunFunction(cmdFunc, adminId, targetId, funcParam.param4, funcParam.param5);

			PlayerBase targetPly = ZBGetPlayerById(targetId);
			if (targetId != adminId && targetPly) {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + cmdFunc.GetName() + " on target " + targetPly.GetIdentity().GetName()  + " (" + targetPly.GetIdentity().GetId() + ")");
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + cmdFunc.GetName());
			}
		} else {
			if (!GetGame().IsMultiplayer() && cmdFunc) {
				Log( "ZomBerryAdmin", "Executed " + cmdFunc.GetName() + " (singleplayer)");
				RunFunction(cmdFunc, adminId, targetId, funcParam.param4, funcParam.param5);
			}
		}
	}

	//Calls the function with the (funcName, adminId, targetId, cursor, values) signature
	protected void RunFunction(ZBerryFunction cmdFunc, int adminId, int targetId, vector cursor, TIntArray values) {
		//Keep the array in a local: a temporary passed straight into the Param5 was gone by the time the function ran
		TIntArray callValues = PadFuncValues(values);
		Param5< string, int, int, vector, autoptr TIntArray > callParams = new Param5< string, int, int, vector, autoptr TIntArray >(cmdFunc.GetName(), adminId, targetId, cursor, callValues);

		GetGame().GameScript.CallFunctionParams( cmdFunc.GetInstance(), cmdFunc.GetName(), NULL, callParams );
	}

	//Functions index up to 3 values, a short or missing array would throw inside them
	protected TIntArray PadFuncValues(TIntArray values) {
		TIntArray result = new TIntArray;
		if (values) result.InsertAll(values);
		while (result.Count() < 3) result.Insert(0);

		return result;
	}

	void SpawnObject( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		//string objName, int adminId, vector targetPlace, bool insideInventory
		Param4< string, int, vector, bool > tgtParam;
		string tmpStr = "";

		if (GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") return;
		}

		if ( !ctx.Read( tgtParam ) ) return;

		if ( type == ZBerryCallType.Server && GetGame().IsServer() ) {
			if (!IsAdmin(sender)) {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to spawn " + tgtParam.param1 + " (NOT AN ADMIN)");
				return;
			}

			//The inventory target is always the sender, the admin id sent by the client is ignored
			string result = DoSpawn(tgtParam.param1, ZBGetPlayerById(sender.GetPlayerId()), tgtParam.param3, tgtParam.param4);
			Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") " + result);
		} else {
			if (!GetGame().IsMultiplayer()) {
				Log( "ZomBerryAdmin", DoSpawn(tgtParam.param1, ZBGetPlayerById(0), tgtParam.param3, tgtParam.param4) + " (singleplayer)");
			}
		}
	}

	//Same configs the Spawn tab lists
	protected bool IsSpawnableClass(string className) {
		if (className == "") return false;

		return GetGame().ConfigIsExisting(CFG_VEHICLESPATH + " " + className) || GetGame().ConfigIsExisting(CFG_WEAPONSPATH + " " + className) || GetGame().ConfigIsExisting(CFG_MAGAZINESPATH + " " + className);
	}

	//Checked in config: many items (SaltyMix, helmets...) have no script class of their own to check instead
	protected bool IsCarryableClass(string className) {
		if (GetGame().ConfigIsExisting(CFG_WEAPONSPATH + " " + className) || GetGame().ConfigIsExisting(CFG_MAGAZINESPATH + " " + className)) return true;

		return GetGame().IsKindOf(className, "Inventory_Base");
	}

	//Attachments already land on the weapon by themselves (CreateInInventory picks free attachment slots),
	//magazines need the weapon to load them: do it when the weapon in hands takes this magazine and has none
	protected Magazine TryLoadMagazine(PlayerBase admin, string className) {
		Weapon_Base weapon = Weapon_Base.Cast(admin.GetHumanInventory().GetEntityInHands());
		if (!weapon || weapon.GetMagazine(weapon.GetCurrentMuzzle())) return NULL;

		TStringArray magazines = new TStringArray;
		GetGame().ConfigGetTextArray(CFG_WEAPONSPATH + " " + weapon.GetType() + " magazines", magazines);

		string classNameLower = className;
		classNameLower.ToLower();
		foreach (string magazine : magazines) {
			magazine.ToLower();
			if (magazine == classNameLower) return weapon.SpawnAttachedMagazine(className, WeaponWithAmmoFlags.NONE);
		}

		return NULL;
	}

	//Returns a log line describing what happened
	protected string DoSpawn(string className, PlayerBase admin, vector position, bool insideInventory) {
		if (!IsSpawnableClass(className)) return "tried to spawn unknown class " + className;

		if (insideInventory) {
			if (!admin) return "tried to add " + className + " to inventory, but has no character";

			if (TryLoadMagazine(admin, className)) return "loaded " + className + " into the weapon in theirs hands";

			EntityAI entity = admin.GetInventory().CreateInInventory(className);
			if (!entity) {
				string reason = "it can't be carried";
				if (IsCarryableClass(className)) reason = "no room in inventory";

				ZomberryStockFunctions.MessagePlayer(admin, "Can't add " + className + " to inventory: " + reason);
				return "couldn't add " + className + " to theirs inventory (" + reason + ")";
			}

			ItemBase item = ItemBase.Cast(entity);
			if (item) item.SetQuantity(item.GetQuantityMax());
			return "added " + className + " to theirs inventory";
		}

		GetGame().CreateObject(className, position, false, true );
		return "spawned " + className + " at " + position;
	}

	void MapTeleport( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1< vector > teleParam;

		if ( !ctx.Read( teleParam ) ) return;

		float atlZ = GetGame().SurfaceY(teleParam.param1[0], teleParam.param1[2]);
		vector reqpos = Vector(teleParam.param1[0], atlZ, teleParam.param1[2]);
		int adminId = 0;
		if (GetGame().IsMultiplayer()) adminId = sender.GetPlayerId();
		PlayerBase adminPly = ZBGetPlayerById(adminId);

		if ( type == ZBerryCallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender)) {
				if (!adminPly) {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport, but has no character");
				} else if (!adminPly.GetCommand_Vehicle()) {
					adminPly.SetPosition(reqpos);
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") teleported to position " + reqpos.ToString());
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport, but was in vehicle");
				}
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport (NOT AN ADMIN)");
			}
		} else {
			if (!GetGame().IsMultiplayer() && adminPly) {
				if (!adminPly.GetCommand_Vehicle()) {
					adminPly.SetPosition(reqpos);
					Log( "ZomBerryAdmin", "Teleported to position " + reqpos.ToString());
				} else {
					Log( "ZomBerryAdmin", "Teleport failed, player in vehicle");
				}
			}
		}
	}
};

modded class MissionServer {
	ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Starting Server side v" + g_zbryVer );
	}

	private ref ZomberryBase GetZomberryBase() {
		if ( !m_ZomberryBase ) {
			m_ZomberryBase = new ZomberryBase;
		}

		return m_ZomberryBase;
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};

modded class MissionGameplay {
	bool m_plyWarned = false;
	ref ZomberryBase m_ZomberryBase;
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {
		m_ZomberryBase = new ZomberryBase;

		ZomberryBase.Log( "ZomBerry", "Starting Client side v" + g_zbryVer );
	}

	void ~MissionGameplay() {

		if (m_ZomberryMenu) delete m_ZomberryMenu;
		if (m_ZomberryBase) delete m_ZomberryBase;
	}

	private ref ZomberryMenu GetZomberryMenu() {
		if ( !m_ZomberryMenu ) {
			m_ZomberryMenu = new ZomberryMenu;
			m_ZomberryMenu.Init();
			GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncPlayers, "SyncPlayers", m_ZomberryMenu, ZBerrySPExec.Client );
			GetZBerryRPC().AddRPC( ZBerryRPCOp.SyncFunctions, "SyncFunctions", m_ZomberryMenu, ZBerrySPExec.Client );
		}

		return m_ZomberryMenu;
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		m_ZomberryBase.OnClientReady();
	}

	override void OnKeyPress(int key) {
		super.OnKeyPress(key);

		if (GetZomberryMenu().GetLayoutRoot().IsVisible()) {
			GetZomberryMenu().OnBindKeyPress(key);
		} else if (m_ZomberryBase.IsAdmin()) {
			ZomberryBase.GetKeyBindsMgr().OnKeyPress(key);
		}
	}

	void DisplayMessage(string msg) {
		GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", msg, ""));
	}

	override void OnUpdate(float timeslice) {
		super.OnUpdate(timeslice);

		if (GetUApi().GetInputByName("UAZBerryOpenAdminMenu").LocalPress()) {
			string r_zbryVer = m_ZomberryBase.GetRemoteVersion();
			UIScriptedMenu menu = GetUIManager().GetMenu();

			if (!menu && !GetZomberryMenu().GetLayoutRoot().IsVisible() && m_ZomberryBase.IsAdmin()) {
				GetUIManager().ShowScriptedMenu( GetZomberryMenu(), NULL );
				PlayerControlDisable(INPUT_EXCLUDE_ALL);
			} else if (GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
				GetUIManager().HideScriptedMenu( GetZomberryMenu() );
				PlayerControlEnable(false);
			} else if (r_zbryVer.Substring(0, 3) != g_zbryVer.Substring(0, 3)) {
				DisplayMessage("[ZomBerry]: Admin auth succeeded, but clientside was disabled due to version mismatch. C: " + g_zbryVer + ", S: " + r_zbryVer);
			} else if (r_zbryVer.Contains("CFGFailed")) {
				DisplayMessage("[ZomBerry]: Serverside loaded, but misconfigured. 0 admins in list, please check admins.cfg, script_date_time.log(s) and FAQ");
			}
			if (g_zbryVer.Substring(2, 3).ToFloat() > r_zbryVer.Substring(2, 3).ToFloat() && !m_plyWarned) {
				DisplayMessage("[ZomBerry] INFO: Don't forget to update your server to v" + g_zbryVer + "+ to get latest features!");
				DisplayMessage("[ZomBerry] INFO: Server is currently running on: v" + r_zbryVer);
				m_plyWarned = true;
			}
		}

		if (GetUApi().GetInputByName("UAUIBack").LocalPress() && GetZomberryMenu().GetLayoutRoot().IsVisible() && GetZomberryMenu().GetCloseClearance()) {
			GetUIManager().HideScriptedMenu( GetZomberryMenu() );
			PlayerControlEnable(false);
		}
	}
};
