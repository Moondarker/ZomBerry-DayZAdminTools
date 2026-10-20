//Client side of the streams: tells the server what this client shows (Subscribe, only when that changes)
//and keeps the latest roster, player states and markers for the UI
class ZBerryStreamClient {
	protected bool m_ListShown; //Players panel (the menu is open)
	protected bool m_MapShown;
	protected int m_MarkerMask;
	protected int m_SelectedId = -1;
	protected int m_SentStreams = -1, m_SentMarkerMask = -1, m_SentSelectedId = -2, m_SentViewFlags = -1;

	protected ref map<int, ref ZBerryRosterEntry> m_Roster = new map<int, ref ZBerryRosterEntry>;
	protected ref TIntArray m_RosterOrder = new TIntArray;
	protected ref map<int, int> m_Positions = new map<int, int>;
	protected ref map<int, int> m_Misc = new map<int, int>;
	protected ref array<ref ZBerryMapMarkerArray> m_Markers = new array<ref ZBerryMapMarkerArray>;
	protected ref TIntArray m_Dropped = new TIntArray;
	protected int m_ServerTime; //ms since the server started, as of the last message

	ref ScriptInvoker OnPlayersChanged = new ScriptInvoker(); //Roster or states changed
	ref ScriptInvoker OnMarkersChanged = new ScriptInvoker(); //(int category)

	void ZBerryStreamClient() {
		for (int i = 0; i < ZBerryEntityCategory.MARKER_COUNT; ++i) {
			m_Markers.Insert(new ZBerryMapMarkerArray);
			m_Dropped.Insert(0);
		}

		GetZBerryRPC().AddRPC( ZBerryRPCOp.Roster, "Roster", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.PlayerState, "PlayerState", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Markers, "Markers", this, ZBerrySPExec.Client );
	}

	void SetListShown(bool shown) {
		m_ListShown = shown;
		Flush();
	}

	void SetMapShown(bool shown) {
		m_MapShown = shown;
		Flush();
	}

	void SetMarkerMask(int mask) {
		m_MarkerMask = mask;
		Flush();
	}

	void SetSelected(int playerId) {
		m_SelectedId = playerId;
		Flush();
	}

	//The menu closed: nothing to show anymore (ESP is switched by the server, it isn't affected)
	void HideAll() {
		m_ListShown = false;
		m_MapShown = false;
		m_MarkerMask = 0;
		Flush();
	}

	//Sends Subscribe if what this client shows changed
	protected void Flush() {
		int streams = 0;
		if (m_ListShown || m_MapShown) streams = ZBerryStreamFlags.ROSTER | ZBerryStreamFlags.STATE;
		int viewFlags = 0;
		if (m_MapShown) viewFlags = ZBerryStreamFlags.VIEW_MAP;
		int markerMask = 0;
		if (m_MapShown) markerMask = m_MarkerMask;

		if (streams == m_SentStreams && markerMask == m_SentMarkerMask && m_SelectedId == m_SentSelectedId && viewFlags == m_SentViewFlags) return;
		m_SentStreams = streams;
		m_SentMarkerMask = markerMask;
		m_SentSelectedId = m_SelectedId;
		m_SentViewFlags = viewFlags;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.Subscribe, new Param4<int, int, int, int>(streams, markerMask, m_SelectedId, viewFlags), true, NULL );
	}

	//Joining a server again, or new rights: subscribe from scratch
	void Reset() {
		m_SentStreams = -1;
		Flush();
	}

	void Roster( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<int, int, ref ZBerryRosterArray> data; //Server time, version, roster
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param3 ) return;

		m_ServerTime = data.param1;
		m_Roster.Clear();
		m_RosterOrder.Clear();
		foreach (ZBerryRosterEntry entry : data.param3) {
			m_Roster.Set(entry.Id, entry);
			m_RosterOrder.Insert(entry.Id);
		}

		//States of players who left
		TIntArray leftIds = new TIntArray;
		foreach (int id, int position : m_Positions) {
			if (!m_Roster.Contains(id)) leftIds.Insert(id);
		}
		foreach (int leftId : leftIds) {
			m_Positions.Remove(leftId);
			m_Misc.Remove(leftId);
		}

		OnPlayersChanged.Invoke();
	}

	void PlayerState( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<int, bool, ref TIntArray> data; //Server time, full or delta, 3 ints per player (ZBerryStatePacker)
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param3 ) return;

		m_ServerTime = data.param1;
		if (data.param2) {
			m_Positions.Clear();
			m_Misc.Clear();
		}

		TIntArray packed = data.param3;
		for (int i = 0; i + 2 < packed.Count(); i += ZBerryStatePacker.INTS_PER_PLAYER) {
			m_Positions.Set(packed[i], packed[i + 1]);
			m_Misc.Set(packed[i], packed[i + 2]);
		}

		OnPlayersChanged.Invoke();
	}

	void Markers( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<int, int, ref ZBerryMapMarkerArray> data; //Category, left out, markers
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param3 ) return;
		if (data.param1 < 0 || data.param1 >= ZBerryEntityCategory.MARKER_COUNT) return;

		m_Markers[data.param1] = data.param3;
		m_Dropped[data.param1] = data.param2;
		OnMarkersChanged.Invoke(data.param1);
	}

	int GetServerTime() {
		return m_ServerTime;
	}

	int GetPlayerCount() {
		return m_RosterOrder.Count();
	}

	//The list as the UI uses it: roster order, names with what they hold, positions on the surface
	ZBerryPlayerArray BuildPlayers() {
		ZBerryPlayerArray players = new ZBerryPlayerArray;
		foreach (int id : m_RosterOrder) {
			ZBerryRosterEntry entry = m_Roster.Get(id);
			string name = entry.Name;
			if (entry.Extra != "") name += " [" + entry.Extra + "]";

			vector position = GetPosition(id);
			int misc = m_Misc.Get(id);
			int health = 0;
			int blood = 0;
			if (ZBerryStatePacker.HasHealth(misc)) {
				health = ZBerryStatePacker.GetHealth(misc);
				blood = ZBerryStatePacker.GetBlood(misc);
			}

			players.Insert(new ZBerryPlayer(id, name, entry.IsAdmin, position, health, blood));
		}
		return players;
	}

	bool HasPosition(int playerId) {
		int position;
		return m_Positions.Find(playerId, position) && position != 0;
	}

	vector GetPosition(int playerId) {
		int packed;
		if (!m_Positions.Find(playerId, packed) || packed == 0) return "0 0 0";

		float x, z;
		ZBerryStatePacker.UnpackPosition(packed, x, z);
		return Vector(x, GetGame().SurfaceY(x, z), z);
	}

	//Yaw in degrees, false when the server didn't send it
	bool GetHeading(int playerId, out float heading) {
		int position;
		if (!m_Positions.Find(playerId, position) || position == 0) return false;

		heading = ZBerryStatePacker.GetHeading(m_Misc.Get(playerId));
		return true;
	}

	bool GetHealth(int playerId, out int health, out int blood) {
		int misc;
		if (!m_Misc.Find(playerId, misc) || !ZBerryStatePacker.HasHealth(misc)) return false;

		health = ZBerryStatePacker.GetHealth(misc);
		blood = ZBerryStatePacker.GetBlood(misc);
		return true;
	}

	bool IsAdmin(int playerId) {
		ZBerryRosterEntry entry = m_Roster.Get(playerId);
		return entry && entry.IsAdmin;
	}

	string GetName(int playerId) {
		ZBerryRosterEntry entry = m_Roster.Get(playerId);
		if (!entry) return "";
		return entry.Name;
	}

	TIntArray GetPlayerIds() {
		return m_RosterOrder;
	}

	ZBerryMapMarkerArray GetMarkers(int category) {
		if (category < 0 || category >= m_Markers.Count()) return NULL;
		return m_Markers[category];
	}

	int GetDroppedMarkers(int category) {
		if (category < 0 || category >= m_Dropped.Count()) return 0;
		return m_Dropped[category];
	}
};

static ref ZBerryStreamClient g_ZBerryStreamClient;

//Client and singleplayer
static ZBerryStreamClient GetZBerryStreams() {
	if (!g_ZBerryStreamClient) g_ZBerryStreamClient = new ZBerryStreamClient();

	return g_ZBerryStreamClient;
}
