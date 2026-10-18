//Map with player markers, cursor coordinates and Alt+LMB teleport
class ZBerryUIMapTab extends ZBerryUITab {
	static const int COLOR_MARK_ADMIN = 0xFF47EB00;
	static const int COLOR_MARK_PLAYER = 0xFF55AAFF;
	static const string MARK_ICON = "\\dz\\gear\\navigation\\data\\map_tree_ca.paa";
	static const float PLAYERS_SYNC_INTERVAL = 2.0; //Seconds between player position refreshes while the map is open

	protected MapWidget m_MapWidget;
	protected TextWidget m_MapText;

	protected vector m_MapPos;
	protected float m_MapScale;
	protected bool m_MapDiff;
	protected bool m_MouseOnMap = false;
	protected bool m_TeleportAllowed = true;
	protected float m_SyncTimer = 0;

	protected override string GetLayoutName() {
		return "MapTab.layout";
	}

	protected override void OnInit() {
		m_MapWidget = MapWidget.Cast( BindWidget("MapWidget") );
		m_MapText = TextWidget.Cast( m_Root.FindAnyWidget("MapText") );
	}

	protected override void OnActivate() {
		if (!m_MapPos) {
			m_MapPos = m_MapWidget.GetMapPos();
			m_MapScale = m_MapWidget.GetScale();
		} else {
			m_MapWidget.SetMapPos(m_MapPos);
			m_MapWidget.SetScale(m_MapScale);
		}

		SetActionButton("WIP", ZBerryUIUtils.TEXT_DEFAULT);

		m_SyncTimer = 0;
		ZBerryUIPlayersPanel.RequestSync();
	}

	override void OnMenuHide() {
		m_MapDiff = false;
	}

	void SetPlayers(ZBerryPlayerArray players) {
		m_MapWidget.ClearUserMarks();

		for ( int i = 0; i < players.Count(); ++i ) {
			ZBerryPlayer ply = players.Get(i);

			if (ply.m_IsAdmin) {
				m_MapWidget.AddUserMark(ply.m_PlayerPos, ply.m_PlayerName, COLOR_MARK_ADMIN, MARK_ICON);
			} else {
				m_MapWidget.AddUserMark(ply.m_PlayerPos, ply.m_PlayerName, COLOR_MARK_PLAYER, MARK_ICON);
			}
		}
	}

	override void Update(float tDelta) {
		if (!IsVisible()) return;

		m_SyncTimer += tDelta;
		if (m_SyncTimer >= PLAYERS_SYNC_INTERVAL) {
			m_SyncTimer = 0;
			ZBerryUIPlayersPanel.RequestSync();
		}

		//Map widget resets its position after being hidden, so restore the last one
		if (!m_MapDiff && m_MapPos && m_MapPos != m_MapWidget.GetMapPos()) {
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(CorrectMapPos, 100, false, m_MapPos.ToString(false), m_MapScale);
		} else {
			m_MapPos = m_MapWidget.GetMapPos();
			m_MapScale = m_MapWidget.GetScale();
		}

		if (!m_MouseOnMap) return;

		int sX, sY;
		GetMousePos(sX, sY);
		vector mapXZY = m_MapWidget.ScreenToMap(Vector(sX, sY, 0));
		m_MapText.SetText("X: " + mapXZY[0] + "  Y: " + mapXZY[2]);

		if ((KeyState(KeyCode.KC_LMENU) & 0x00000001) && GetMouseState(MouseState.LEFT) < 0) { //OnClick doesn't work inside MapWidget
			if (m_TeleportAllowed) {
				m_TeleportAllowed = false;
				GetRPCManager().SendRPC( "ZomBerryAT", "MapTeleport", new Param1< vector >( mapXZY ), true );
			}
		} else {
			m_TeleportAllowed = true;
		}
	}

	void CorrectMapPos(string mapPos, float mapScale) {
		m_MapWidget.SetMapPos(mapPos.ToVector());
		m_MapWidget.SetScale(mapScale);
		m_MapDiff = true;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w == m_MapWidget) {
			m_MapWidget.SetScale(0.94);
			return true;
		}
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w == m_MapWidget) {
			m_MouseOnMap = true;
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_MapWidget) {
			m_MouseOnMap = false;
			return true;
		}
		return false;
	}
};
