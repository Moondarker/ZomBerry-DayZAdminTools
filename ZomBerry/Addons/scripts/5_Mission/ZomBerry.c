static string g_zbryVer = "0.7.0";

//Wires ZomBerry together on both sides (server: MissionServer, client and singleplayer: MissionGameplay)
//and keeps the shared config/logger/keybinds. The actual work lives in core/: auth, request handlers, installation mode
class ZomberryBase {
	ref ZomberryStockFunctions m_ZomberryStockFunctions;
	protected ref ZBerryAuth m_Auth;
	protected ref ZBerryServerRPC m_ServerRPC;
	protected ref ZBerryInstallMode m_InstallMode;

	static ref ZomberryConfig m_ZomberryConfig;
	static ref ZomberryLogger m_ZomberryLogger;
	static ref ZomberryKeybinds m_ZomberryKeybinds;

	void ZomberryBase() {
		m_ZomberryStockFunctions = new ZomberryStockFunctions;

		if (!GetConfig().IsDefaultIO()) {
			GetLogger().SwitchToCustomIO();
		}

		m_Auth = new ZBerryAuth;
		m_ServerRPC = new ZBerryServerRPC(m_Auth);

		if (ZBerryInstallMode.IsActive() && GetGame().IsMultiplayer()) {
			m_InstallMode = new ZBerryInstallMode(m_ServerRPC);
			m_InstallMode.RegisterCommands();
		} else if (GetGame().IsServer() || !GetGame().IsMultiplayer()) {
			m_ZomberryStockFunctions.Init();
		}

		if (GetConfig().GetDebugLvl() >= 2) {
			GetZomberryCmdAPI().Debug();
		}
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
		m_Auth.RequestAuth();
	}

	void OnServerReady() {
		m_Auth.LoadAdmins();
	}

	string GetRemoteVersion() {
		return m_Auth.GetRemoteVersion();
	}

	bool IsAdmin(PlayerIdentity plyIdent = NULL) {
		return m_Auth.IsAdmin(plyIdent);
	}
};

modded class MissionServer {
	ref ZomberryBase m_ZomberryBase;

	void MissionServer() {
		ZomberryBase.Log( "ZomBerry", "Starting Server side " + ZBerryVersion.GetFullVersion() );
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

		ZomberryBase.Log( "ZomBerry", "Starting Client side " + ZBerryVersion.GetFullVersion() );
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
			} else if (!ZBerryVersion.IsCompatible(r_zbryVer, g_zbryVer)) {
				DisplayMessage("[ZomBerry]: Admin auth succeeded, but clientside was disabled due to version mismatch. C: " + g_zbryVer + ", S: " + r_zbryVer);
			} else if (r_zbryVer.Contains("CFGFailed")) {
				DisplayMessage("[ZomBerry]: Serverside loaded, but misconfigured. 0 admins in list, please check admins.cfg, script_date_time.log(s) and FAQ");
			}
			if (ZBerryVersion.IsNewer(g_zbryVer, r_zbryVer) && !m_plyWarned) {
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
