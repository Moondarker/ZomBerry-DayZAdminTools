class ZomberryMapFullscreen extends ScriptedWidgetEventHandler {
	MapWidget m_MapWidget;
	private TextWidget m_FooterTxt;

	ZomberryMenu m_Menu;
	Widget m_Root;

	void ZomberryMapFullscreen(ZomberryMenu source) {
		m_Menu = source;
		m_Root = GetGame().GetWorkspace().CreateWidgets( "ZomBerry\\AT\\gui\\layouts\\AdminMap.layout" );
		m_MapWidget = MapWidget.Cast( m_Root.FindAnyWidget("MapWidget") );
		m_FooterTxt = TextWidget.Cast( m_Root.FindAnyWidget("FooterText") );

		m_Root.SetHandler(this);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button) {
        if ((KeyState(KeyCode.KC_LMENU) & 0x00000001) && (button == MouseState.LEFT)) {
            if (w == m_MapWidget) {
				int sX, sY;
				GetMousePos(sX, sY);
				vector mapXZY = m_MapWidget.ScreenToMap(Vector(sX, sY, 0));
                GetRPCManager().SendRPC( "ZomBerryAT", "MapTeleport", new Param1< vector >( mapXZY ), true );
                return true;
            }
        } else if (w != m_MapWidget) {
			m_Menu.CloseFullMap();
			return true;
		}
        return false;
    }

	void Update() {
		int sX, sY;

		GetMousePos(sX, sY);
		vector mapXZY = m_MapWidget.ScreenToMap(Vector(sX, sY, 0));
		m_FooterTxt.SetText("X: " + keepLength(mapXZY[0]) + "  Y: " + keepLength(mapXZY[2]) + " | Use Alt+LMB to teleport | Click on this bar to minimize");
	}

	static string keepLength(float number) {
		int floored = Math.Floor(number);
		int counter = 5;
		string result = "";

		while (floored) {
			floored /= 10;
			counter--;
		}

		for (int i = 0; i < counter; i++) {
			result += " ";
		}

		return (result + Math.Floor(number).ToString());
	}
}
