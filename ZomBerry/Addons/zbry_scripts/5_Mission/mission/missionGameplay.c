modded class MissionGameplay {
	bool m_plyWarned = false;
	ref ZomberryBase m_ZomberryBase;
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {
		m_ZomberryBase = new ref ZomberryBase;

		ZomberryBase.Log( "ZomBerry", "Starting Client side v" + g_zbryVer );
	}

	void ~MissionGameplay() {

		if (g_ZomberryClient) delete g_ZomberryClient;
		if (m_ZomberryMenu) delete m_ZomberryMenu;
		if (m_ZomberryBase) delete m_ZomberryBase;
	}

	private ref ZomberryMenu GetZomberryMenu() {
		if ( !m_ZomberryMenu ) {
			m_ZomberryMenu = new ref ZomberryMenu;
			m_ZomberryMenu.Init();
			GetZomberryClient();
			GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctions", m_ZomberryMenu, SingeplayerExecutionType.Client );
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
			GetZomberryMenu().OnKeyPress(key);
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

			if (!menu && m_ZomberryBase.IsAdmin()) {
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

		if (GetUApi().GetInputByName("UAUIBack").LocalPress() && GetZomberryMenu().GetCloseClearance()) {
			GetUIManager().HideScriptedMenu( GetZomberryMenu() );
			PlayerControlEnable(false);
		}
	}
};
