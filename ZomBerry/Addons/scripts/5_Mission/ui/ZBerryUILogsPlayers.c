//Everyone the synced log mentions, for the log view's left pane: online players first (admins green), then offline ones
//by their last event. Double-click one to see only the events they took part in, again (or Show all) for everybody
class ZBerryLogParty {
	string Id; //Steam64
	string Name;
	int LastTime; //ZBerryTime of their newest event, 0 for damage only
	int Events;
	bool Admin;
	bool Online;
};

typedef array<ref ZBerryLogParty> ZBerryLogPartyArray;

class ZBerryUILogsPlayers extends ZBerryUIComponent {
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_SELECTED = 0xFFFF751A;
	static const int COLOR_OFFLINE = 0xFF909090;

	protected TextListboxWidget m_List; //The visible variant of m_Lists
	protected ref ZBerryScaledList m_Lists;
	protected ButtonWidget m_ShowAllButton;
	protected TextWidget m_Info;
	protected float m_LaidOutHeight = -1;

	protected ref ZBerryLogPartyArray m_Parties = new ZBerryLogPartyArray; //In row order
	protected string m_SelectedId = "";
	protected ZBerryUILogsTab m_Listener;

	void Init(Widget parent, ZBerryUILogsTab listener) {
		m_Listener = listener;
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/LogsPlayers.layout", parent)) return;

		m_Lists = new ZBerryScaledList(m_Root, "LogPlayerList", this);
		m_Lists.Pick();
		m_List = m_Lists.GetActive();
		m_ShowAllButton = ButtonWidget.Cast( BindWidget("LogsShowAll") );
		m_Info = TextWidget.Cast( m_Root.FindAnyWidget("LogPlayerInfo") );
		UpdateInfo();
	}

	//Menu shown: the resolution may have changed since
	void UpdateListScale() {
		if (!m_Lists.Pick()) return;

		m_List = m_Lists.GetActive();
		FillList();
	}

	string GetSelectedName() {
		ZBerryLogParty party = FindParty(m_SelectedId);
		if (party) return party.Name;
		return "";
	}

	//From the synced log and the roster (for who's online now)
	void Rebuild() {
		map<string, ref ZBerryLogParty> byId = new map<string, ref ZBerryLogParty>;
		ZBerryEventLog log = GetZBerryEventLog();

		ZBerryLogEventArray events = log.GetEvents();
		foreach (ZBerryLogEvent logEvent : events) {
			ZBerryLogParty actor = AddParty(byId, logEvent.ActorId, logEvent.ActorName, logEvent.Time, true);
			if (actor && IsAdminAction(logEvent.Type)) actor.Admin = true;
			AddParty(byId, logEvent.TargetId, logEvent.TargetName, logEvent.Time, true);
			for (int extra = 0; extra < logEvent.ExtraTargetIds.Count() && extra < logEvent.ExtraTargetNames.Count(); ++extra) AddParty(byId, logEvent.ExtraTargetIds[extra], logEvent.ExtraTargetNames[extra], logEvent.Time, true);
		}

		for (int i = 0; i < log.GetClientPlayerCount(); ++i) {
			string id, name;
			log.GetClientPlayer(i, id, name);
			if (!byId.Contains(id)) AddParty(byId, id, name, 0, false); //Damage only
		}

		//The roster has no Steam64 ids, so online is by name
		ZBerryPlayerArray online = GetZBerryStreams().BuildPlayers();
		foreach (string partyId, ZBerryLogParty party : byId) {
			foreach (ZBerryPlayer ply : online) {
				if (ply.m_PlayerName != party.Name) continue;
				party.Online = true;
				if (ply.m_IsAdmin) party.Admin = true;
				break;
			}
		}

		//Online first, then the most recently active (insertion sort: a few hundred at most)
		m_Parties.Clear();
		foreach (string entryId, ZBerryLogParty entry : byId) {
			int at = m_Parties.Count();
			while (at > 0 && IsBefore(entry, m_Parties[at - 1])) --at;
			m_Parties.InsertAt(entry, at);
		}

		FillList();
	}

	protected static bool IsBefore(ZBerryLogParty a, ZBerryLogParty b) {
		if (a.Online != b.Online) return a.Online;
		return a.LastTime > b.LastTime;
	}

	//Types only admins cause as the actor
	protected static bool IsAdminAction(int type) {
		return type >= ZBerryEventType.FUNCTION_SELF && type <= ZBerryEventType.UNBAN;
	}

	protected static ZBerryLogParty AddParty(map<string, ref ZBerryLogParty> byId, string id, string name, int time, bool isEvent) {
		if (id == "") return NULL;

		ZBerryLogParty party = byId.Get(id);
		if (!party) {
			party = new ZBerryLogParty;
			party.Id = id;
			byId.Set(id, party);
		}
		if (name != "") party.Name = name; //Events come oldest first: the newest name wins
		if (time > party.LastTime) party.LastTime = time;
		if (isEvent) ++party.Events;
		return party;
	}

	protected ZBerryLogParty FindParty(string id) {
		foreach (ZBerryLogParty party : m_Parties) {
			if (party.Id == id) return party;
		}
		return NULL;
	}

	protected void FillList() {
		if (!m_List) return;

		m_List.ClearItems();
		foreach (ZBerryLogParty party : m_Parties) {
			string text = " " + party.Name;
			if (party.Name == "") text = " " + party.Id;
			int row = m_List.AddItem(text, NULL, 0);
			m_List.SetItemColor(row, 0, GetColor(party));
		}
		UpdateInfo();
	}

	protected int GetColor(ZBerryLogParty party) {
		if (party.Id == m_SelectedId) return COLOR_SELECTED;
		if (!party.Online) return COLOR_OFFLINE;
		if (party.Admin) return COLOR_ADMIN;
		return ZBerryUIUtils.TEXT_DEFAULT;
	}

	protected void UpdateInfo() {
		if (!m_Info) return;

		ZBerryLogParty party = FindParty(m_SelectedId);
		if (!party) {
			m_Info.SetText("Double-click a player to see their events");
			return;
		}

		string status = "offline";
		if (party.Online) status = "online";
		m_Info.SetText("Id: " + party.Id + " | " + status + " | " + party.Events + " events");
	}

	protected void Select(string id) {
		m_SelectedId = id;
		for (int row = 0; row < m_Parties.Count(); ++row) m_List.SetItemColor(row, 0, GetColor(m_Parties[row]));
		UpdateInfo();
		if (m_Listener) m_Listener.OnPlayerSelected(m_SelectedId, GetSelectedName());
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w != m_List) return false;

		int row = m_List.GetSelectedRow();
		if (row < 0 || row >= m_Parties.Count()) return true;

		if (m_Parties[row].Id == m_SelectedId) {
			Select(""); //Again: everybody
		} else {
			Select(m_Parties[row].Id);
		}
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w != m_ShowAllButton) return false;

		Select("");
		return true;
	}

	//Same bottom-up layout as the players panel: the button row 10 units above the bottom, the info line 8 above it,
	//the list down to 8 above that (SetPos/SetSize take physical pixels, the pane is exactly 350 units wide)
	void Update(float tDelta) {
		float screenWidth, screenHeight;
		m_Root.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth <= 0 || screenHeight <= 0 || Math.AbsFloat(screenHeight - m_LaidOutHeight) < 0.5) return;
		m_LaidOutHeight = screenHeight;

		float u = screenWidth / 350;
		float infoY = screenHeight - (10 + 36 + 8 + 20) * u;
		float listY = 10 * u;
		float listHeight = Math.Max(60 * u, infoY - 8 * u - listY);
		m_Info.SetPos(4 * u, infoY);

		array<TextListboxWidget> lists = m_Lists.GetAll();
		foreach (TextListboxWidget list : lists) {
			float width, oldHeight;
			list.GetSize(width, oldHeight);
			list.SetSize(width, listHeight);
		}
	}
};
