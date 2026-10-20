//Menu shell: title, tab switching, shared header button and RPC routing. All page logic lives in 5_Mission/ui components
class ZomberryMenu extends UIScriptedMenu {
	protected TextWidget m_TxtTitle;
	protected ButtonWidget m_ActionButton;

	protected ref ZBerryUIPlayersPanel m_PlayersPanel;
	protected ref ZBerryUIFunctionsTab m_FunctionsTab;
	protected ref ZBerryUISpawnTab m_SpawnTab;
	protected ref ZBerryUIMapTab m_MapTab;
	protected ref ZBerryUIPermsTab m_PermsTab;
	protected ref ZBerryUIKickBanDialog m_KickBanDialog;

	protected ref array<ZBerryUITab> m_Tabs = new array<ZBerryUITab>;
	protected ZBerryUITab m_ActiveTab;

	override Widget Init() {
		if (layoutRoot) return layoutRoot;

		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/scripts/5_Mission/layouts/AdminDialog.layout" );
		if (!layoutRoot) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load AdminDialog.layout" );
			return NULL;
		}

		m_TxtTitle = TextWidget.Cast( layoutRoot.FindAnyWidget("TitleText") );
		m_ActionButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("BindButton") );

		Widget contentWrapper = layoutRoot.FindAnyWidget("ContentWrapper");
		Widget sectionWrapper = layoutRoot.FindAnyWidget("SectionWrapper");

		m_PlayersPanel = new ZBerryUIPlayersPanel();
		m_PlayersPanel.Init(contentWrapper);

		m_FunctionsTab = new ZBerryUIFunctionsTab();
		m_FunctionsTab.SetPlayersPanel(m_PlayersPanel);
		AddTab(m_FunctionsTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/FunctionsTab.layout", "FunctionsButton");

		m_SpawnTab = new ZBerryUISpawnTab();
		m_SpawnTab.SetPlayersPanel(m_PlayersPanel);
		AddTab(m_SpawnTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/SpawnTab.layout", "SpawnButton");

		m_MapTab = new ZBerryUIMapTab();
		AddTab(m_MapTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/MapTab.layout", "MapButton");

		m_PermsTab = new ZBerryUIPermsTab();
		AddTab(m_PermsTab, sectionWrapper, "ZomBerry/AT/scripts/5_Mission/layouts/PermsTab.layout", "PermsButton");
		m_PermsTab.SetPlayersPanel(m_PlayersPanel);

		m_KickBanDialog = new ZBerryUIKickBanDialog();
		m_KickBanDialog.Init(layoutRoot); //Last: drawn above everything else
		m_PlayersPanel.SetDialog(m_KickBanDialog);

		SelectTab(m_FunctionsTab);
		ApplyPermissions();
		GetZBerryStreams().OnPlayersChanged.Insert(OnStreamPlayers);

		return layoutRoot;
	}

	void ~ZomberryMenu() {
		if (g_ZBerryStreamClient) g_ZBerryStreamClient.OnPlayersChanged.Remove(OnStreamPlayers);
	}

	//Roster or player states arrived: the list, the map and the title
	protected void OnStreamPlayers() {
		ZBerryStreamClient streams = GetZBerryStreams();
		ZBerryPlayerArray players = streams.BuildPlayers();

		int upTime = streams.GetServerTime() / 1000;
		int hours = upTime / 3600;
		int minutes = (upTime / 60) % 60;
		m_TxtTitle.SetText( "Players in game: " + streams.GetPlayerCount() + " | " + hours + "h " + minutes + "min | ZomBerry Admin Tools " + ZBerryVersion.GetDisplayVersion() + " by Vaker");
		m_PlayersPanel.SetPlayers(players);
		m_MapTab.SetPlayers(players);
	}

	protected void AddTab(ZBerryUITab tab, Widget parent, string layoutPath, string tabButtonName) {
		tab.InitTab(parent, layoutPath, ButtonWidget.Cast( layoutRoot.FindAnyWidget(tabButtonName) ), m_ActionButton);
		m_Tabs.Insert(tab);
	}

	//Tabs the admin may not use are hidden, the server refuses their requests anyway
	protected void ApplyPermissions() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		ShowTabButton(m_SpawnTab, auth.HasAny({ZBerryPermissions.SPAWN_GROUND, ZBerryPermissions.SPAWN_INVENTORY}));
		ShowTabButton(m_MapTab, auth.Has(ZBerryPermissions.MAP_VIEW));
		ShowTabButton(m_PermsTab, auth.CanManagePermissions());

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnPermissionsChanged();
		}
		m_PlayersPanel.OnPermissionsChanged();
	}

	protected void ShowTabButton(ZBerryUITab tab, bool show) {
		if (tab.GetTabButton()) tab.GetTabButton().Show(show);
		if (!show && tab == m_ActiveTab) SelectTab(m_FunctionsTab);
	}

	//The server sent new rights: refresh what depends on them
	void OnPermissionsChanged() {
		if (!layoutRoot) return;

		ApplyPermissions();
		if (layoutRoot.IsVisible()) {
			GetZBerryStreams().Reset(); //The server sends only what the new rights allow
			m_FunctionsTab.OnMenuShow(); //Asks for the (filtered) function list again
		}
	}

	void OnPermsData(ZBerryPermsSnapshot snapshot) {
		if (m_PermsTab) m_PermsTab.SetSnapshot(snapshot);
	}

	//Esc: closes an open dialog first, returns false if there was none
	bool CloseDialogs() {
		if (!m_KickBanDialog || !m_KickBanDialog.IsVisible()) return false;

		m_KickBanDialog.Close();
		return true;
	}

	protected void SelectTab(ZBerryUITab tab) {
		foreach (ZBerryUITab otherTab : m_Tabs) {
			if (otherTab != tab) otherTab.Deactivate();
		}

		m_ActiveTab = tab;
		m_ActiveTab.Activate();
	}

	//Input focus is taken and released by UIScriptedMenu itself (LockControls/UnlockControls in super.OnShow/OnHide).
	//Taking extra game focus here froze the input API while the menu was open: the menu key stayed "held"
	//from the frame the menu opened and UAUIBack never fired, so the menu couldn't be closed
	override void OnShow() {
		super.OnShow();
		ShowQuickbar(false); //It draws above menus and covered the menu's bottom row

		m_TxtTitle.SetText( "Players in game: ... | ZomBerry Admin Tools " + ZBerryVersion.GetDisplayVersion() + " by Vaker" );
		ApplyPermissions();
		m_PlayersPanel.UpdateListScale();
		GetZBerryStreams().SetListShown(true);

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnMenuShow();
		}
	}

	override void OnHide() {
		super.OnHide();
		ShowQuickbar(true);
		m_KickBanDialog.Close();
		GetZBerryStreams().HideAll();

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnMenuHide();
		}
	}

	//Like vanilla's in-game menu: hidden while the menu is open, back afterwards if the player's options show it
	protected void ShowQuickbar(bool show) {
		if (!GetGame().GetMission()) return;

		IngameHud hud = IngameHud.Cast(GetGame().GetMission().GetHud());
		if (hud) hud.ShowQuickbarUI(show && GetGame().GetProfileOption(EDayZProfilesOptions.QUICKBAR));
	}

	override void Update(float timeslice) {
		if (m_ActiveTab) m_ActiveTab.Update(timeslice);
		m_PlayersPanel.Update(timeslice);
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		if ( w == m_ActionButton ) {
			if (m_ActiveTab) m_ActiveTab.OnActionButtonClick();
			return true;
		}

		foreach (ZBerryUITab tab : m_Tabs) {
			if ( w == tab.GetTabButton() ) {
				SelectTab(tab);
				return true;
			}
		}

		return false;
	}

	//Raw key presses forwarded from MissionGameplay while the menu is open (used for keybinding)
	void OnBindKeyPress( int key ) {

		foreach (ZBerryUITab tab : m_Tabs) {
			tab.OnBindKeyPress(key);
		}
	}

	bool GetCloseClearance() {
		if (m_KickBanDialog && m_KickBanDialog.IsVisible()) return false; //Typing a reason

		foreach (ZBerryUITab tab : m_Tabs) {
			if (tab.BlocksClose()) return false;
		}
		return true;
	}

	void SyncFunctions( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryCategoryArray> catListS;

		if ( !(type == ZBerryCallType.Client && GetGame().IsClient() || !GetGame().IsMultiplayer()) ) return;
		if ( !ctx.Read( catListS ) ) return;

		m_FunctionsTab.SetFunctions(catListS.param1);
	}

	void BansData( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<ref ZBerryBanEntryArray, int> bansS; //Bans, server time

		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( bansS ) || !bansS.param1 ) return;

		m_PlayersPanel.SetBans(bansS.param1, bansS.param2);
	}
}
