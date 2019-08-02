class ZBerryESPIcon {
	protected Widget root;

	protected ImageWidget m_Icon;
	protected TextWidget m_Name;

	protected int m_Range;

	void ZBerryESPIcon(int rng = 10) {
		root = GetGame().GetWorkspace().CreateWidgets( "ZomBerry/AT/gui/layouts/EspIcon.layout" );

		m_Icon = ImageWidget.Cast(root.FindAnyWidget( "plyIcon" ));
		m_Icon.SetAlpha(190);
		m_Name = TextWidget.Cast(root.FindAnyWidget( "plyName" ));
		m_Range = rng;
	}

	void ~ZBerryESPIcon() {
		if (root)
			root.Unlink();
	}

	void Update(vector adminPos, string plyName, float plyHP, vector plyPos, vector relDir, int color) {
		float dist = Math.Max(vector.Distance(adminPos, plyPos), 0.1);
		float size = Math.Min(Math.Max((1/dist)*5000, 12), 32);
		float realDir = relDir[0];

		float width = -1;
		float height = -1;
		m_Icon.GetSize(width, height);

		if (plyHP <= 0 || dist > m_Range || dist < 0.25) {
			root.Show(false);
			return;
		}

		m_Icon.SetSize(size, size);
		m_Icon.SetColor(color);
		m_Icon.SetAlpha(Math.Min(dist*0.1, 0.9));
		m_Icon.SetRotation(0, 0, realDir);
		m_Name.SetText(plyName + " (" + Math.Floor(plyHP).ToString() + " HP) - " + Math.Floor(dist).ToString() + "m"); //Those are mostly NBSPs
		m_Name.SetPos(0, 32-size);
		SetPosition(plyPos);
	}

	protected void SetPosition(vector plyPos) {
		float x, y;
		vector sPos = GetGame().GetScreenPos(plyPos);

		if (!PlyInFOV(plyPos))
			return;
		x = Math.Ceil(sPos[0]) - 64;
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
