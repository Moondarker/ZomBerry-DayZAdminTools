static string g_zbryVer = "0.1";

class ZomberryBase {
	ref ZomberryCmdAPI m_ZomberryCmdAPI;
	ref ZomberryStockFunctions m_ZomberryStockFunctions;

	protected bool isAdmin = false;
	protected bool zbryDebug = false;
	protected int menuKey = KeyCode.KC_M;
	protected autoptr TStringArray adminList = new TStringArray;

	void ZomberryBase() {
		m_ZomberryCmdAPI = GetZomberryCmdAPI();
		m_ZomberryStockFunctions = new ref ZomberryStockFunctions;

		m_ZomberryStockFunctions.Init();

		GetRPCManager().AddRPC( "ZomBerryAT", "AdminAuth", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncPlayersRequest", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "ExecuteCommand", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "ZomBerryAT", "SpawnObject", this, SingeplayerExecutionType.Client );
	}

	void OnClientReady() {
		Print( "[ZomBerryDbg] Auth Request sent" );

		GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param1< bool >( true ), true, NULL );
	}

	void OnServerReady() {
		string zbry_path;

		if (!GetCLIParam("zbryDir", zbry_path)) {
			zbry_path = "$CurrentDir:\\" + g_Game.GetMissionPath();
			zbry_path.Replace("mission.c", "");
			Print( "[ZomBerryConfig] INFO: Mission directory is: " + zbry_path);
		} else {
			Print( "[ZomBerryConfig] INFO: Will search for admins.cfg in custom directory: " + zbry_path);
		}

		if (!FileExist(zbry_path + "admins.cfg")) {
			string longWarning = "[ZomBerryConfig] INFO: admins.cfg was not found in mission dir OR you're missing some strings in mission init.c\n";
			longWarning += "SCRIPT       : [ZomBerryConfig] INFO: Please add following line in 'Mission CreateCustomMission' section (between { and }, but before whole 'return' line)\n";
			longWarning += "SCRIPT (THIS): g_Game.SetMissionPath(path); //ZomBerry set path line, DO NOT COPY 'SCRIPT (THIS): ' PART!";
			Print ( "" + longWarning );

			if (FileExist("$profile:\\ZomBerry\\admins.cfg")) {
				zbry_path = "$profile:\\ZomBerry\\";
				Print( "[ZomBerryConfig] INFO: Using admins.cfg from Profile directory");
			} else if (FileExist("$CurrentDir:\\ZomBerry\\Config\\admins.cfg")) {
				zbry_path = "$CurrentDir:\\ZomBerry\\Config\\";
				Print( "[ZomBerryConfig] WARN: Using admins.cfg from ZomBerry Addon directory (Better use Profile or Mission dir)" );
			} else {
				Print( "[ZomBerryConfig] FATAL: admins.cfg was NOT found, please check previous logs and read FAQ." );
			}
		}

		Print( "[ZomBerryDbg] Server ready, loading admin list from: " + zbry_path + "admins.cfg");

		FileHandle adminFile = OpenFile(zbry_path + "admins.cfg", FileMode.READ);
		adminList.Insert("YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54=");
		if (adminFile != 0) {
			string sLine = "";
			Print("[ZomBerryConfig] admins.cfg loaded");
			while ( FGets(adminFile,sLine) > 0 ) {
				adminList.Insert(sLine);
				Print("[ZomBerryConfig] Added admin: " + sLine);
			}
			CloseFile(adminFile);
		} else {
			Print("[ZomBerryConfig] FATAL: admins.cfg loading failed");
		}
	}

	bool isAdmin() {
		return isAdmin;
	}

	int getMenuKey() {
		return menuKey;
	}

	void AdminAuth( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param1< bool > toServer;
		if ( !ctx.Read( toServer ) ) return;

		if ( type == CallType.Server ) {
			if (adminList.Find(sender.GetId()) != -1) {
				GetRPCManager().SendRPC( "ZomBerryAT", "AdminAuth", new Param1< bool >( false ), true, sender );
				Print( "[ZomBerryDbg] Auth respond to admin " + sender.GetName() + " (" + sender.GetId() + ")");
			} else {
				Print( "[ZomBerryDbg] Auth Request ignored (not an admin) " + sender.GetName() + " (" + sender.GetId() + ")" );
			}
		} else {
			if (!toServer.param1 || GetGame().IsMultiplayer()) {
				Print( "[ZomBerryDbg] Auth Respond received" );
				isAdmin = true;
			} else {
				Print( "[ZomBerryDbg] Auth ignored, singleplayer" );
				isAdmin = true;
			}
			ConfigureClient();
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

					/*plyData = new ZBerryPlayer(75, "PlayerDummy", false);
					playerListS.Insert(plyData);

					plyData = new ZBerryPlayer(80, "AdminDummy", true);
					playerListS.Insert(plyData);*/
				}

				GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param1<ref ZBerryPlayerArray> (playerListS), true, sender );
				Print( "[ZomBerryDbg] " + sender.GetName() + " (" + sender.GetId() + ") - player list sync");

			} else {
				Print( "[ZomBerryDbg] " + sender.GetName() + " (" + sender.GetId() + ") - player list sync denied (not an admin)" );
			}
		} else {
			player = PlayerBase.Cast(GetGame().GetPlayer());
			if (player.GetItemInHands()) {plyName = (" [" + player.GetItemInHands().GetInventoryItemType().GetName() + "]")}

			plyData = new ZBerryPlayer(0, "Player" + plyName, true);

			playerListS.Insert(plyData);
			GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayers", new Param1<ref ZBerryPlayerArray> (playerListS), true, NULL );
			Print( "[ZomBerryDbg] Player list sync singleplayer");
		}
	}

	void ExecuteCommand( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target ) {
		Param4< string, int, int, vector > funcParam; //Function name, Admin Id, Target Id, Admin cursor
		if ( !ctx.Read( funcParam ) ) return;

		int targetId = funcParam.param3;
		if ( type == CallType.Server && GetGame().IsServer() ) {
			PlayerIdentity targetIdent = ZBGetPlayerById(targetId).GetIdentity();
			if (adminList.Find(sender.GetId()) != -1) {
				GetGame().GameScript.CallFunctionParams( m_ZomberryCmdAPI.GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, funcParam );
				Print( "[ZomBerryAdmin] " + sender.GetName() + " (" + sender.GetId() + ") executed " + funcParam.param1 + " on target " + targetIdent.GetName()  + " (" + targetIdent.GetId() + ")");
			} else {
				Print( "[ZomBerryAdmin] WARN: " + sender.GetName() + " (" + sender.GetId() + ") tried to execute " + funcParam.param1 + " but IS NOT ADMIN");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				Print( "[ZomBerryAdmin] Executed " + funcParam.param1 + " (singleplayer)");
				GetGame().GameScript.CallFunctionParams( m_ZomberryCmdAPI.GetFunc(funcParam.param1).GetInstance(), funcParam.param1, NULL, funcParam );
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
					Print( "[ZomBerryAdmin] " + sender.GetName() + " (" + sender.GetId() + ") added " + tgtParam.param1 + " to theirs inventory");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Print( "[ZomBerryAdmin] " + sender.GetName() + " (" + sender.GetId() + ") spawned " + tgtParam.param1 + " at " + tgtParam.param3);
				}
			} else {
				Print( "[ZomBerryAdmin] " + sender.GetName() + " (" + sender.GetId() + ") tried to spawn " + tgtParam.param1 + " (NOT AN ADMIN)");
			}
		} else {
			if (!GetGame().IsMultiplayer()) {
				if (tgtParam.param4) {
					item = ItemBase.Cast(ZBGetPlayerById(tgtParam.param2).GetInventory().CreateInInventory(tgtParam.param1));
					item.SetQuantity(1);
					Print( "[ZomBerryAdmin] Added " + tgtParam.param1 + " to inventory (singleplayer)");
				} else {
					GetGame().CreateObject(tgtParam.param1, tgtParam.param3, false, true );
					Print( "[ZomBerryAdmin] Spawned " + tgtParam.param1 + " at " + tgtParam.param3 + " (singleplayer)");
				}
			}
		}
	}

	void ConfigureClient() {
		FileHandle configFile;

		if (!FileExist("$saves:\\ZomBerry.cfg")) {
			Print( "[ZomBerryConfig] INFO: config file not found, trying to create new one in $saves:\\ZomBerry.cfg" );
			configFile = OpenFile("$saves:\\ZomBerry.cfg", FileMode.WRITE);

			if (configFile != 0) {
				FPrintln(configFile, "debug = 0;");
				FPrintln(configFile, "menuKey = \"KC_M\";");
				Print( "[ZomBerryConfig] INFO: config file created successfully." );
			} else {
				Print( "[ZomBerryConfig] WARN: unable to create a config file, please make one manually." );
			};
			CloseFile(configFile);
		};

		configFile = OpenFile("$saves:\\ZomBerry.cfg", FileMode.READ);
		if (configFile != 0) {
			string sLine = "";
			int idx = 0;
			TStringArray sParams = new TStringArray;
			while ( FGets(configFile,sLine) > 0 ) {
				++idx;
				sLine.Replace(" ", ""); sLine.Replace(";", "");
				sLine.Split("=", sParams);
				if (sParams.Count() != 2) {
					Print( "[ZomBerryConfig] WARN: config line " + idx.ToString() + " (param " + sParams[0] + ") - parse error (not key = value), " + sParams.Count() + " tokens" );
					sParams = {};
					continue;
				}

				switch (sParams[0]) {
					case "debug": {
						if (sParams[1] == "1") {
							zbryDebug = true;
							Print( "[ZomBerryConfig] INFO: Debug enabled." );
						}
						break;
					}

					case "menuKey": {
						string tempKeyCode;
						for (int i = 0; i < 126; ++i) {
							tempKeyCode = typename.EnumToString(KeyCode, i);
							if (sParams[1] == tempKeyCode) {
								menuKey = i;
								Print( "[ZomBerryConfig] INFO: Menu key set to " + tempKeyCode );
								break;
							}
						}
						break;
					}
				}
				sParams = {};
			}
			CloseFile(configFile);
		};
		/*UAInput zbryMenu = GetUApi().GetInputByName( "UAZomberryOpenMenu" );  //UApi is not ready yet
		if (zbryMenu.BindingCount() == 0) {
			GetUApi().DeRegisterInput( "UAZomberryOpenMenu" );
			zbryMenu = GetUApi().RegisterInput( "UAZomberryOpenMenu", "STR_USRACT_OPEN_ZBRY_MENU", "infantry" );
			zbryMenu.BindCombo( "kK" );
			GetUApi().Export();
			Print ("[ZomBerryDbg] KeyBind not found, created new one.");
		}
		Print (zbryMenu);*/
	}
};

modded class MissionServer {

	void MissionServer() {
		Print( "[ZomBerryDbg] Loaded Server Mission" );
	}

	override void OnInit() {
		super.OnInit();

		GetZomberryBase().OnServerReady();
	}
};

modded class MissionGameplay {

	void MissionGameplay() {

		Print( "[ZomBerryDbg] Loaded Client Mission" );
	}

	override void OnMissionStart() {
		super.OnMissionStart();

		GetZomberryBase().OnClientReady();
	}

	override void OnKeyPress( int key ) {
		super.OnKeyPress( key );
		UIManager UIMgr = GetGame().GetUIManager();

		switch ( key ) {
			case GetZomberryBase().getMenuKey(): {
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
