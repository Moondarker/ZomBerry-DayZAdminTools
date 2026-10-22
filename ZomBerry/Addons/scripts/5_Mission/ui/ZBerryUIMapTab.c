//Map with player and object markers, arrows for where players face, cursor coordinates and Alt+LMB teleport.
//The top row: marker filters (a dropdown that opens on hover), teleport by typed coordinates, and the saved locations
//(a dropdown that opens on click). Everything shown comes from the streams (GetZBerryStreams), this tab only says what it shows
class ZBerryUIMapTab extends ZBerryUITab {
	static const int COLOR_MARK_ADMIN = 0xFF47EB00;
	static const int COLOR_MARK_PLAYER = 0xFF55AAFF;
	static const int COLOR_MARK_LOCATION = 0xFFE070FF;
	static const string MARK_ICON = "\\dz\\gear\\navigation\\data\\map_tree_ca.paa";
	static const string LOCATION_ICON = "\\dz\\gear\\navigation\\data\\map_tourism_ca.paa";
	static const string HINT = "Teleport: Alt+LMB";
	static const string HINT_PLAYERS = "Teleport: Alt+LMB, selected player: Ctrl+LMB";
	static const float DROPDOWN_LINGER = 0.25; //Seconds a dropdown stays open after the mouse left it and its button

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

	protected ButtonWidget m_FiltersButton;
	protected Widget m_FiltersDropdown;
	protected float m_FiltersTimer = 0; //Seconds since the mouse left the dropdown and its button
	protected ButtonWidget m_PlayersFilter, m_LocationsFilter;
	protected ref array<ButtonWidget> m_CategoryFilters = new array<ButtonWidget>; //Per marker category

	protected Widget m_Coords;
	protected EditBoxWidget m_CoordX, m_CoordY;
	protected ButtonWidget m_CoordGo, m_CoordPlayer;

	protected ButtonWidget m_LocationsButton, m_LocationAdd;
	protected Widget m_LocationsDropdown, m_LocationRows;
	protected ScrollWidget m_LocationsScroll;
	protected TextWidget m_LocationsEmpty;
	protected EditBoxWidget m_LocationName;
	protected bool m_TypingName; //The name box has the focus: the dropdown stays open
	protected float m_LocationsTimer = 0;
	protected ref array<ref ZBerryUILocationRow> m_Rows = new array<ref ZBerryUILocationRow>;

	protected ZBerryUIPlayersPanel m_PlayersPanel; //Whom Ctrl+LMB and the Player buttons teleport
	protected bool m_ShowPlayers = true;
	protected bool m_ShowLocations = true;
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

		m_FiltersButton = ButtonWidget.Cast( BindWidget("MapFiltersButton") );
		m_FiltersDropdown = m_Root.FindAnyWidget("MapFiltersDropdown");
		m_PlayersFilter = ButtonWidget.Cast( BindWidget("MapFilterPlayers") );
		m_LocationsFilter = ButtonWidget.Cast( BindWidget("MapFilterLocations") );
		TStringArray buttonNames = {"MapFilterVehicles", "MapFilterTents", "MapFilterStorage", "MapFilterBases", "MapFilterBodies"};
		foreach (string buttonName : buttonNames) {
			m_CategoryFilters.Insert( ButtonWidget.Cast( BindWidget(buttonName) ) );
			m_ShowCategory.Insert(false);
		}

		m_Coords = m_Root.FindAnyWidget("MapCoords");
		m_CoordX = EditBoxWidget.Cast( BindWidget("MapCoordX") );
		m_CoordY = EditBoxWidget.Cast( BindWidget("MapCoordY") );
		m_CoordGo = ButtonWidget.Cast( BindWidget("MapCoordGo") );
		m_CoordPlayer = ButtonWidget.Cast( BindWidget("MapCoordPlayer") );

		m_LocationsButton = ButtonWidget.Cast( BindWidget("MapLocationsButton") );
		m_LocationsDropdown = m_Root.FindAnyWidget("MapLocationsDropdown");
		m_LocationsScroll = ScrollWidget.Cast( m_Root.FindAnyWidget("MapLocationsScroll") );
		m_LocationRows = m_Root.FindAnyWidget("MapLocationsRows");
		m_LocationsEmpty = TextWidget.Cast( m_Root.FindAnyWidget("MapLocationsEmpty") );
		m_LocationName = EditBoxWidget.Cast( BindWidget("MapLocationName") );
		m_LocationAdd = ButtonWidget.Cast( BindWidget("MapLocationAdd") );

		UpdateFilterButtons();
		UpdateControls();

		GetZBerryStreams().OnMarkersChanged.Insert(OnMarkersChanged);
		GetZBerryStreams().OnLocationsChanged.Insert(OnLocationsChanged);
	}

	void ~ZBerryUIMapTab() {
		if (!g_ZBerryStreamClient) return;
		g_ZBerryStreamClient.OnMarkersChanged.Remove(OnMarkersChanged);
		g_ZBerryStreamClient.OnLocationsChanged.Remove(OnLocationsChanged);
	}

	//Each filter needs its permission, active ones are green
	protected void UpdateFilterButtons() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		m_PlayersFilter.Show(auth.Has(ZBerryPermissions.MAP_PLAYER_MARKERS));
		ZBerryUIUtils.SetButtonActive(m_PlayersFilter, m_ShowPlayers);
		ZBerryUIUtils.SetButtonActive(m_LocationsFilter, m_ShowLocations);

		for (int category = 0; category < m_CategoryFilters.Count(); ++category) {
			ButtonWidget button = m_CategoryFilters[category];
			bool allowed = auth.Has(ZBerryPermissions.MapMarkerNode(category));
			if (!allowed) m_ShowCategory[category] = false;

			button.Show(allowed);
			ZBerryUIUtils.SetButtonActive(button, m_ShowCategory[category]);
		}
		LayoutFilters();
	}

	//Hidden filters leave no gaps: the shown ones are stacked, the dropdown fits them
	protected void LayoutFilters() {
		float dropdownWidth, dropdownHeight;
		m_FiltersDropdown.GetScreenSize(dropdownWidth, dropdownHeight);
		if (dropdownWidth <= 0) return; //Hidden: laid out when shown

		float u = dropdownWidth / 120; //SetPos/SetSize take physical pixels: the dropdown is 120 units wide
		array<ButtonWidget> buttons = {m_PlayersFilter};
		buttons.InsertAll(m_CategoryFilters);
		buttons.Insert(m_LocationsFilter);

		int shown = 0;
		foreach (ButtonWidget button : buttons) {
			if (!button.IsVisible()) continue;

			button.SetPos(4 * u, (4 + shown * 25) * u);
			++shown;
		}
		m_FiltersDropdown.SetSize(dropdownWidth, (4 + shown * 25 + 3) * u);
	}

	//Teleport controls and location management follow the admin's rights
	protected void UpdateControls() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		bool canTeleport = auth.Has(ZBerryPermissions.MAP_TELEPORT);
		bool canMovePlayers = auth.Has(ZBerryPermissions.MAP_TELEPORT_PLAYERS);
		bool canManage = auth.Has(ZBerryPermissions.MAP_LOCATIONS_MANAGE);

		m_Coords.Show(canTeleport || canMovePlayers);
		m_CoordGo.Show(canTeleport);
		m_CoordPlayer.Show(canMovePlayers);
		m_LocationName.Show(canManage);
		m_LocationAdd.Show(canManage);
		RebuildLocationRows();
	}

	//Bit mask of the shown categories
	protected int GetCategoryMask() {
		int mask = 0;
		for (int category = 0; category < m_ShowCategory.Count(); ++category) {
			if (m_ShowCategory[category]) mask = mask | (1 << category);
		}
		return mask;
	}

	void SetPlayersPanel(ZBerryUIPlayersPanel playersPanel) {
		m_PlayersPanel = playersPanel;
	}

	protected string GetHint() {
		if (GetZBerryClientAuth().Has(ZBerryPermissions.MAP_TELEPORT_PLAYERS)) return HINT_PLAYERS;
		return HINT;
	}

	protected void OnMarkersChanged(int category) {
		if (IsVisible()) Redraw();
	}

	protected void OnLocationsChanged() {
		RebuildLocationRows();
		if (IsVisible()) Redraw();
	}

	//Players, saved locations and the shown categories; the widget can only clear all marks at once
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

		if (m_ShowLocations) {
			ZBerryLocationArray locations = streams.GetLocations();
			foreach (ZBerryLocation location : locations) {
				m_MapWidget.AddUserMark(location.Position, location.Name, COLOR_MARK_LOCATION, LOCATION_ICON);
			}
		}

		if (dropped > 0) {
			m_MapHint.SetText(GetHint() + " | " + dropped + " markers not shown (limit)");
		} else {
			m_MapHint.SetText(GetHint());
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

	//---------------------------------------------------------------- Saved locations

	//One row per location, rows reused; the empty note when there are none
	protected void RebuildLocationRows() {
		if (!m_LocationRows) return;

		ZBerryClientAuth auth = GetZBerryClientAuth();
		bool canTeleport = auth.Has(ZBerryPermissions.MAP_TELEPORT);
		bool canMovePlayers = auth.Has(ZBerryPermissions.MAP_TELEPORT_PLAYERS);
		bool canManage = auth.Has(ZBerryPermissions.MAP_LOCATIONS_MANAGE);

		ZBerryLocationArray locations = GetZBerryStreams().GetLocations();
		while (m_Rows.Count() > locations.Count()) {
			m_Rows[m_Rows.Count() - 1].Destroy();
			m_Rows.Remove(m_Rows.Count() - 1);
		}
		while (m_Rows.Count() < locations.Count()) {
			ZBerryUILocationRow row = new ZBerryUILocationRow();
			row.Init(m_LocationRows, this);
			m_Rows.Insert(row);
		}

		for (int i = 0; i < locations.Count(); ++i) {
			m_Rows[i].Setup(locations[i].Name, canTeleport, canMovePlayers, canManage);
		}

		m_LocationsEmpty.Show(locations.Count() == 0);
		m_LocationRows.Update();
	}

	protected ZBerryLocation FindLocation(string name) {
		ZBerryLocationArray locations = GetZBerryStreams().GetLocations();
		foreach (ZBerryLocation location : locations) {
			if (location.Name == name) return location;
		}
		return NULL;
	}

	//Centres the map on it
	void ShowLocation(string name) {
		ZBerryLocation location = FindLocation(name);
		if (!location) return;

		m_MapWidget.SetMapPos(location.Position);
		m_MapPos = m_MapWidget.GetMapPos(); //Or Update would move the map back
	}

	void TeleportToLocation(string name, bool selectedPlayer) {
		int playerId = -1;
		if (selectedPlayer && !GetSelectedPlayer(playerId)) return;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.LocationTeleport, new Param2<string, int>(name, playerId), true, NULL );
	}

	void DeleteLocation(string name) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LocationDelete, new Param1<string>(name), true, NULL );
	}

	protected void AddLocation() {
		string name = m_LocationName.GetText().Trim();
		if (name == "") {
			ZBerryUIUtils.Message("Type a name for the location first");
			return;
		}

		GetZBerryRPC().SendRPC( ZBerryRPCOp.LocationAdd, new Param1<string>(name), true, NULL );
		m_LocationName.SetText("");
	}

	protected bool GetSelectedPlayer(out int playerId) {
		if (m_PlayersPanel && m_PlayersPanel.GetSelectedPlayerId(playerId)) return true;

		ZBerryUIUtils.Message("Double-click a player in the list first");
		return false;
	}

	//---------------------------------------------------------------- Typed coordinates

	//As the map shows them: X across, Y up the map (the world's z)
	protected void TeleportToCoords(bool selectedPlayer) {
		string xText = m_CoordX.GetText().Trim();
		string yText = m_CoordY.GetText().Trim();
		if (xText == "" || yText == "") {
			ZBerryUIUtils.Message("Type both coordinates first, X and Y as the map shows them");
			return;
		}

		int playerId = -1;
		if (selectedPlayer && !GetSelectedPlayer(playerId)) return;

		float x = xText.ToFloat();
		float y = yText.ToFloat();
		GetZBerryRPC().SendRPC( ZBerryRPCOp.MapTeleport, new Param2< vector, int >( Vector(x, 0, y), playerId ), true );
	}

	//---------------------------------------------------------------- Dropdowns

	protected bool IsMouseOver(Widget w) {
		if (!w || !w.IsVisible()) return false;

		int mouseX, mouseY;
		GetMousePos(mouseX, mouseY);
		float x, y, width, height;
		w.GetScreenPos(x, y);
		w.GetScreenSize(width, height);
		return mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + height;
	}

	protected void ShowFilters(bool show) {
		m_FiltersDropdown.Show(show);
		if (show) LayoutFilters(); //After showing: hidden widgets have no screen size
		m_FiltersTimer = 0;
	}

	protected void ShowLocations(bool show) {
		m_LocationsDropdown.Show(show);
		m_LocationsTimer = 0;
		if (!show && m_TypingName) {
			SetFocus(NULL);
			m_TypingName = false;
		}
	}

	//Filters open while the mouse is on their button; both dropdowns close DROPDOWN_LINGER after the mouse left them
	//(the locations one not while its name is typed). Checked by position: rows have their own event handlers
	protected void UpdateDropdowns(float tDelta) {
		bool onFilters = IsMouseOver(m_FiltersButton) || IsMouseOver(m_FiltersDropdown);
		if (IsMouseOver(m_FiltersButton) && !m_FiltersDropdown.IsVisible()) ShowFilters(true);
		if (m_FiltersDropdown.IsVisible()) {
			if (onFilters) {
				m_FiltersTimer = 0;
			} else {
				m_FiltersTimer += tDelta;
				if (m_FiltersTimer >= DROPDOWN_LINGER) ShowFilters(false);
			}
		}

		if (!m_LocationsDropdown.IsVisible()) return;
		if (m_TypingName || IsMouseOver(m_LocationsButton) || IsMouseOver(m_LocationsDropdown)) {
			m_LocationsTimer = 0;
			return;
		}
		m_LocationsTimer += tDelta;
		if (m_LocationsTimer >= DROPDOWN_LINGER) ShowLocations(false);
	}

	override bool BlocksClose() {
		return IsMouseOver(m_CoordX) || IsMouseOver(m_CoordY) || m_TypingName;
	}

	//---------------------------------------------------------------- Events

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_PlayersFilter) {
			m_ShowPlayers = !m_ShowPlayers;
			UpdateFilterButtons();
			Redraw();
			return true;
		}
		if (w == m_LocationsFilter) {
			m_ShowLocations = !m_ShowLocations;
			UpdateFilterButtons();
			Redraw();
			return true;
		}
		if (w == m_FiltersButton) {
			ShowFilters(!m_FiltersDropdown.IsVisible());
			return true;
		}
		if (w == m_LocationsButton) {
			ShowLocations(!m_LocationsDropdown.IsVisible());
			return true;
		}
		if (w == m_LocationAdd) {
			AddLocation();
			return true;
		}
		if (w == m_CoordGo || w == m_CoordPlayer) {
			TeleportToCoords(w == m_CoordPlayer);
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

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w == m_LocationName && finished && m_LocationName.GetText().Trim() != "") {
			AddLocation();
			return true;
		}
		return false;
	}

	override bool OnFocus(Widget w, int x, int y) {
		if (w == m_LocationName) m_TypingName = true;
		return false;
	}

	override bool OnFocusLost(Widget w, int x, int y) {
		if (w == m_LocationName) m_TypingName = false;
		return false;
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
		ShowFilters(false);
		ShowLocations(false);
	}

	override void OnPermissionsChanged() {
		UpdateFilterButtons();
		UpdateControls();
		GetZBerryStreams().SetMarkerMask(GetCategoryMask());
		Redraw();
	}

	override void OnMenuShow() {
		if (IsVisible()) OnActivate(); //The menu reopened on this tab
	}

	override void OnMenuHide() {
		m_MapDiff = false;
		ShowFilters(false);
		ShowLocations(false);
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
		UpdateDropdowns(tDelta);

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

		//OnClick doesn't work inside MapWidget: Alt + left button teleports the admin, Ctrl + left button the selected player
		bool clicked = GetMouseState(MouseState.LEFT) < 0;
		bool selfTeleport = clicked && (KeyState(KeyCode.KC_LMENU) & 0x00000001);
		bool playerTeleport = clicked && (KeyState(KeyCode.KC_LCONTROL) & 0x00000001);
		if (!selfTeleport && !playerTeleport) {
			m_TeleportAllowed = true;
			return;
		}
		if (!m_TeleportAllowed) return;
		m_TeleportAllowed = false;

		ZBerryClientAuth auth = GetZBerryClientAuth();
		if (selfTeleport && auth.Has(ZBerryPermissions.MAP_TELEPORT)) {
			GetZBerryRPC().SendRPC( ZBerryRPCOp.MapTeleport, new Param2< vector, int >( mapXZY, -1 ), true );
		} else if (playerTeleport && auth.Has(ZBerryPermissions.MAP_TELEPORT_PLAYERS)) {
			int playerId;
			if (!m_PlayersPanel || !m_PlayersPanel.GetSelectedPlayerId(playerId)) {
				ZBerryUIUtils.Message("Double-click a player in the list first");
				return;
			}
			GetZBerryRPC().SendRPC( ZBerryRPCOp.MapTeleport, new Param2< vector, int >( mapXZY, playerId ), true );
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
