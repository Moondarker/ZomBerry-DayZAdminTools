static string g_zbryVer = "0.3";

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

	bool isAdmin() {
		return isAdmin;
	}

	void AdminAuth( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param2< bool, string > authInfo; //direction (true = to server), version (only in client dir)
		if ( !ctx.Read( authInfo ) ) {
			Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") - Auth error, please update client to latest version (v" + g_zbryVer + "+)!");
			return;
		}

		if ( type == CallType.Server ) {
			if (adminList.Find(sender.GetId()) != -1) {
				GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param2< bool, string >( false, g_zbryVer ), true, sender );
				Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
				if (authInfo.param2 != g_zbryVer) {
					Log( "ZomBerryAT", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! S: " + g_zbryVer + ", C: " + authInfo.param2 + ", clientside won't start!");
				}
			} else {
				Log( "ZomBerryDbg", "Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!authInfo.param1 || GetGame().IsMultiplayer()) {
				Log( "ZomBerryDbg", "Auth Respond received" );
				if (authInfo.param2 != g_zbryVer) {
					Log( "ZomBerryAT", "ERROR: " + sender.GetName() + " (" + sender.GetId() + ") ZomBerry version mismatch! C: " + g_zbryVer + ", S: " + authInfo.param2 + ", clientside won't start!");
				} else {
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
			if (adminList.Find(sender.GetId()) != -1) {
				GetGame().GetPlayers(players);

				for (int i = 0; i < players.Count(); ++i) {
					player = PlayerBase.Cast(players.Get(i));
					plyId = player.GetIdentity().GetPlayerId();
					plyName = player.GetIdentity().GetName();
					plyAdmin = (adminList.Find(player.GetIdentity().GetId()) != -1);

					if (player.GetItemInHands()) {plyName += (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

					plyData = new ZBerryPlayer(plyId, plyName, plyAdmin);
					playerListS.Insert(plyData);
				}

				GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param1<ref ZBerryPlayerArray> (playerListS), true, sender );
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync");

			} else {
				Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync denied (not an admin)" );
			}
		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			plyData = new ZBerryPlayer(0, "Player" + plyName, true);

			playerListS.Insert(plyData);
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param1<ref ZBerryPlayerArray> (playerListS), true, NULL );
			Log( "ZomBerryDbg", "Player list sync singleplayer");
		}
	}

	void SyncFunctionsRequest( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (adminList.Find(sender.GetId()) != -1) {
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
		Param5< string, int, int, vector, ref TIntArray > funcParam; //Function name, Admin Id, Target Id, Admin cursor, Function params
		Param4< string, int, int, vector > altFuncParam; //Backwards compatibility
		if ( !ctx.Read( funcParam ) ) return;

		int targetId = funcParam.param3;
		if ( type == CallType.Server && GetGame().IsServer() ) {
			PlayerIdentity targetIdent = ZBGetPlayerById(targetId).GetIdentity();
			if (adminList.Find(sender.GetId()) != -1) {
				/*if (funcParam.param5.Count() != 0) {
					GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, funcParam );
				} else {*/
					altFuncParam = new Param4< string, int, int, vector >(funcParam.param1, funcParam.param2, targetId, funcParam.param4);
					GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, altFuncParam );
				//}
				if (targetId != funcParam.param2) {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + funcParam.param1 + " on target " + targetIdent.GetName()  + " (" + targetIdent.GetId() + ")");
				} else {
					Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + funcParam.param1);
				}
			} else {
				Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute " + funcParam.param1 + " but IS NOT ADMIN");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				Log( "ZomBerryAdmin", "Executed " + funcParam.param1 + " (singleplayer)");
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, funcParam );
			}
		}
	}

	void SpawnObject( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		//string objName, int adminId, vector targetPlace
		Param4< string, int, vector, bool > tgtParam;
		ItemBase item;

		if ( !ctx.Read( tgtParam ) ) return;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (adminList.Find(sender.GetId()) != -1) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					item.SetQuantity(1);
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
					item.SetQuantity(1);
					Log( "ZomBerryAdmin", "Added " + tgtParam.param1 + " to inventory (singleplayer)");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Log( "ZomBerryAdmin", "Spawned " + tgtParam.param1 + " at " + tgtParam.param3 + " (singleplayer)");
				}
			}
		}
	}
};

modded class MissionServer {

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Loaded Server side v" + g_zbryVer );
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};

modded class MissionGameplay {
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {

		ZomberryBase.Log( "ZomBerry", "Loaded Client side v" + g_zbryVer );
	}

	ref ZomberryMenu GetZomberryMenu() {
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
				} else if (!UIMgr.IsMenuOpen(MENU_INGAME) && !UIMgr.IsMenuOpen(MENU_INVENTORY) && !UIMgr.IsMenuOpen(MENU_CHAT_INPUT) && GetZomberryBase().isAdmin()) {
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
