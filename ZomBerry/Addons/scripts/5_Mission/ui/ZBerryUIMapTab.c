//Map with player and object markers (filter buttons on top), arrows for where players face, cursor coordinates and Alt+LMB teleport.
//Everything shown comes from the streams (GetZBerryStreams), this tab only says what it shows
class ZBerryUIMapTab extends ZBerryUITab {
	static const int COLOR_MARK_ADMIN = 0xFF47EB00;
	static const int COLOR_MARK_PLAYER = 0xFF55AAFF;
	static const string MARK_ICON = "\\dz\\gear\\navigation\\data\\map_tree_ca.paa";
	static const string HINT = "Teleport using Alt+LMB";

	//Per marker category (ZBerryEntityCategory below MARKER_COUNT)
	static TIntArray GetCategoryColors() {
		return {0xFFFFC34D, 0xFF4DD9A0, 0xFFC08A5A, 0xFFFF5A5A, 0xFFBBBBBB};
	}

	static TStringArray GetCategoryIcons() {
		return {"\\dz\\gear\\navigation\\data\\map_station_ca.paa", "\\dz\\gear\\navigation\\data\\map_camp_ca.paa", "\\dz\\gear\\navigation\\data\\map_store_ca.paa", "\\dz\\gear\\navigation\\data\\map_tshelter_ca.paa", "\\dz\\gear\\navigation\\data\\map_border_cross_ca.paa"};
	}

	protected MapWidget m_MapWidget;
	protected CanvasWidget m_Arrows; //Over the map, redrawn every frame (the map can move and zoom)
	protected TextWidget m_MapText, m_MapHint;
	protected ButtonWidget m_PlayersFilter;
	protected ref array<ButtonWidget> m_CategoryFilters = new array<ButtonWidget>; //Per marker category

	protected bool m_ShowPlayers = true;
	protected ref array<bool> m_ShowCategory = new array<bool>;
	protected ref ZBerryPlayerArray m_LastPlayers;

	protected vector m_MapPos;
	protected float m_MapScale;
	protected bool m_MapDiff;
	protected bool m_MouseOnMap = false;
	protected bool m_TeleportAllowed = true;

	protected override void OnInit() {
		m_MapWidget = MapWidget.Cast( BindWidget("MapWidget") );
		m_MapText = TextWidget.Cast( m_Root.FindAnyWidget("MapText") );
		m_MapHint = TextWidget.Cast( m_Root.FindAnyWidget("MapHint") );
		m_Arrows = CanvasWidget.Cast( m_Root.FindAnyWidget("MapArrows") );

		m_PlayersFilter = ButtonWidget.Cast( BindWidget("MapFilterPlayers") );
		TStringArray buttonNames = {"MapFilterVehicles", "MapFilterTents", "MapFilterStorage", "MapFilterBases", "MapFilterBodies"};
		foreach (string buttonName : buttonNames) {
			m_CategoryFilters.Insert( ButtonWidget.Cast( BindWidget(buttonName) ) );
			m_ShowCategory.Insert(false);
		}
		UpdateFilterButtons();

		GetZBerryStreams().OnMarkersChanged.Insert(OnMarkersChanged);
	}

	void ~ZBerryUIMapTab() {
		if (g_ZBerryStreamClient) g_ZBerryStreamClient.OnMarkersChanged.Remove(OnMarkersChanged);
	}

	//Each filter needs its permission, active ones are green
	protected void UpdateFilterButtons() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		m_PlayersFilter.Show(auth.Has(ZBerryPermissions.MAP_PLAYER_MARKERS));
		ZBerryUIUtils.SetButtonActive(m_PlayersFilter, m_ShowPlayers);

		for (int category = 0; category < m_CategoryFilters.Count(); ++category) {
			ButtonWidget button = m_CategoryFilters[category];
			bool allowed = auth.Has(ZBerryPermissions.MapMarkerNode(category));
			if (!allowed) m_ShowCategory[category] = false;

			button.Show(allowed);
			ZBerryUIUtils.SetButtonActive(button, m_ShowCategory[category]);
		}
	}

	//Bit mask of the shown categories
	protected int GetCategoryMask() {
		int mask = 0;
		for (int category = 0; category < m_ShowCategory.Count(); ++category) {
			if (m_ShowCategory[category]) mask = mask | (1 << category);
		}
		return mask;
	}

	protected void OnMarkersChanged(int category) {
		if (IsVisible()) Redraw();
	}

	//Players and the shown categories; the widget can only clear all marks at once
	protected void Redraw() {
		m_MapWidget.ClearUserMarks();
		ZBerryStreamClient streams = GetZBerryStreams();

		TIntArray colors = GetCategoryColors();
		TStringArray icons = GetCategoryIcons();
		int dropped = 0;
		for (int category = 0; category < m_ShowCategory.Count(); ++category) {
			if (!m_ShowCategory[category]) continue;

			ZBerryMapMarkerArray markers = streams.GetMarkers(category);
			if (!markers) continue;
			foreach (ZBerryMapMarker marker : markers) {
				m_MapWidget.AddUserMark(marker.Position, marker.Label, colors[category], icons[category]);
			}
			dropped += streams.GetDroppedMarkers(category);
		}

		if (dropped > 0) {
			m_MapHint.SetText(HINT + " | " + dropped + " markers not shown (limit)");
		} else {
			m_MapHint.SetText(HINT);
		}

		if (!m_LastPlayers || !m_ShowPlayers || !GetZBerryClientAuth().Has(ZBerryPermissions.MAP_PLAYER_MARKERS)) return;

		foreach (ZBerryPlayer ply : m_LastPlayers) {
			if (!streams.HasPosition(ply.m_PlayerID)) continue;

			if (ply.m_IsAdmin) {
				m_MapWidget.AddUserMark(ply.m_PlayerPos, ply.m_PlayerName, COLOR_MARK_ADMIN, MARK_ICON);
			} else {
				m_MapWidget.AddUserMark(ply.m_PlayerPos, ply.m_PlayerName, COLOR_MARK_PLAYER, MARK_ICON);
			}
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_PlayersFilter) {
			m_ShowPlayers = !m_ShowPlayers;
			UpdateFilterButtons();
			Redraw();
			return true;
		}

		ButtonWidget clicked = ButtonWidget.Cast(w);
		int category = -1;
		if (clicked) category = m_CategoryFilters.Find(clicked);
		if (category == -1) return false;

		m_ShowCategory[category] = !m_ShowCategory[category];
		UpdateFilterButtons();
		GetZBerryStreams().SetMarkerMask(GetCategoryMask());
		Redraw();
		return true;
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

		GetZBerryStreams().SetMarkerMask(GetCategoryMask());
		GetZBerryStreams().SetMapShown(true);
		Redraw();
	}

	protected override void OnDeactivate() {
		GetZBerryStreams().SetMapShown(false);
	}

	override void OnPermissionsChanged() {
		UpdateFilterButtons();
		GetZBerryStreams().SetMarkerMask(GetCategoryMask());
		Redraw();
	}

	override void OnMenuShow() {
		if (IsVisible()) OnActivate(); //The menu reopened on this tab
	}

	override void OnMenuHide() {
		m_MapDiff = false;
	}

	//A small arrow in front of each player marker, pointing where they face. Canvas coordinates are physical pixels
	protected void DrawArrows() {
		if (!m_Arrows) return;
		m_Arrows.Clear();
		if (!m_LastPlayers || !m_ShowPlayers || !GetZBerryClientAuth().Has(ZBerryPermissions.MAP_PLAYER_MARKERS)) return;

		ZBerryStreamClient streams = GetZBerryStreams();
		float canvasX, canvasY, canvasWidth, canvasHeight;
		m_Arrows.GetScreenPos(canvasX, canvasY);
		m_Arrows.GetScreenSize(canvasWidth, canvasHeight);
		float size = 9 * ZBerryUIUtils.GetUIScale();
		float lineWidth = Math.Max(1, 2 * ZBerryUIUtils.GetUIScale());

		foreach (ZBerryPlayer ply : m_LastPlayers) {
			float heading;
			if (!streams.GetHeading(ply.m_PlayerID, heading)) continue;

			vector screenPos = m_MapWidget.MapToScreen(ply.m_PlayerPos);
			float x = screenPos[0] - canvasX;
			float y = screenPos[1] - canvasY;
			if (x < 0 || y < 0 || x > canvasWidth || y > canvasHeight) continue;

			//Yaw 0 is north (up), growing clockwise; screen y grows downwards
			float angle = heading * Math.DEG2RAD;
			float dirX = Math.Sin(angle);
			float dirY = -Math.Cos(angle);
			float tipX = x + dirX * size * 2.2;
			float tipY = y + dirY * size * 2.2;
			float baseX = x + dirX * size * 1.1;
			float baseY = y + dirY * size * 1.1;
			float sideX = -dirY * size * 0.6;
			float sideY = dirX * size * 0.6;

			int color = COLOR_MARK_PLAYER;
			if (ply.m_IsAdmin) color = COLOR_MARK_ADMIN;
			m_Arrows.DrawLine(baseX + sideX, baseY + sideY, tipX, tipY, lineWidth, color);
			m_Arrows.DrawLine(baseX - sideX, baseY - sideY, tipX, tipY, lineWidth, color);
		}
	}

	void SetPlayers(ZBerryPlayerArray players) {
		m_LastPlayers = players;
		if (IsVisible()) Redraw();
	}

	override void Update(float tDelta) {
		if (!IsVisible()) return;

		DrawArrows();

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
			if (m_TeleportAllowed && GetZBerryClientAuth().Has(ZBerryPermissions.MAP_TELEPORT)) {
				m_TeleportAllowed = false;
				GetZBerryRPC().SendRPC( ZBerryRPCOp.MapTeleport, new Param1< vector >( mapXZY ), true );
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
