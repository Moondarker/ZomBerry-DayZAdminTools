class ZBerryESPIcon {
	protected Widget root;

	protected ImageWidget m_Icon;
	protected TextWidget m_Name;

	void ZBerryESPIcon() {
		root = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/gui/layouts/EspIcon.layout" );

		m_Icon = ImageWidget.Cast(root.FindAnyWidget( "plyIcon" ));
		m_Icon.SetAlpha(190);
		m_Name = TextWidget.Cast(root.FindAnyWidget( "plyName" ));
	}

	void ~ZBerryESPIcon() {
		if (root)
			root.Unlink();
	}

	void Update(vector adminPos, string plyName, float plyHP, vector plyPos, vector relDir = "0 0 0", int color = 0xFFFFFF) {
		float dist = Math.Max(vector.Distance(adminPos, plyPos), 0.1);
		float size = Math.Min(Math.Max((1/dist)*5000, 8), 32);
		float realDir = relDir[0];

		float width = -1;
		float height = -1;
		m_Icon.GetSize(width, height);

		if (plyHP <= 0) {
			root.Show(false);
			return;
		}

		m_Icon.SetSize(size, size);
		//m_Icon.SetColor(color);
		m_Icon.SetRotation(0, 0, realDir);
		m_Name.SetText(plyName + " (" + Math.Floor(plyHP).ToString() + " HP) - " + Math.Floor(dist).ToString() + "m");
		SetPosition(plyPos);
	}

	protected void SetPosition(vector plyPos) {
		float x, y;
		vector sPos = GetGame().GetScreenPos(plyPos + "0 1.85 0");

		if (!PlyInFOV(plyPos))
			return;
		x = Math.Ceil(sPos[0]) - 48;
		y = Math.Ceil(sPos[1]);
		root.SetPos(x, y);
	}

	private bool PlyInFOV(vector plyPos) {
		vector srp = GetGame().GetScreenPosRelative(plyPos);

		if ((srp[0] >= 1 || srp[1] >= 1 || srp[0] == 0 || srp[1] == 0) || srp[2] < 0) {
			root.Show( false );
			return false;
		} else {
			root.Show( true );
			return true;
		}
	}
}
