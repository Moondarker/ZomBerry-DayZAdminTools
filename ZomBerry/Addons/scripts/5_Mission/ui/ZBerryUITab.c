//A menu page with its own tab button. The header action button (Bind/Safe/WIP) is shared and owned by the menu,
//each tab decides what it shows and does while the tab is active
class ZBerryUITab extends ZBerryUIComponent {
	protected ButtonWidget m_TabButton;
	protected ButtonWidget m_ActionButton;

	//The layout path is passed in by the menu (see LoadLayout for why it is a full literal path)
	void InitTab(Widget parent, string layoutPath, ButtonWidget tabButton, ButtonWidget actionButton) {
		m_TabButton = tabButton;
		m_ActionButton = actionButton;

		if (!LoadLayout(layoutPath, parent)) return;
		OnInit();
	}

	ButtonWidget GetTabButton() {
		return m_TabButton;
	}

	void Activate() {
		Show(true);
		ZBerryUIUtils.SetButtonActive(m_TabButton, true);
		OnActivate();
	}

	void Deactivate() {
		Show(false);
		ZBerryUIUtils.SetButtonActive(m_TabButton, false);
		OnDeactivate();
	}

	protected void SetActionButton(string text, int color) {
		ZBerryUIUtils.SetButtonText(m_ActionButton, text);
		ZBerryUIUtils.SetButtonTextColor(m_ActionButton, color);
	}

	protected void OnInit() {}
	protected void OnActivate() {}
	protected void OnDeactivate() {}

	void OnActionButtonClick() {}
	void OnMenuShow() {}
	void OnMenuHide() {}
	void OnBindKeyPress(int key) {}
	void Update(float tDelta) {}
	void OnPermissionsChanged() {} //Rights from the server changed (see GetZBerryClientAuth())

	//True while the tab needs keyboard input, so the menu hotkey must not close the menu
	bool BlocksClose() {
		return false;
	}
};
