//One line of the in-game admin log
class ZBerryLogEntry {
	int Time; //ZBerryTime seconds (UTC)
	int Kind; //ZBerryActivityLog.KIND_*
	string Text; //Players appear as "Name (Steam64 id)", so a search by name or id finds them

	void ZBerryLogEntry(int time = 0, int kind = 0, string text = "") {
		Time = time;
		Kind = kind;
		Text = text;
	}
};

typedef array<ref ZBerryLogEntry> ZBerryLogEntryArray;

//In-game admin log: what admins did and who killed whom, searchable. It's fed only by ZomberryLogger (log through
//ZomberryBase.Log, never into this directly): server side it keeps the last MAX_ENTRIES lines of the admin, ban, auth
//and kill modules in memory (the ZomBerry log file keeps everything) and answers LogRequest from admins with
//zomberry.logs.view. Client side it keeps the last answer and raises OnLogData, for the log view to draw
class ZBerryActivityLog {
	static const int KIND_ADMIN = 0; //Admin actions: functions, spawning, teleports, permission changes
	static const int KIND_BANS = 1; //Kicks, bans, refused joins
	static const int KIND_AUTH = 2; //Setup token, version problems
	static const int KIND_KILLS = 3; //Player deaths
	static const int ALL_KINDS = 15;
	static const int MAX_ENTRIES = 1000;
	static const int MAX_RESULTS = 200; //Per answer

	//Server
	protected ref ZBerryLogEntryArray m_Entries = new ZBerryLogEntryArray;
	protected ZBerryAuth m_Auth;

	//Client: the last answer
	protected ref ZBerryLogEntryArray m_LastResult = new ZBerryLogEntryArray;
	protected int m_LastServerTime;
	ref ScriptInvoker OnLogData = new ScriptInvoker(); //No arguments: read GetLastResult()

	void ZBerryActivityLog() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LogRequest, "LogRequest", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LogData, "LogData", this, ZBerrySPExec.Client );
	}

	//Server: called by ZomberryBase once auth exists
	void SetAuth(ZBerryAuth auth) {
		m_Auth = auth;
	}

	//Which kind a log module goes to, -1 for modules the in-game log doesn't keep
	static int GetKindOfModule(string module) {
		if (module == ZomberryLogger.MODULE_ADMIN) return KIND_ADMIN;
		if (module == ZomberryLogger.MODULE_BANS) return KIND_BANS;
		if (module == ZomberryLogger.MODULE_AUTH) return KIND_AUTH;
		if (module == ZomberryLogger.MODULE_KILLS) return KIND_KILLS;
		return -1;
	}

	//Server, from ZomberryLogger.Log only
	void Record(string module, string text) {
		int kind = GetKindOfModule(module);
		if (kind != -1) Add(kind, text);
	}

	protected void Add(int kind, string text) {
		m_Entries.Insert(new ZBerryLogEntry(ZBerryTime.Now(), kind, text));
		while (m_Entries.Count() > MAX_ENTRIES) m_Entries.RemoveOrdered(0);
	}

	//Newest first. kindMask: bits of KIND_*, search: case-insensitive text (a name or Steam64 id), "" for all
	ZBerryLogEntryArray Query(int kindMask, string search, int maxResults = MAX_RESULTS) {
		ZBerryLogEntryArray result = new ZBerryLogEntryArray;
		search.ToLower();

		for (int i = m_Entries.Count() - 1; i >= 0 && result.Count() < maxResults; --i) {
			ZBerryLogEntry entry = m_Entries[i];
			if ((kindMask & (1 << entry.Kind)) == 0) continue;
			if (search != "") {
				string text = entry.Text;
				text.ToLower();
				if (!text.Contains(search)) continue;
			}
			result.Insert(entry);
		}
		return result;
	}

	int GetCount() {
		return m_Entries.Count();
	}

	//"Victim (id) was killed by Killer (id) with M4-A1 from 154 m", "... was killed by an infected", "... died"
	static string DescribeKill(PlayerBase victim, Object killer) {
		string text = ZBDescribePlayer(victim.GetIdentity());
		if (!victim.GetIdentity()) { //Disconnected meanwhile, or a character nobody plays
			text = "An unnamed character";
			if (victim.GetCachedName() != "") text = victim.GetCachedName();
		}

		EntityAI killerEntity = EntityAI.Cast(killer);
		if (!killerEntity || killerEntity == victim) return text + " died";

		PlayerBase killerPlayer = PlayerBase.Cast(killerEntity.GetHierarchyRootPlayer());
		if (killerPlayer && killerPlayer != victim) {
			text += " was killed by " + ZBDescribePlayer(killerPlayer.GetIdentity());
			if (killerEntity != killerPlayer) text += " with " + killerEntity.GetDisplayName();
			float distance = vector.Distance(victim.GetPosition(), killerPlayer.GetPosition());
			return text + " from " + Math.Round(distance) + " m";
		}

		if (killerEntity.IsInherited(ZombieBase)) return text + " was killed by an infected";
		if (killerEntity.IsInherited(Transport)) return text + " was run over by a " + killerEntity.GetDisplayName();
		return text + " was killed by " + killerEntity.GetDisplayName();
	}

	//Server, from the 4_World hook. Static: deaths are logged once, through ZomberryBase.Log, however many instances exist
	static void OnPlayerKilled(PlayerBase victim, Object killer) {
		if (!GetGame().IsServer() || !victim) return;

		ZomberryBase.Log( ZomberryLogger.MODULE_KILLS, DescribeKill(victim, killer) + " at " + victim.GetPosition().ToString() );
	}

	//Server
	void LogRequest( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2< int, string > request; //Kind mask, search text
		if ( !ctx.Read( request ) ) return;
		if (GetGame().IsMultiplayer() && (type != ZBerryCallType.Server || !GetGame().IsServer() || !sender)) return;
		if (!m_Auth || !m_Auth.HasPermission(sender, ZBerryPermissions.LOGS_VIEW)) {
			ZomberryBase.Log( "ZomBerryAdmin", "WARN: " + ZBDescribePlayer(sender) + " tried to read the admin log, but IS NOT ALLOWED (" + ZBerryPermissions.LOGS_VIEW + ")" );
			return;
		}

		ZBerryLogEntryArray entries = Query(request.param1, request.param2);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LogData, new Param2<int, ref ZBerryLogEntryArray>(ZBerryTime.Now(), entries), true, sender );
	}

	//Client: ask for the log, the answer raises OnLogData
	void Request(int kindMask = ALL_KINDS, string search = "") {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LogRequest, new Param2<int, string>(kindMask, search), true, NULL );
	}

	//Client
	void LogData( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<int, ref ZBerryLogEntryArray> data; //Server time, entries (newest first)
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param2 ) return;

		m_LastServerTime = data.param1;
		m_LastResult = data.param2;
		OnLogData.Invoke();
	}

	ZBerryLogEntryArray GetLastResult() {
		return m_LastResult;
	}

	//Server time of the last answer, for "5 min ago"
	int GetLastServerTime() {
		return m_LastServerTime;
	}
};

static ref ZBerryActivityLog g_ZBerryActivityLog;

static ZBerryActivityLog GetZBerryActivityLog() {
	if (!g_ZBerryActivityLog) {
		g_ZBerryActivityLog = new ZBerryActivityLog();
		ZBerryKillFeed.OnPlayerKilled.Insert(ZBerryActivityLog.OnPlayerKilled);
	}

	return g_ZBerryActivityLog;
}
