//In-game log view: events and player damage from the synced log (GetZBerryEventLog), newest first. Search by text,
//name or Steam64 id, filter by type (dropdown) and by player (its own left pane, ZBerryUILogsPlayers, shown instead of
//the players panel). The header action button pauses and resumes live updates. Right-clicking a row opens its actions
//(a death: its loadout overview); double-clicking a death opens the overview directly
class ZBerryUILogsTab extends ZBerryUITab {
	static const int ACTION_LOADOUT = 1;
	static const int ACTION_COPY_LINE = 2;
	static const int ACTION_COPY_ALL = 3;
	static const int ACTION_TELEPORT_BACK = 4;

	static const int TYPE_DAMAGE = 20; //Filter bit of the damage queue, after the event types
	static const int ALL_TYPES = 2097150; //Bits 1..20
	static const int MAX_ROWS = 300; //Newest matches shown, a listbox gets slow with thousands
	static const float SEARCH_DELAY = 0.3; //After the last keystroke
	static const float REFRESH_INTERVAL = 1; //While the log keeps arriving, at most one rebuild per second

	static const int COLOR_KILL = 0xFFFF5A5A;
	static const int COLOR_DAMAGE = 0xFFE8A060;
	static const int COLOR_HARMFUL = 0xFFFF9090;
	static const int COLOR_BANS = 0xFFFFC840;
	static const int COLOR_REFUSED = 0xFFFF4040;
	static const int COLOR_SESSION = 0xFF909090;
	static const int COLOR_PERMISSIONS = 0xFF80C0FF;
	static const int COLOR_SERVER = 0xFFC8A0FF;
	static const int COLOR_MESSAGE = 0xFF70E0E0;
	static const int COLOR_TYPE_ON = 0xFF47EB00;

	protected TextListboxWidget m_List; //The visible variant of m_Lists
	protected ref ZBerryScaledList m_Lists;
	protected EditBoxWidget m_SearchInput;
	protected ButtonWidget m_SearchButton, m_TypesButton;
	protected Widget m_TypesDropdown;
	protected ref array<ButtonWidget> m_TypeButtons = new array<ButtonWidget>;
	protected ref TIntArray m_TypeButtonTypes = new TIntArray; //Type of each dropdown row, -1 for All, -2 for None
	protected TextWidget m_Summary, m_Details;
	protected float m_LaidOutHeight = -1;

	protected Widget m_SideParent;
	protected ref ZBerryUILogsPlayers m_Players;

	protected int m_TypeMask = ALL_TYPES;
	protected string m_Search = "";
	protected string m_PlayerId = "";
	protected string m_PlayerName = "";
	protected bool m_Live = true;
	protected int m_LocalOffset; //Events are kept in UTC, shown in the admin's local time
	protected bool m_Dirty = true; //The log changed since the last rebuild
	protected bool m_PlayersBuilt = false;
	protected float m_RefreshTimer = 0;
	protected float m_SearchTimer = -1; //Counting down to applying the typed search, -1 when nothing is pending
	protected bool m_MouseOnSearch = false;
	protected ref TStringArray m_RowDetails = new TStringArray;
	protected ref array<ref ZBerryLogEvent> m_RowEvents = new array<ref ZBerryLogEvent>; //NULL for damage rows. Held: the log may drop them meanwhile

	protected ZBerryUIActionsMenu m_ActionsMenu;
	protected ZBerryUIDeathLoadoutDialog m_DeathDialog;
	protected ZBerryUITeleportBackDialog m_TeleportDialog;
	protected int m_ActionsTeleportId = -1; //The revertable teleport the actions menu was opened for, -1 for none
	protected int m_PendingTeleportId = -1; //Teleport back asked for, waiting for its players
	protected ref ZBerryLogEvent m_ActionsEvent; //The row the actions menu was opened for
	protected int m_ActionsRow = -1;

	//Before InitTab: where the left pane goes (the menu's content area)
	void SetSideParent(Widget parent) {
		m_SideParent = parent;
	}

	//They belong to the menu (drawn above every tab)
	void SetDialogs(ZBerryUIActionsMenu actionsMenu, ZBerryUIDeathLoadoutDialog deathDialog, ZBerryUITeleportBackDialog teleportDialog) {
		m_ActionsMenu = actionsMenu;
		m_DeathDialog = deathDialog;
		m_TeleportDialog = teleportDialog;
	}

	void ~ZBerryUILogsTab() {
		if (g_ZBerryClientTeleports) g_ZBerryClientTeleports.OnReturns.Remove(OnTeleportReturns);
		if (!g_ZBerryEventLog) return;
		g_ZBerryEventLog.OnEventsChanged.Remove(OnLogChanged);
		g_ZBerryEventLog.OnDamageChanged.Remove(OnLogChanged);
	}

	protected override void OnInit() {
		m_Lists = new ZBerryScaledList(m_Root, "LogList", this);
		m_Lists.Pick();
		m_List = m_Lists.GetActive();
		m_SearchInput = EditBoxWidget.Cast( BindWidget("LogSearchInput") );
		m_SearchButton = ButtonWidget.Cast( BindWidget("LogSearchButton") );
		m_TypesButton = ButtonWidget.Cast( BindWidget("LogTypesButton") );
		m_TypesDropdown = m_Root.FindAnyWidget("LogTypesDropdown");
		m_Summary = TextWidget.Cast( m_Root.FindAnyWidget("LogSummary") );
		m_Details = TextWidget.Cast( m_Root.FindAnyWidget("LogDetails") );

		AddTypeButton("LogTypeAll", -1);
		AddTypeButton("LogTypeNone", -2);
		for (int type = ZBerryEventType.FUNCTION_SELF; type <= TYPE_DAMAGE; ++type) AddTypeButton("LogType" + type, type);
		UpdateTypeButtons();

		if (m_SideParent) {
			m_Players = new ZBerryUILogsPlayers();
			m_Players.Init(m_SideParent, this);
			m_Players.Show(false);
		}

		GetZBerryEventLog().OnEventsChanged.Insert(OnLogChanged);
		GetZBerryEventLog().OnDamageChanged.Insert(OnLogChanged);
	}

	protected void AddTypeButton(string name, int type) {
		ButtonWidget btn = ButtonWidget.Cast( BindWidget(name) );
		if (!btn) return;

		m_TypeButtons.Insert(btn);
		m_TypeButtonTypes.Insert(type);
	}

	protected override void OnActivate() {
		if (m_Players) m_Players.Show(true);
		UpdateLiveButton();
		Rebuild();
	}

	protected override void OnDeactivate() {
		if (m_Players) m_Players.Show(false);
		ShowDropdown(false);
	}

	override void OnMenuShow() {
		if (m_Lists.Pick()) m_List = m_Lists.GetActive();
		if (m_Players) m_Players.UpdateListScale();
		m_Dirty = true;
		if (IsVisible()) Rebuild();
	}

	override void OnMenuHide() {
		ShowDropdown(false);
		m_MouseOnSearch = false;
	}

	override bool BlocksClose() {
		return m_MouseOnSearch;
	}

	override bool HasSidePane() {
		return m_Players != NULL;
	}

	override void OnActionButtonClick() {
		m_Live = !m_Live;
		UpdateLiveButton();
		if (m_Live && m_Dirty) Rebuild();
	}

	protected void UpdateLiveButton() {
		if (m_Live) {
			SetActionButton("Live", COLOR_GREEN);
		} else {
			SetActionButton("Paused", ZBerryUIUtils.TEXT_DEFAULT);
		}
	}

	//Batches keep arriving after login (and whenever something happens): rebuilt on the next Update
	protected void OnLogChanged() {
		m_Dirty = true;
	}

	//From the left pane: "" for everybody
	void OnPlayerSelected(string steamId, string name) {
		m_PlayerId = steamId;
		m_PlayerName = name;
		Rebuild();
	}

	override void Update(float tDelta) {
		LayoutRows();
		if (m_Players) m_Players.Update(tDelta);

		if (m_SearchTimer >= 0) {
			m_SearchTimer -= tDelta;
			if (m_SearchTimer < 0) ApplySearch();
		}

		m_RefreshTimer += tDelta;
		if (m_Dirty && m_Live && m_RefreshTimer >= REFRESH_INTERVAL) Rebuild();
	}

	//The list runs down to the details line at the bottom (layouts can't pin to both edges, see the players panel)
	protected void LayoutRows() {
		float screenWidth, screenHeight;
		m_Root.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth <= 0 || screenHeight <= 0 || Math.AbsFloat(screenHeight - m_LaidOutHeight) < 0.5) return;
		m_LaidOutHeight = screenHeight;

		float u = screenWidth / 600;
		float detailsY = screenHeight - (10 + 54) * u; //Three wrapped lines, long events don't fit one
		float listY = 70 * u;
		float listHeight = Math.Max(60 * u, detailsY - 8 * u - listY);
		m_Details.SetPos(4 * u, detailsY);

		array<TextListboxWidget> lists = m_Lists.GetAll();
		foreach (TextListboxWidget list : lists) {
			float width, oldHeight;
			list.GetSize(width, oldHeight);
			list.SetSize(width, listHeight);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_TypesButton) {
			ShowDropdown(!m_TypesDropdown.IsVisible());
			return true;
		}

		ButtonWidget btn = ButtonWidget.Cast(w);
		int typeRow = -1;
		if (btn) typeRow = m_TypeButtons.Find(btn);
		if (typeRow != -1) {
			ToggleType(m_TypeButtonTypes[typeRow]);
			return true;
		}

		ShowDropdown(false);
		if (w == m_SearchButton) {
			ApplySearch();
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (w != m_SearchInput) return false;

		if (finished) {
			ApplySearch();
		} else {
			m_SearchTimer = SEARCH_DELAY;
		}
		return true;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = true;
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_SearchInput) {
			m_MouseOnSearch = false;
			return true;
		}
		return false;
	}

	//---------------------------------------------------------------- Row actions

	//The selected row's event if it's a death whose gear may be looked at, NULL otherwise
	protected ZBerryLogEvent GetSelectedDeath(out int row) {
		row = m_List.GetSelectedRow();
		if (row < 0 || row >= m_RowEvents.Count()) return NULL;

		ZBerryLogEvent logEvent = m_RowEvents[row];
		if (!logEvent || logEvent.Type != ZBerryEventType.KILL) return NULL;
		if (!GetZBerryClientAuth().Has(ZBerryPermissions.LOADOUTS_USE)) return NULL;
		return logEvent;
	}

	//On release: the list has handled the press by then (and selected the row under the mouse, if it does that)
	override bool OnMouseButtonUp(Widget w, int x, int y, int button) {
		if (w != m_List || button != MouseState.RIGHT || !m_ActionsMenu) return false;

		int row = m_List.GetSelectedRow();
		if (row < 0 || row >= m_RowDetails.Count()) return false;

		int deathRow;
		ZBerryLogEvent death = GetSelectedDeath(deathRow);
		m_ActionsEvent = death;
		m_ActionsRow = row;
		m_ActionsMenu.Clear();
		if (death) m_ActionsMenu.Add("Loadout overview", ACTION_LOADOUT);
		m_ActionsTeleportId = -1;
		ZBerryLogEvent rowEvent = NULL;
		if (row < m_RowEvents.Count()) rowEvent = m_RowEvents[row];
		if (rowEvent && (rowEvent.Flags & ZBerryEventFlags.REVERTABLE) != 0 && GetZBerryClientAuth().Has(ZBerryPermissions.TELEPORT_REVERT)) {
			m_ActionsTeleportId = rowEvent.Id;
			m_ActionsMenu.Add("Teleport back", ACTION_TELEPORT_BACK);
		}
		m_ActionsMenu.Add("Copy this line", ACTION_COPY_LINE);
		m_ActionsMenu.Add("Copy the shown log", ACTION_COPY_ALL);
		m_ActionsMenu.OnAction.Clear(); //Whoever opens it listens
		m_ActionsMenu.OnAction.Insert(OnRowAction);

		string title = "Log line";
		if (death) title = "Death of " + death.TargetName;
		m_ActionsMenu.Open(x, y, title);
		return true;
	}

	protected void OnRowAction(int action) {
		if (action == ACTION_TELEPORT_BACK) {
			StartTeleportBack(m_ActionsTeleportId);
		} else if (action == ACTION_LOADOUT) {
			OpenOverview(m_ActionsEvent, m_ActionsRow);
		} else if (action == ACTION_COPY_LINE) {
			if (m_ActionsRow < 0 || m_ActionsRow >= m_RowDetails.Count()) return;
			GetGame().CopyToClipboard(m_RowDetails[m_ActionsRow]);
			ZBerryUIUtils.Message("Copied the line to the clipboard");
		} else if (action == ACTION_COPY_ALL) {
			string text = "";
			foreach (string line : m_RowDetails) text += line + "\n";
			GetGame().CopyToClipboard(text);
			ZBerryUIUtils.Message("Copied " + m_RowDetails.Count() + " lines to the clipboard");
		}
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w != m_List || button != MouseState.LEFT) return false;

		int row;
		ZBerryLogEvent logEvent = GetSelectedDeath(row);
		if (!logEvent) return false;

		OpenOverview(logEvent, row);
		return true;
	}

	//Asks for the teleport's players: one is sent back at once, several open the dialog
	protected void StartTeleportBack(int eventId) {
		if (eventId < 0) return;
		m_PendingTeleportId = eventId;
		ZBerryClientTeleports teleports = GetZBerryClientTeleports(); //Again each time: the client side is built anew per server
		teleports.OnReturns.Remove(OnTeleportReturns);
		teleports.OnReturns.Insert(OnTeleportReturns);
		teleports.RequestReturns(eventId);
	}

	protected void OnTeleportReturns(int eventId) {
		if (eventId != m_PendingTeleportId) return;
		m_PendingTeleportId = -1; //The answer to the revert comes back too, it's not for this

		string reason;
		ZBerryTeleportRowArray rows;
		if (!GetZBerryClientTeleports().GetReturns(eventId, reason, rows)) return;
		if (reason != "") {
			ZBerryUIUtils.Message(reason);
			return;
		}
		if (rows && rows.Count() == 1) {
			TStringArray everyone = new TStringArray;
			GetZBerryClientTeleports().Revert(eventId, everyone);
			return;
		}
		if (m_TeleportDialog) m_TeleportDialog.OpenFor(eventId, DescribeEventRow(eventId));
	}

	//The details line of the event's row, "" if it isn't shown
	protected string DescribeEventRow(int eventId) {
		for (int i = 0; i < m_RowEvents.Count() && i < m_RowDetails.Count(); ++i) {
			if (m_RowEvents[i] && m_RowEvents[i].Id == eventId) return m_RowDetails[i];
		}
		return "";
	}

	protected void OpenOverview(ZBerryLogEvent logEvent, int row) {
		if (!m_DeathDialog || !logEvent) return;

		string description = "";
		if (row >= 0 && row < m_RowDetails.Count()) description = m_RowDetails[row];
		m_DeathDialog.Open(logEvent, description, ZBerryTime.FormatLocalClock(logEvent.Time, m_LocalOffset));
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn) {
		if (w != m_List) return false;

		ShowDropdown(false);
		if (row >= 0 && row < m_RowDetails.Count()) {
			m_Details.SetText(m_RowDetails[row]);
		} else {
			m_Details.SetText("");
		}
		return true;
	}

	protected void ApplySearch() {
		m_SearchTimer = -1;
		string search = m_SearchInput.GetText();
		search = search.Trim();
		if (search == m_Search) return;

		m_Search = search;
		Rebuild();
	}

	protected void ShowDropdown(bool show) {
		if (m_TypesDropdown) m_TypesDropdown.Show(show);
	}

	protected void ToggleType(int type) {
		if (type == -1) {
			m_TypeMask = ALL_TYPES;
		} else if (type == -2) {
			m_TypeMask = 0;
		} else {
			m_TypeMask = m_TypeMask ^ (1 << type);
		}
		UpdateTypeButtons();
		Rebuild();
	}

	protected void UpdateTypeButtons() {
		int shown = 0;
		int lastShown = 0;
		for (int i = 0; i < m_TypeButtons.Count(); ++i) {
			int type = m_TypeButtonTypes[i];
			if (type < 0) continue;

			bool on = (m_TypeMask & (1 << type)) != 0;
			string mark = "[ ] ";
			int color = ZBerryUIUtils.TEXT_DEFAULT;
			if (on) {
				mark = "[x] ";
				color = COLOR_TYPE_ON;
				++shown;
				lastShown = type;
			}
			ZBerryUIUtils.SetButtonText(m_TypeButtons[i], "  " + mark + GetTypeName(type));
			ZBerryUIUtils.SetButtonTextColor(m_TypeButtons[i], color);
		}

		string caption = "Types: " + shown + " of " + TYPE_DAMAGE;
		if (shown == TYPE_DAMAGE) caption = "Types: all";
		if (shown == 0) caption = "Types: none";
		if (shown == 1) caption = "Types: " + GetTypeName(lastShown);
		ZBerryUIUtils.SetButtonText(m_TypesButton, caption);
	}

	static string GetTypeName(int type) {
		if (type == TYPE_DAMAGE) return "Damage";
		return ZBerryEventType.GetName(type);
	}

	//---------------------------------------------------------------- Rows

	//Events and damage records merged by time, newest first, until MAX_ROWS
	protected void Rebuild() {
		if (m_Players && (m_Dirty || !m_PlayersBuilt)) { //Only when the log changed: it costs a pass over everything
			m_Players.Rebuild();
			m_PlayersBuilt = true;
		}
		m_Dirty = false;
		m_RefreshTimer = 0;
		if (!m_List) return;

		ZBerryEventLog log = GetZBerryEventLog();
		string search = m_Search;
		m_LocalOffset = ZBerryTime.GetLocalOffset();
		search.ToLower();

		ZBerryLogEventArray events = new ZBerryLogEventArray;
		if ((m_TypeMask & (ALL_TYPES - (1 << TYPE_DAMAGE))) != 0) events = log.FilterEvents(m_TypeMask, search);

		//The player's index in the damage name table: records are filtered on ints, strings only for the shown ones
		bool withDamage = (m_TypeMask & (1 << TYPE_DAMAGE)) != 0;
		int playerIndex = -1;
		if (m_PlayerId != "") playerIndex = log.FindClientPlayer(m_PlayerId);
		if (m_PlayerId != "" && playerIndex == -1) withDamage = false;

		m_List.ClearItems();
		m_RowDetails.Clear();
		m_RowEvents.Clear();
		m_Details.SetText("");

		int eventAt = NextEvent(events, 0);
		int damageAt = -1;
		if (withDamage) damageAt = NextDamage(log, log.GetClientDamageCount() - 1, playerIndex, search);

		bool capped = false;
		while (eventAt != -1 || damageAt != -1) {
			if (m_List.GetNumItems() >= MAX_ROWS) {
				capped = true;
				break;
			}

			bool takeEvent = damageAt == -1;
			if (eventAt != -1 && damageAt != -1) takeEvent = events[eventAt].Time >= log.GetClientDamageTime(damageAt);

			if (takeEvent) {
				AddEventRow(events[eventAt]);
				eventAt = NextEvent(events, eventAt + 1);
			} else {
				AddDamageRow(log, damageAt);
				damageAt = NextDamage(log, damageAt - 1, playerIndex, search);
			}
		}

		UpdateSummary(log, capped);
	}

	//Index of the next event (newest first) the player filter keeps, -1 at the end
	protected int NextEvent(ZBerryLogEventArray events, int from) {
		for (int i = from; i < events.Count(); ++i) {
			if (m_PlayerId == "" || events[i].Involves(m_PlayerId)) return i;
		}
		return -1;
	}

	//Index of the next damage record (going back from 'from') the filters keep, -1 at the end
	protected int NextDamage(ZBerryEventLog log, int from, int playerIndex, string searchLower) {
		for (int i = from; i >= 0; --i) {
			int attackerIndex, victimIndex;
			if (!log.GetClientDamageParties(i, attackerIndex, victimIndex)) continue;
			if (playerIndex != -1 && attackerIndex != playerIndex && victimIndex != playerIndex) continue;
			if (searchLower != "") {
				string all = DescribeDamage(log, i, true);
				all.ToLower();
				if (!all.Contains(searchLower)) continue;
			}
			return i;
		}
		return -1;
	}

	protected void AddEventRow(ZBerryLogEvent logEvent) {
		int row = m_List.AddItem(" " + ZBerryTime.FormatLocalClock(logEvent.Time, m_LocalOffset) + "  " + logEvent.FormatNames(), NULL, 0);
		m_List.SetItemColor(row, 0, GetEventColor(logEvent));
		m_RowDetails.Insert(ZBerryTime.FormatLocal(logEvent.Time, m_LocalOffset) + " | " + GetTypeName(logEvent.Type) + " | " + logEvent.Format());
		m_RowEvents.Insert(logEvent);
	}

	protected void AddDamageRow(ZBerryEventLog log, int index) {
		int time = log.GetClientDamageTime(index);
		int row = m_List.AddItem(" " + ZBerryTime.FormatLocalClock(time, m_LocalOffset) + "  " + DescribeDamage(log, index, false), NULL, 0);
		m_List.SetItemColor(row, 0, COLOR_DAMAGE);
		m_RowDetails.Insert(ZBerryTime.FormatLocal(time, m_LocalOffset) + " | Damage | " + DescribeDamage(log, index, true));
		m_RowEvents.Insert(NULL);
	}

	//"A hit B 3x with KA-M, 37.5 damage, 30 m", with the Steam64 ids when withIds
	protected static string DescribeDamage(ZBerryEventLog log, int index, bool withIds) {
		int time, hits, distance;
		float damage;
		string attackerId, attackerName, victimId, victimName, weapon;
		if (!log.GetClientDamage(index, time, attackerId, attackerName, victimId, victimName, hits, damage, weapon, distance)) return "";

		string attacker = attackerName;
		string victim = victimName;
		if (withIds) {
			attacker = ZBerryLogEvent.DescribeParty(attackerName, attackerId);
			victim = ZBerryLogEvent.DescribeParty(victimName, victimId);
		}

		string text = attacker + " hit " + victim;
		if (hits > 1) text += " " + hits + "x";
		if (weapon != "") text += " with " + weapon;
		return text + ", " + damage.ToString() + " damage, " + distance + " m";
	}

	protected static int GetEventColor(ZBerryLogEvent logEvent) {
		switch (logEvent.Type) {
			case ZBerryEventType.KILL: return COLOR_KILL;
			case ZBerryEventType.REFUSED: return COLOR_REFUSED;
			case ZBerryEventType.KICK:
			case ZBerryEventType.BAN:
			case ZBerryEventType.UNBAN:
			case ZBerryEventType.JOIN_REFUSED: return COLOR_BANS;
			case ZBerryEventType.LOGIN:
			case ZBerryEventType.LOGOUT: return COLOR_SESSION;
			case ZBerryEventType.PERMISSIONS:
			case ZBerryEventType.AUTH: return COLOR_PERMISSIONS;
			case ZBerryEventType.SERVER: return COLOR_SERVER;
			case ZBerryEventType.MESSAGE: return COLOR_MESSAGE;
		}
		if ((logEvent.Flags & ZBerryEventFlags.HARMFUL) != 0) return COLOR_HARMFUL;
		return ZBerryUIUtils.TEXT_DEFAULT;
	}

	protected void UpdateSummary(ZBerryEventLog log, bool capped) {
		string text = m_List.GetNumItems().ToString() + " entries";
		if (capped) text = "Newest " + MAX_ROWS + " entries";
		if (m_PlayerId != "") text += " with " + m_PlayerName;
		if (m_Search != "") text += " matching \"" + m_Search + "\"";
		text += " | " + log.GetEvents().Count() + " events, " + log.GetClientDamageCount() + " damage records kept";
		m_Summary.SetText(text);
	}
};
