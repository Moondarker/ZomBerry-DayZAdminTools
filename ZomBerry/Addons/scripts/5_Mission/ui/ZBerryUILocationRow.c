//One saved location in the Map tab's locations dropdown: its name (click: show it on the map), Go, Player and delete.
//Delete asks for a second click
class ZBerryUILocationRow extends ZBerryUIComponent {
	static const float DELETE_CONFIRM_TIME = 3; //Seconds the second click may take

	protected ButtonWidget m_NameButton, m_GoButton, m_PlayerButton, m_DeleteButton;
	protected ZBerryUIMapTab m_Tab;
	protected string m_Name;
	protected float m_DeleteArmedAt = -1; //Game time of the first click, -1 when not armed

	void Init(Widget parent, ZBerryUIMapTab tab) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/LocationRow.layout", parent)) return;

		m_Tab = tab;
		m_NameButton = ButtonWidget.Cast( BindWidget("LocRowName") );
		m_GoButton = ButtonWidget.Cast( BindWidget("LocRowGo") );
		m_PlayerButton = ButtonWidget.Cast( BindWidget("LocRowPlayer") );
		m_DeleteButton = ButtonWidget.Cast( BindWidget("LocRowDelete") );
	}

	void Setup(string name, bool canGo, bool canMovePlayer, bool canDelete) {
		m_Name = name;
		ZBerryUIUtils.SetButtonText(m_NameButton, " " + name);
		m_GoButton.Show(canGo);
		m_PlayerButton.Show(canMovePlayer);
		m_DeleteButton.Show(canDelete);
		DisarmDelete();
	}

	void Destroy() {
		if (m_Root) m_Root.Unlink();
		m_Root = NULL;
	}

	protected void DisarmDelete() {
		m_DeleteArmedAt = -1;
		ZBerryUIUtils.SetButtonText(m_DeleteButton, "X");
		ZBerryUIUtils.SetButtonTextColor(m_DeleteButton, ZBerryUIUtils.TEXT_DEFAULT);
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_NameButton) {
			m_Tab.ShowLocation(m_Name);
			return true;
		}
		if (w == m_GoButton) {
			m_Tab.TeleportToLocation(m_Name, false);
			return true;
		}
		if (w == m_PlayerButton) {
			m_Tab.TeleportToLocation(m_Name, true);
			return true;
		}
		if (w == m_DeleteButton) {
			float now = GetGame().GetTickTime();
			if (m_DeleteArmedAt >= 0 && now - m_DeleteArmedAt <= DELETE_CONFIRM_TIME) {
				DisarmDelete();
				m_Tab.DeleteLocation(m_Name);
			} else {
				m_DeleteArmedAt = now;
				ZBerryUIUtils.SetButtonText(m_DeleteButton, "?");
				ZBerryUIUtils.SetButtonTextColor(m_DeleteButton, ZBerryUILogsTab.COLOR_REFUSED);
			}
			return true;
		}
		return false;
	}
};
