//Debug preview of the log view, until the menu header gets its tab: F9 with "DebugLevel" 1 or more in the client's
//ZomBerryConfig.json. The admin menu's frame with only the Logs tab and its player pane
class ZomberryLogMenu extends UIScriptedMenu {
	protected ButtonWidget m_ActionButton;
	protected ref ZBerryUILogsTab m_LogsTab;

	override Widget Init() {
		if (layoutRoot) return layoutRoot;

		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/scripts/5_Mission/layouts/AdminDialog.layout" );
		if (!layoutRoot) {
			ZomberryBase.Log( "ZomBerryUI", "ERROR: Can't load AdminDialog.layout" );
			return NULL;
		}

		TStringArray tabButtons = {"FunctionsButton", "SpawnButton", "MapButton", "PermsButton"};
		foreach (string buttonName : tabButtons) {
			Widget tabButton = layoutRoot.FindAnyWidget(buttonName);
			if (tabButton) tabButton.Show(false);
		}

		TextWidget title = TextWidget.Cast( layoutRoot.FindAnyWidget("TitleText") );
		if (title) title.SetText( "Logs (debug preview, F9) | ZomBerry Admin Tools " + ZBerryVersion.GetDisplayVersion() + " by Vaker" );
		m_ActionButton = ButtonWidget.Cast( layoutRoot.FindAnyWidget("BindButton") );

		m_LogsTab = new ZBerryUILogsTab();
		m_LogsTab.SetSideParent(layoutRoot.FindAnyWidget("ContentWrapper"));
		m_LogsTab.InitTab(layoutRoot.FindAnyWidget("SectionWrapper"), "ZomBerry/AT/scripts/5_Mission/layouts/LogsTab.layout", NULL, m_ActionButton);
		m_LogsTab.Activate();

		return layoutRoot;
	}

	override void OnShow() {
		super.OnShow();
		ShowQuickbar(false);
		m_LogsTab.OnMenuShow();
	}

	override void OnHide() {
		super.OnHide();
		ShowQuickbar(true);
		m_LogsTab.OnMenuHide();
	}

	protected void ShowQuickbar(bool show) {
		if (!GetGame().GetMission()) return;

		IngameHud hud = IngameHud.Cast(GetGame().GetMission().GetHud());
		if (hud) hud.ShowQuickbarUI(show && GetGame().GetProfileOption(EDayZProfilesOptions.QUICKBAR));
	}

	override void Update(float timeslice) {
		m_LogsTab.Update(timeslice);
	}

	override bool OnClick( Widget w, int x, int y, int button ) {
		if ( w != m_ActionButton ) return false;

		m_LogsTab.OnActionButtonClick();
		return true;
	}

	bool GetCloseClearance() {
		return !m_LogsTab.BlocksClose();
	}
};
