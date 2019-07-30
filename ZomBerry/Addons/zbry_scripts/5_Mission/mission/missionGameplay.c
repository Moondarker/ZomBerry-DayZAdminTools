modded class MissionGameplay {
	bool m_plyWarned = false;
	ref ZomberryBase m_ZomberryBase;
	ref ZomberryMenu m_ZomberryMenu;

	void MissionGameplay() {

		ZomberryBase.Log( "ZomBerry", "Starting Client side v" + g_zbryVer );
	}

	void ~MissionGameplay() {

		if (g_ZomberryClient) delete g_ZomberryClient;
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
			GetZomberryClient();
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

		if (GetZomberryMenu().GetLayoutRoot().IsVisible()) {
			GetZomberryMenu().OnKeyPress( key );
		} else if (GetZomberryBase().IsAdmin()) {
			ZomberryBase.GetKeyBindsMgr().OnKeyPress( key );
		}

		switch ( key ) {
			case ZomberryBase.GetConfig().GetMenuKey(): {
				string r_zbryVer = GetZomberryBase().GetRemoteVersion();
				if (!GetZomberryMenu().GetLayoutRoot().IsVisible() && !UIMgr.IsMenuOpen(MENU_INGAME) && !UIMgr.IsMenuOpen(MENU_CHAT_INPUT) && GetZomberryBase().IsAdmin()) {
					UIMgr.HideDialog(); UIMgr.CloseAll();
					UIMgr.ShowScriptedMenu( GetZomberryMenu() , NULL );
				} else if (r_zbryVer.Substring(0, 3) != g_zbryVer.Substring(0, 3)) {
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", "[ZomBerry]: Admin auth succeeded, but clientside was disabled due to version mismatch. C: " + g_zbryVer + ", S: " + r_zbryVer, ""));
				} else if (r_zbryVer.Contains("CFGFailed")) {
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", "[ZomBerry]: Serverside loaded, but misconfigured. 0 admins in list, please check admins.cfg, script_date_time.log(s) and FAQ", ""));
				}
				if (g_zbryVer.Substring(2, 3).ToFloat() > r_zbryVer.Substring(2, 3).ToFloat() && !m_plyWarned) {
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", "[ZomBerry] INFO: Don't forget to update your server to v" + g_zbryVer + "+ to get latest features!", ""));
					GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "", "[ZomBerry] INFO: Server is currently running on: v" + r_zbryVer, ""));
					m_plyWarned = true;
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
