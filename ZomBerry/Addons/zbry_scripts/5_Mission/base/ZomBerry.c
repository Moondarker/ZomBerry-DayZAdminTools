static string g_zbryVer = "1.0.0";

class ZomberryBase {
	private ScriptCallQueue SCQueue;
	protected string remoteZbryVer = g_zbryVer;
	protected bool isAdmin = false;
	protected autoptr TStringArray adminList = new TStringArray;
	protected autoptr array<PlayerIdentity> m_activeAdmins;
	ref ZomberryStockFunctions m_ZomberryStockFunctions;
	static ref ZomberryConfig m_ZomberryConfig;
	static ref ZomberryPermissions m_ZomberryPermissions;
	static ref ZomberryLogger m_ZomberryLogger;
	static ref ZomberryKeybinds m_ZomberryKeybinds;

	void ZomberryBase() {
		m_activeAdmins = new array<PlayerIdentity>;
		m_ZomberryStockFunctions = new ZomberryStockFunctions;
		string tmpStr = "";

		if (!GetConfig().IsDefaultIO()) {
			GetLogger().SwitchToCustomIO();
		}

		if (GetGame().IsServer() || !GetGame().IsMultiplayer()) {
			m_ZomberryStockFunctions.Init();
			SCQueue = GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM);
			SCQueue.CallLater(SyncPlayersList, 5000);
		}

		if (GetConfig().GetDebugLvl() >= 2) {
			GetZomberryCmdAPI().Debug();
		}

		GetRPCManager().AddRPC( "ZomBerryAT", "AdminAuth", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SetActiveAdmin", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctionsRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "ExecuteCommand", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SpawnObject", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "MapTeleport", this, SingeplayerExecutionType.Client );
		//GetRPCManager().AddRPC( "ZomBerryAT", "MiscFunctionData", this, SingeplayerExecutionType.Server );
	}

	static ref ZomberryConfig GetConfig() {
		if ( !m_ZomberryConfig ) {
			m_ZomberryConfig = new ref ZomberryConfig;
		}

		return m_ZomberryConfig;
	}

	static ref ZomberryPermissions GetPermsMgr() {
		if ( !m_ZomberryPermissions ) {
			m_ZomberryPermissions = new ref ZomberryPermissions;
		}

		return m_ZomberryPermissions;
	}

	static ref ZomberryLogger GetLogger() {
		if ( !m_ZomberryLogger ) {
			m_ZomberryLogger = new ref ZomberryLogger;
		}

		return m_ZomberryLogger;
	}

	static ref ZomberryKeybinds GetKeyBindsMgr() {
		if ( !m_ZomberryKeybinds ) {
			m_ZomberryKeybinds = new ref ZomberryKeybinds;
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

		GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param2< bool, string >( true, g_zbryVer ), true, NULL );
	}

	void OnServerReady() {
		//adminList = GetConfig().ConfigureAdmins();
	}

	string GetRemoteVersion() {
		return remoteZbryVer;
	}

	bool IsAdmin(PlayerIdentity plyIdent = NULL) {
		string tmpStr = "";
		if (!GetGame().IsMultiplayer() || GetGame().IsClient()) return isAdmin;
		if (GetPermsMgr().CheckPermission(plyIdent.GetPlainId(), "OpenMenu")) return true;

		if (GetCLIParam("zbryGiveAdminRightsToEveryone", tmpStr)) {
			if (tmpStr == "true") return true;
		}
		return false;
	}

	void AdminAuth( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2< bool, string > authInfo; //direction (true = to server), version (only in client dir)
		if ( !ctx.Read( authInfo ) ) {
			Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		if ( type == CallType.Server ) {
			if (IsAdmin(sender)) {
				GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param2< bool, string >( false, g_zbryVer ), true, sender );
				Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
				if (authInfo.param2 != g_zbryVer) {
					Log( "ZomBerryAT", "WARN: Admin " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! S: v" + g_zbryVer + ", C: v" + authInfo.param2 + ", clientside may not start!");
				}
			} else {
				if (GetPermsMgr().CountAdmins() < 1) GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param2< bool, string >( false, g_zbryVer + "CFGFailed" ), true, sender );
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

	void SetActiveAdmin( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2<bool, bool> isActive;

		if (!ctx.Read( isActive ) || m_activeAdmins.Find(sender) != -1) return;

		if (isActive.param1)
			m_activeAdmins.Insert(sender);
		else
			m_activeAdmins.RemoveItem(sender);

		if (isActive.param2) SyncPlayersList(); //to be changed
	}

	void SyncPlayersList() {
		ref ZBerryPlayerArray playerListS = new ZBerryPlayerArray;
		array<Man> players = new array<Man>;
		PlayerBase player;
		int plyId;
		string plyName;
		vector plyPos;
		bool plyAdmin;

		if (GetGame().IsMultiplayer() && GetGame().IsServer()) {
			GetGame().GetPlayers(players);

			for (int i = 0; i < players.Count(); ++i) {
				player = PlayerBase.Cast(players.Get(i));
				plyId = player.GetIdentity().GetPlayerId();
				plyName = player.GetIdentity().GetName();
				plyAdmin = IsAdmin(player.GetIdentity());
				MiscGameplayFunctions.GetHeadBonePos(player, plyPos);

				if (player.GetItemInHands() && !player.GetCommand_Vehicle()) {plyName += (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}
				if (player.GetCommand_Vehicle()) {plyName += (" [" + player.GetCommand_Vehicle().GetTransport().GetDisplayName() + "]")}

				playerListS.Insert(new ZBerryPlayer(plyId, plyName, plyAdmin, plyPos, player.GetHealth(), player.GetHealth("", "Blood"), player.GetOrientation()));
			}

			foreach(PlayerIdentity admin: m_activeAdmins)
				GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, admin );

		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			MiscGameplayFunctions.GetHeadBonePos(player, plyPos);
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			playerListS.Insert(new ZBerryPlayer(0, "Player" + plyName, true, plyPos, player.GetHealth(), player.GetHealth("", "Blood"), player.GetOrientation()));
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, NULL );
		}

		SCQueue.CallLater(SyncPlayersList, 5000);
	}

	void SyncFunctionsRequest( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender)) {
				catList = GetZomberryCmdAPI().GetList();

				GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctions", new Param1<ref ZBerryCategoryArray> (catList), true, sender );
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync");

			} else {
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync denied (not an admin)" );
			}
		} else {
			catList = GetZomberryCmdAPI().GetList();

			GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctions", new Param1<ref ZBerryCategoryArray> (catList), true, NULL );
			Log( "ZomBerryDbg", "Function list sync singleplayer");
		}
	}

	void ExecuteCommand( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param5< int, int, int, vector, autoptr TIntArray > funcParam; //Function id, Admin Id, Target Id, Admin cursor, Function params
		Param5< string, int, int, vector, autoptr TIntArray > oldFuncParam; //Backwards compatibility
		if ( !ctx.Read( funcParam ) ) return;

		oldFuncParam = new Param5< string, int, int, vector, autoptr TIntArray >(GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName(), funcParam.param2, funcParam.param3, funcParam.param4, funcParam.param5 );

		int targetId = funcParam.param3;
		string fncName = GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName();
		if ( type == CallType.Server && GetGame().IsServer() ) {
			PlayerIdentity targetIdent = ZBGetPlayerById(targetId).GetIdentity();
			if (GetPermsMgr().CheckPermission(sender.GetPlainId(), fncName)) {
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), fncName, NULL, oldFuncParam );
				if (targetId != funcParam.param2) {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + fncName + " on target " + targetIdent.GetName()  + " (" + targetIdent.GetId() + ")");
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + fncName);
				}
			} else {
				Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute " + fncName + " (But had no permission to do that)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				Log( "ZomBerryAdmin", "Executed " + fncName + " (singleplayer)");
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), fncName, NULL, oldFuncParam );
			}
		}
	}

	void SpawnObject( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		//string objName, int adminId, vector targetPlace, bool insideInventory
		Param4< string, int, vector, bool > tgtParam;
		ItemBase item;
		string tmpStr = "";

		if (GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") return;
		}

		if ( !ctx.Read( tgtParam ) ) return;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (GetPermsMgr().CheckPermission(sender.GetPlainId(), "SpawnEverything") || GetPermsMgr().CheckPermission(sender.GetPlainId(), "Spawn" + tgtParam.param1)) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					if (item) item.SetQuantity(item.GetQuantityMax());
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") added " + tgtParam.param1 + " to theirs inventory");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") spawned " + tgtParam.param1 + " at " + tgtParam.param3);
				}
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to spawn " + tgtParam.param1 + " (But had no permission to do that)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					if (item) item.SetQuantity(item.GetQuantityMax());
					Log( "ZomBerryAdmin", "Added " + tgtParam.param1 + " to inventory (singleplayer)");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Log( "ZomBerryAdmin", "Spawned " + tgtParam.param1 + " at " + tgtParam.param3 + " (singleplayer)");
				}
			}
		}
	}

	void MapTeleport( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1< vector > teleParam;

		if ( !ctx.Read( teleParam ) ) return;

		float atlZ = GetGame().SurfaceY(teleParam.param1[0], teleParam.param1[2]);
		vector reqpos = Vector(teleParam.param1[0], atlZ, teleParam.param1[2]);
		int adminId = 0;
		if (GetGame().IsMultiplayer()) adminId = sender.GetPlayerId();
		PlayerBase adminPly = ZBGetPlayerById(adminId);

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (GetPermsMgr().CheckPermission(sender.GetPlainId(), "MapTeleport")) {
				if (!adminPly.GetCommand_Vehicle()) {
					adminPly.SetPosition(reqpos);
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") teleported to position " + reqpos.ToString());
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport, but was in vehicle");
				}
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport (But had no permission to do that)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
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
