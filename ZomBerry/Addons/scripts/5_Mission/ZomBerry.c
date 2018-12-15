static string g_zbryVer = "0.5.0";

class ZomberryBase {
	protected bool isAdmin = false;
	protected autoptr TStringArray adminList = new TStringArray;
	ref ZomberryStockFunctions m_ZomberryStockFunctions;
	static ref ZomberryConfig m_ZomberryConfig;
	static ref ZomberryLogger m_ZomberryLogger;

	void ZomberryBase() {
		m_ZomberryStockFunctions = new ref ZomberryStockFunctions;

		if (GetGame().IsServer() || !GetGame().IsMultiplayer()) m_ZomberryStockFunctions.Init();

		if (GetConfig().GetDebugLvl() >= 2) {
			GetZomberryCmdAPI().Debug();
		}

		GetRPCManager().AddRPC( "ZomBerryAT", "AdminAuth", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncPlayersRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctionsRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "ExecuteCommand", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SpawnObject", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "MapTeleport", this, SingeplayerExecutionType.Client );
	}

	static ref ZomberryConfig GetConfig() {
		if ( !m_ZomberryConfig ) {
			m_ZomberryConfig = new ref ZomberryConfig;
		}

		return m_ZomberryConfig;
	}

	static ref ZomberryLogger GetLogger() {
		if ( !m_ZomberryLogger ) {
			m_ZomberryLogger = new ref ZomberryLogger;
		}

		return m_ZomberryLogger;
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
		adminList = GetConfig().ConfigureAdmins();
	}

	bool IsAdmin(string plyUID = "") {
		string tmpStr = "";
		if (!GetGame().IsMultiplayer() || GetGame().IsClient()) return isAdmin;
		if (adminList.Find(plyUID) != -1) return true;

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
			if (IsAdmin(sender.GetId())) {
				GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param2< bool, string >( false, g_zbryVer ), true, sender );
				Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
				if (authInfo.param2 != g_zbryVer) {
					Log( "ZomBerryAT", "WARN: Admin " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! S: v" + g_zbryVer + ", C: v" + authInfo.param2 + ", clientside may not start!");
				}
			} else {
				Log( "ZomBerryDbg", "Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!authInfo.param1 || GetGame().IsMultiplayer()) {
				Log( "ZomBerryDbg", "Auth Respond received" );
				if (authInfo.param2.Substring(0, 3) != g_zbryVer.Substring(0, 3)) {
					Log( "ZomBerryAT", "ERROR: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2 + ", clientside won't start!");
				} else {
					if (authInfo.param2 != g_zbryVer) Log( "ZomBerryAT", "WARN: ZomBerry version mismatch! C: v" + g_zbryVer + ", S: v" + authInfo.param2);
					isAdmin = true;
				}
			} else {
				Log( "ZomBerryDbg", "Auth ignored, singleplayer" );
				isAdmin = true;
			}
			GetConfig();
		}
	}

	void SyncPlayersRequest( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		ref ZBerryPlayer plyData;
		ref ZBerryPlayerArray playerListS = new ZBerryPlayerArray;
		array<Man> players = new array<Man>;
		PlayerBase player;
		int plyId;
		string plyName;
		bool plyAdmin;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender.GetId())) {
				GetGame().GetPlayers(players);

				for (int i = 0; i < players.Count(); ++i) {
					player = PlayerBase.Cast(players.Get(i));
					plyId = player.GetIdentity().GetPlayerId();
					plyName = player.GetIdentity().GetName();
					plyAdmin = IsAdmin(player.GetIdentity().GetId());

					if (player.GetItemInHands() && !player.GetCommand_Vehicle()) {plyName += (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}
					if (player.GetCommand_Vehicle()) {plyName += (" [" + player.GetCommand_Vehicle().GetTransport().GetDisplayName() + "]")}

					plyData = new ZBerryPlayer(plyId, plyName, plyAdmin, player.GetPosition());
					playerListS.Insert(plyData);
				}

				GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, sender );
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync");

			} else {
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync denied (not an admin)" );
			}
		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			plyData = new ZBerryPlayer(0, "Player" + plyName, true, player.GetPosition());

			playerListS.Insert(plyData);
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param2<ref ZBerryPlayerArray, int> (playerListS, GetGame().GetTime()), true, NULL );
			Log( "ZomBerryDbg", "Player list sync singleplayer");
		}
	}

	void SyncFunctionsRequest( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender.GetId())) {
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
		if ( type == CallType.Server && GetGame().IsServer() ) {
			PlayerIdentity targetIdent = ZBGetPlayerById(targetId).GetIdentity();
			if (IsAdmin(sender.GetId())) {
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName(), NULL, oldFuncParam );
				if (targetId != funcParam.param2) {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName() + " on target " + targetIdent.GetName()  + " (" + targetIdent.GetId() + ")");
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName());
				}
			} else {
				Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute " + GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName() + " but IS NOT ADMIN");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				Log( "ZomBerryAdmin", "Executed " + funcParam.param1 + " (singleplayer)");
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), GetZomberryCmdAPI().GetFunc(funcParam.param1).GetName(), NULL, funcParam );
			}
		}
	}

	void SpawnObject( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		//string objName, int adminId, vector targetPlace, bool insideInventory
		Param4< string, int, vector, bool > tgtParam;
		ItemBase item;

		if ( !ctx.Read( tgtParam ) ) return;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (IsAdmin(sender.GetId())) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					item.SetQuantity(item.GetQuantityMax());
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") added " + tgtParam.param1 + " to theirs inventory");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") spawned " + tgtParam.param1 + " at " + tgtParam.param3);
				}
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to spawn " + tgtParam.param1 + " (NOT AN ADMIN)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					item.SetQuantity(item.GetQuantityMax());
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
			if (IsAdmin(sender.GetId())) {
				if (!adminPly.GetCommand_Vehicle()) {
					adminPly.SetPosition(reqpos);
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") teleported to position " + reqpos.ToString());
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport, but was in vehicle");
				}
			} else {
				Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to teleport (NOT AN ADMIN)");
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

modded class MissionServer {
	ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Loaded Server side v" + g_zbryVer );
	}

	private ref ZomberryBase GetZomberryBase() {
		if ( !m_ZomberryBase ) {
			m_ZomberryBase = new ref ZomberryBase;
		}

		return m_ZomberryBase;
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};

modded class MissionGameplay {
	ref ZomberryBase m_ZomberryBase;
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {

		ZomberryBase.Log( "ZomBerry", "Loaded Client side v" + g_zbryVer );
	}

	private ref ZomberryBase GetZomberryBase() {
		if ( !m_ZomberryBase ) {
			m_ZomberryBase = new ref ZomberryBase;
		}

		return m_ZomberryBase;
	}

	private ref ZomberryMenu GetZomberryMenu() {
		if ( !m_ZomberryMenu ) {
			m_ZomberryMenu = new ref ZomberryMenu;
			m_ZomberryMenu.Init();
			GetRPCManager().AddRPC( "ZomBerryAT", "SyncPlayers", m_ZomberryMenu, SingeplayerExecutionType.Client );
			GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctions", m_ZomberryMenu, SingeplayerExecutionType.Client );
		}

		return m_ZomberryMenu;
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		GetZomberryBase().OnClientReady();
	}

	override void OnKeyPress( int key ) {
		super.OnKeyPress( key );
		UIManager UIMgr = GetGame().GetUIManager();

		switch ( key ) {
			case ZomberryBase.GetConfig().GetMenuKey(): {
				if (GetZomberryMenu().GetLayoutRoot().IsVisible()) { //TODO Fix: Might become NULL
					UIMgr.HideScriptedMenu( GetZomberryMenu() );
				} else if (!UIMgr.IsMenuOpen(MENU_INGAME) && !UIMgr.IsMenuOpen(MENU_INVENTORY) && !UIMgr.IsMenuOpen(MENU_CHAT_INPUT) && GetZomberryBase().IsAdmin()) {
					UIMgr.ShowScriptedMenu( GetZomberryMenu() , NULL );
				}
				break;
			}

			case KeyCode.KC_ESCAPE: {
				if (GetZomberryMenu().GetLayoutRoot().IsVisible()) {
					UIMgr.HideScriptedMenu( GetZomberryMenu() );
				}
				break;
			}
		}
	}
};
