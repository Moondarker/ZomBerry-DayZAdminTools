//Dev builds only (F9 with the menu open): the same widgets with different text settings side by side,
//to see which ones scale with the resolution. Temporary, removed once the menu's text settings are chosen
class ZBerryUIScaleLab extends ZBerryUIComponent {
	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/ScaleLab.layout", parent)) return;

		for (int list = 1; list <= 4; ++list) {
			TextListboxWidget listbox = TextListboxWidget.Cast( m_Root.FindAnyWidget("LabList" + list) );
			if (!listbox) continue;

			TStringArray rows = {"AKM [AKM]", "Apple [Apple]", "Mag_AKM_30Rnd [AKM 30rnd Mag]", "Fireplace [Fireplace]", "ZmbM_HikerSkinny_Blue", "OffroadHatchback [Ada 4x4]"};
			foreach (string row : rows) listbox.AddItem(row, NULL, 0);
		}

		for (int edit = 1; edit <= 4; ++edit) {
			EditBoxWidget editbox = EditBoxWidget.Cast( m_Root.FindAnyWidget("LabEdit" + edit) );
			if (editbox) editbox.SetText("FireplaceBase");
		}

		Show(false);
	}

	void Toggle(Widget menuRoot) {
		Show(!IsVisible());
		if (IsVisible()) UpdateInfo(menuRoot);
	}

	//What the engine does with exact units: the menu is 970 units wide
	protected void UpdateInfo(Widget menuRoot) {
		int screenWidth, screenHeight;
		GetScreenSize(screenWidth, screenHeight);

		float menuWidth, menuHeight;
		menuRoot.GetScreenSize(menuWidth, menuHeight);

		string info = "Screen " + screenWidth + "x" + screenHeight + " | menu 970 units = " + menuWidth + " px, engine scale " + (menuWidth / 970) + " | height/1080 = " + (screenHeight / 1080.0);
		TextWidget.Cast( m_Root.FindAnyWidget("LabInfo") ).SetText(info);
		ZomberryBase.Log( "ZomBerryUI", "Scale lab: " + info );
	}
};
