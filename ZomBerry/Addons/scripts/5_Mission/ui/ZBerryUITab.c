//A menu page with its own tab button. The header action button (Bind/Safe/WIP) is shared and owned by the menu,
//each tab decides what it shows and does while the tab is active
class ZBerryUITab extends ZBerryUIComponent {
	protected ButtonWidget m_TabButton;
	protected ButtonWidget m_ActionButton;

	void InitTab(Widget parent, ButtonWidget tabButton, ButtonWidget actionButton) {
		m_TabButton = tabButton;
		m_ActionButton = actionButton;

		if (!LoadLayout(GetLayoutName(), parent)) return;
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
	}

	protected void SetActionButton(string text, int color) {
		m_ActionButton.SetText(text);
		m_ActionButton.SetTextColor(color);
	}

	protected string GetLayoutName() {
		return "";
	}

	protected void OnInit() {}
	protected void OnActivate() {}

	void OnActionButtonClick() {}
	void OnMenuShow() {}
	void OnMenuHide() {}
	void OnBindKeyPress(int key) {}
	void Update(float tDelta) {}

	//True while the tab needs keyboard input, so the menu hotkey must not close the menu
	bool BlocksClose() {
		return false;
	}
};
