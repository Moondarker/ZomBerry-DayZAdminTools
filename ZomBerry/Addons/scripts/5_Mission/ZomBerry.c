static string g_zbryVer = "0.2";

class ZomberryBase {
	ref ZomberryStockFunctions m_ZomberryStockFunctions;

	protected bool isAdmin = false;
	protected autoptr TStringArray adminList = new TStringArray;

	void ZomberryBase() {
		m_ZomberryStockFunctions = new ref ZomberryStockFunctions;

		m_ZomberryStockFunctions.Init();

		GetRPCManager().AddRPC( "ZomBerryAT", "AdminAuth", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncPlayersRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctionsRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "ExecuteCommand", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SpawnObject", this, SingeplayerExecutionType.Client );
	}

	void OnClientReady() {
		GetZomberryLogger().Log( "ZomBerryDbg", "Auth Request sent" );

		GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param1< bool >( true ), true, NULL );
	}

	void OnServerReady() {
		adminList = GetZomberryConfig().ConfigureAdmins();
	}

	bool isAdmin() {
		return isAdmin;
	}

	void AdminAuth( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1< bool > toServer;
		if ( !ctx.Read( toServer ) ) return;

		if ( type == CallType.Server ) {
			if (adminList.Find(sender.GetId()) != -1) {
				GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param1< bool >( false ), true, sender );
				GetZomberryLogger().Log( "ZomBerryDbg", "Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
			} else {
				GetZomberryLogger().Log( "ZomBerryDbg", "Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!toServer.param1 || GetGame().IsMultiplayer()) {
				GetZomberryLogger().Log( "ZomBerryDbg", "Auth Respond received" );
				isAdmin = true;
			} else {
				GetZomberryLogger().Log( "ZomBerryDbg", "Auth ignored, singleplayer" );
				isAdmin = true;
			}
			GetZomberryConfig();
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
				GetZomberryLogger().Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync");

			} else {
				GetZomberryLogger().Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - player list sync denied (not an admin)" );
			}
		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			plyData = new ZBerryPlayer(0, "Player" + plyName, true);

			playerListS.Insert(plyData);
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param1<ref ZBerryPlayerArray> (playerListS), true, NULL );
			GetZomberryLogger().Log( "ZomBerryDbg", "Player list sync singleplayer");
		}
	}

	void SyncFunctionsRequest( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		ref ZBerryCategoryArray catList;

		if ( type == CallType.Server && GetGame().IsServer() ) {
			if (adminList.Find(sender.GetId()) != -1) {
				catList = GetZomberryCmdAPI().GetList();

				/*for (int i = 0; i < catList.Count(); ++i) { //Full list breakdown
					ref ZBerryCategory s1 = catList.Get(i);
					s1.Debug();
				}*/

				GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctions", new Param1<ref ZBerryCategoryArray> (catList), true, sender );
				GetZomberryLogger().Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync");

			} else {
				GetZomberryLogger().Log( "ZomBerryDbg", "" + sender.GetName() + " (" + sender.GetId() + ") - function list sync denied (not an admin)" );
			}
		} else {
			catList = GetZomberryCmdAPI().GetList();

			GetRPCManager().SendRPC( "ZomBerryAT", "SyncFunctions", new Param1<ref ZBerryCategoryArray> (catList), true, NULL );
			GetZomberryLogger().Log( "ZomBerryDbg", "Function list sync singleplayer");
		}
	}

	void ExecuteCommand( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param4< string, int, int, vector > funcParam; //Function name, Admin Id, Target Id, Admin cursor
		if ( !ctx.Read( funcParam ) ) return;

		int targetId = funcParam.param3;
		if ( type == CallType.Server && GetGame().IsServer() ) {
			PlayerIdentity targetIdent = ZBGetPlayerById(targetId).GetIdentity();
			if (adminList.Find(sender.GetId()) != -1) {
				GetGame().GameScript.CallFunctionParams( GetZomberryCmdAPI().GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, funcParam );
				GetZomberryLogger().Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") executed " + funcParam.param1 + " on target " + targetIdent.GetName()  + " (" + targetIdent.GetId() + ")");
			} else {
				GetZomberryLogger().Log( "ZomBerryAdmin", "WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute " + funcParam.param1 + " but IS NOT ADMIN");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				GetZomberryLogger().Log( "ZomBerryAdmin", "Executed " + funcParam.param1 + " (singleplayer)");
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
					GetZomberryLogger().Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") added " + tgtParam.param1 + " to theirs inventory");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					GetZomberryLogger().Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") spawned " + tgtParam.param1 + " at " + tgtParam.param3);
				}
			} else {
				GetZomberryLogger().Log( "ZomBerryAdmin", "" + sender.GetName() + " (" + sender.GetId() + ") tried to spawn " + tgtParam.param1 + " (NOT AN ADMIN)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					item.SetQuantity(1);
					GetZomberryLogger().Log( "ZomBerryAdmin", "Added " + tgtParam.param1 + " to inventory (singleplayer)");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					GetZomberryLogger().Log( "ZomBerryAdmin", "Spawned " + tgtParam.param1 + " at " + tgtParam.param3 + " (singleplayer)");
				}
			}
		}
	}
};

modded class MissionServer {

	void MissionServer() {
		GetZomberryLogger().Log( "ZomBerryDbg", "Loaded Server Mission" );
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};

modded class MissionGameplay {

	void MissionGameplay() {

		GetZomberryLogger().Log( "ZomBerryDbg", "Loaded Client Mission" );
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		GetZomberryBase().OnClientReady();
	}

	override void OnKeyPress( int key ) {
		super.OnKeyPress( key );
		UIManager UIMgr = GetGame().GetUIManager();

		switch ( key ) {
			case GetZomberryConfig().GetMenuKey(): {
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
