//"Teleport back" for a teleport that moved several players, from the Logs tab: each of them with where they were and
//whether they can be moved back now. Click rows to tick them, then send everyone or only the ticked ones back.
//Part of the menu's own layout like the other dialogs; Esc closes it
class ZBerryUITeleportBackDialog extends ZBerryUIComponent {
	static const int COLOR_UNAVAILABLE = 0xFF808080;

	protected TextWidget m_Status;
	protected MultilineTextWidget m_Info;
	protected TextListboxWidget m_List; //The visible variant of m_Lists
	protected ref ZBerryScaledList m_Lists;
	protected ButtonWidget m_AllButton, m_SelectedButton, m_CloseButton;

	protected int m_EventId = -1;
	protected bool m_Answered;
	protected ref ZBerryTeleportRowArray m_Rows = new ZBerryTeleportRowArray;
	protected ref TStringArray m_Ticked = new TStringArray; //Steam64 ids

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/TeleportBackDialog.layout", parent)) return;

		m_Info = MultilineTextWidget.Cast( m_Root.FindAnyWidget("TeleportInfo") );
		m_Status = TextWidget.Cast( m_Root.FindAnyWidget("TeleportStatus") );
		m_Lists = new ZBerryScaledList(m_Root, "TeleportRows", this);
		m_Lists.Pick();
		m_List = m_Lists.GetActive();
		m_AllButton = ButtonWidget.Cast( BindWidget("TeleportAll") );
		m_SelectedButton = ButtonWidget.Cast( BindWidget("TeleportSelected") );
		m_CloseButton = ButtonWidget.Cast( BindWidget("TeleportClose") );
		Show(false);
	}

	void ~ZBerryUITeleportBackDialog() {
		if (g_ZBerryClientTeleports) g_ZBerryClientTeleports.OnReturns.Remove(OnReturns);
	}

	//A teleport event's players: shown at once, filled as soon as the server sends them
	void OpenFor(int eventId, string description) {
		if (!m_Root) return;

		m_EventId = eventId;
		m_Answered = false;
		m_Rows.Clear();
		m_Ticked.Clear();
		m_Info.SetText(description);
		if (m_Lists.Pick()) m_List = m_Lists.GetActive();
		Refresh();
		Show(true);

		//Again on every open: the client side is built anew for each server, the menu isn't
		ZBerryClientTeleports teleports = GetZBerryClientTeleports();
		teleports.OnReturns.Remove(OnReturns);
		teleports.OnReturns.Insert(OnReturns);
		teleports.RequestReturns(m_EventId);
	}

	void Close() {
		Show(false);
	}

	protected void OnReturns(int eventId) {
		if (eventId != m_EventId || !IsVisible()) return;

		string reason;
		ZBerryTeleportRowArray rows;
		if (!GetZBerryClientTeleports().GetReturns(eventId, reason, rows)) return;

		m_Answered = true;
		m_Rows.Clear();
		if (rows) {
			foreach (ZBerryTeleportRow row : rows) m_Rows.Insert(row);
		}
		for (int i = m_Ticked.Count() - 1; i >= 0; --i) { //Only who can still be moved stays ticked
			int ticked = FindRow(m_Ticked[i]);
			if (ticked == -1 || m_Rows[ticked].RowStatus != ZBerryTeleportStatus.PENDING) m_Ticked.Remove(i);
		}
		Refresh(reason);
	}

	protected int FindRow(string steamId) {
		for (int i = 0; i < m_Rows.Count(); ++i) {
			if (m_Rows[i].SteamId == steamId) return i;
		}
		return -1;
	}

	static string DescribeStatus(int status) {
		if (status == ZBerryTeleportStatus.PENDING) return "can be moved back";
		if (status == ZBerryTeleportStatus.OFFLINE) return "offline";
		if (status == ZBerryTeleportStatus.IN_VEHICLE) return "in a vehicle";
		if (status == ZBerryTeleportStatus.DIED) return "died since";
		return "back";
	}

	protected void Refresh(string reason = "") {
		m_List.ClearItems();

		if (!m_Answered) {
			SetStatus("Loading...", COLOR_UNAVAILABLE);
		} else if (reason != "") {
			SetStatus(reason, ZBerryUILogsTab.COLOR_REFUSED);
		} else {
			int pending = 0;
			foreach (ZBerryTeleportRow counted : m_Rows) {
				if (counted.RowStatus == ZBerryTeleportStatus.PENDING) ++pending;
			}
			SetStatus(m_Rows.Count().ToString() + " players, " + pending + " can be moved back. Click players to tick them", ZBerryUIUtils.TEXT_DEFAULT);

			foreach (ZBerryTeleportRow row : m_Rows) {
				string tick = "[  ]  ";
				if (m_Ticked.Find(row.SteamId) != -1) tick = "[x]  ";
				string from = ZBDescribePosition(row.FromPos);
				string status = DescribeStatus(row.RowStatus);
				int index = m_List.AddItem(tick + row.PlayerName + "    from " + from + "    " + status, NULL, 0);
				if (row.RowStatus != ZBerryTeleportStatus.PENDING) m_List.SetItemColor(index, 0, COLOR_UNAVAILABLE);
			}
		}
		UpdateButtons();
	}

	protected void SetStatus(string text, int color) {
		m_Status.SetText(text);
		m_Status.SetColor(color);
	}

	//Ticks or unticks a row that can be moved back
	protected void Toggle(int row) {
		if (row < 0 || row >= m_Rows.Count() || m_Rows[row].RowStatus != ZBerryTeleportStatus.PENDING) return;

		string steamId = m_Rows[row].SteamId;
		int ticked = m_Ticked.Find(steamId);
		if (ticked == -1) {
			m_Ticked.Insert(steamId);
		} else {
			m_Ticked.Remove(ticked);
		}
		Refresh();
		m_List.SelectRow(row);
	}

	protected bool HasPending() {
		foreach (ZBerryTeleportRow row : m_Rows) {
			if (row.RowStatus == ZBerryTeleportStatus.PENDING) return true;
		}
		return false;
	}

	protected void UpdateButtons() {
		SetAvailable(m_AllButton, m_Answered && HasPending());
		SetAvailable(m_SelectedButton, m_Answered && m_Ticked.Count() > 0);
	}

	protected static void SetAvailable(ButtonWidget btn, bool available) {
		if (available) {
			ZBerryUIUtils.SetButtonTextColor(btn, ZBerryUIUtils.TEXT_DEFAULT);
		} else {
			ZBerryUIUtils.SetButtonTextColor(btn, COLOR_UNAVAILABLE);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_CloseButton) {
			Close();
			return true;
		}
		if (w == m_List) {
			Toggle(m_List.GetSelectedRow());
			return true;
		}
		if (w == m_AllButton) {
			if (!m_Answered || !HasPending()) {
				ZBerryUIUtils.Message("Nobody here can be moved back now");
				return true;
			}
			TStringArray everyone = new TStringArray;
			GetZBerryClientTeleports().Revert(m_EventId, everyone);
			return true;
		}
		if (w == m_SelectedButton) {
			if (m_Ticked.Count() == 0) {
				ZBerryUIUtils.Message("Tick the players to move back first");
				return true;
			}
			TStringArray ticked = new TStringArray;
			ticked.InsertAll(m_Ticked);
			GetZBerryClientTeleports().Revert(m_EventId, ticked);
			return true;
		}
		return false;
	}
};
