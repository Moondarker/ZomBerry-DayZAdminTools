//Map with player and object markers (filter buttons on top), cursor coordinates and Alt+LMB teleport
class ZBerryUIMapTab extends ZBerryUITab {
	static const int COLOR_MARK_ADMIN = 0xFF47EB00;
	static const int COLOR_MARK_PLAYER = 0xFF55AAFF;
	static const string MARK_ICON = "\\dz\\gear\\navigation\\data\\map_tree_ca.paa";
	static const float MARKERS_REFRESH_INTERVAL = 10; //Seconds, while the map is shown and a category is on
	static const string HINT = "Teleport using Alt+LMB";

	//Per ZBerryMapCategory
	static TIntArray GetCategoryColors() {
		return {0xFFFFC34D, 0xFF4DD9A0, 0xFFC08A5A, 0xFFFF5A5A, 0xFFBBBBBB};
	}

	static TStringArray GetCategoryIcons() {
		return {"\\dz\\gear\\navigation\\data\\map_station_ca.paa", "\\dz\\gear\\navigation\\data\\map_camp_ca.paa", "\\dz\\gear\\navigation\\data\\map_store_ca.paa", "\\dz\\gear\\navigation\\data\\map_tshelter_ca.paa", "\\dz\\gear\\navigation\\data\\map_border_cross_ca.paa"};
	}

	protected MapWidget m_MapWidget;
	protected TextWidget m_MapText, m_MapHint;
	protected ButtonWidget m_PlayersFilter;
	protected ref array<ButtonWidget> m_CategoryFilters = new array<ButtonWidget>; //Per ZBerryMapCategory

	protected bool m_ShowPlayers = true;
	protected ref array<bool> m_ShowCategory = new array<bool>;
	protected ref ZBerryPlayerArray m_LastPlayers;
	protected ref ZBerryMapMarkerArray m_Markers;
	protected float m_MarkersTimer = 0;

	protected vector m_MapPos;
	protected float m_MapScale;
	protected bool m_MapDiff;
	protected bool m_MouseOnMap = false;
	protected bool m_TeleportAllowed = true;

	protected override void OnInit() {
		m_MapWidget = MapWidget.Cast( BindWidget("MapWidget") );
		m_MapText = TextWidget.Cast( m_Root.FindAnyWidget("MapText") );
		m_MapHint = TextWidget.Cast( m_Root.FindAnyWidget("MapHint") );

		m_PlayersFilter = ButtonWidget.Cast( BindWidget("MapFilterPlayers") );
		TStringArray buttonNames = {"MapFilterVehicles", "MapFilterTents", "MapFilterStorage", "MapFilterBases", "MapFilterBodies"};
		foreach (string buttonName : buttonNames) {
			m_CategoryFilters.Insert( ButtonWidget.Cast( BindWidget(buttonName) ) );
			m_ShowCategory.Insert(false);
		}
		UpdateFilterButtons();
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

	protected void RequestMarkers() {
		m_MarkersTimer = 0;
		int mask = GetCategoryMask();
		if (mask == 0) {
			m_Markers = NULL;
			Redraw();
			return;
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.MapMarkersRequest, new Param1<int>(mask), true, NULL );
	}

	void SetMarkers(ZBerryMapMarkerArray markers, int dropped) {
		m_Markers = markers;
		if (dropped > 0) {
			m_MapHint.SetText(HINT + " | " + dropped + " markers not shown (limit)");
		} else {
			m_MapHint.SetText(HINT);
		}
		Redraw();
	}

	//Players and the shown categories; the widget can only clear all marks at once
	protected void Redraw() {
		m_MapWidget.ClearUserMarks();

		if (m_Markers) {
			TIntArray colors = GetCategoryColors();
			TStringArray icons = GetCategoryIcons();
			foreach (ZBerryMapMarker marker : m_Markers) {
				if (marker.Category < 0 || marker.Category >= m_ShowCategory.Count() || !m_ShowCategory[marker.Category]) continue;
				m_MapWidget.AddUserMark(marker.Position, marker.Label, colors[marker.Category], icons[marker.Category]);
			}
		}

		if (!m_LastPlayers || !m_ShowPlayers || !GetZBerryClientAuth().Has(ZBerryPermissions.MAP_PLAYER_MARKERS)) return;

		foreach (ZBerryPlayer ply : m_LastPlayers) {
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
		Redraw();
		if (m_ShowCategory[category]) RequestMarkers();
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

		ZBerryUIPlayersPanel.RequestSync(); //Fresh markers right away, the players panel refreshes them from then on
		RequestMarkers();
	}

	override void OnPermissionsChanged() {
		UpdateFilterButtons();
		Redraw();
	}

	override void OnMenuHide() {
		m_MapDiff = false;
	}

	void SetPlayers(ZBerryPlayerArray players) {
		m_LastPlayers = players;
		Redraw();
	}

	override void Update(float tDelta) {
		if (!IsVisible()) return;

		m_MarkersTimer += tDelta;
		if (m_MarkersTimer >= MARKERS_REFRESH_INTERVAL && GetCategoryMask() != 0) RequestMarkers();

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
