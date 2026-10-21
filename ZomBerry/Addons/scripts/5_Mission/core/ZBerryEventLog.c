//What kind of thing an event is. Values travel to clients: only ever append
class ZBerryEventType {
	static const int FUNCTION_SELF = 1; //A function run on the admin themselves
	static const int FUNCTION_PLAYER = 2; //... on another player
	static const int FUNCTION_SERVER = 3; //... on the world (time, weather, cleanup)
	static const int SPAWN = 4;
	static const int TELEPORT = 5;
	static const int PERMISSIONS = 6; //Permission changes, features switched off by them
	static const int KICK = 7;
	static const int BAN = 8;
	static const int UNBAN = 9;
	static const int JOIN_REFUSED = 10; //A banned player tried to join
	static const int LOGIN = 11; //In-game list only
	static const int LOGOUT = 12; //In-game list only
	static const int KILL = 13;
	static const int AUTH = 14; //Setup token, versions
	static const int REFUSED = 15; //Something an admin may not do, or a wrong setup token
	static const int SERVER = 16; //Server started or stopping
	static const int MESSAGE = 17; //Announcements and private messages from admins

	static string GetName(int type) {
		TStringArray names = {"", "Function (self)", "Function (player)", "Function (server)", "Spawn", "Teleport", "Permissions", "Kick", "Ban", "Unban", "Join refused", "Login", "Logout", "Kill", "Auth", "Refused", "Server", "Message"};
		if (type < 0 || type >= names.Count()) return "";
		return names[type];
	}

	//Kept in the in-game list only, not written to the ZomBerry log file
	static bool IsInGameOnly(int type) {
		return type == LOGIN || type == LOGOUT;
	}

	//Which ZomBerry log module the file line uses
	static string GetModule(int type) {
		if (type == KICK || type == BAN || type == UNBAN || type == JOIN_REFUSED) return ZomberryLogger.MODULE_BANS;
		if (type == AUTH) return ZomberryLogger.MODULE_AUTH;
		if (type == KILL) return ZomberryLogger.MODULE_KILLS;
		return ZomberryLogger.MODULE_ADMIN;
	}
};

class ZBerryEventFlags {
	static const int HARMFUL = 1; //A harmful function (see AddCommand)
};

//One event of the in-game log. Text is a template: {actor} and {target} stand for the two players, so the log view can
//highlight them (or make them clickable) without parsing names out of text
class ZBerryLogEvent {
	int Id; //Increasing, for syncing
	int Time; //ZBerryTime seconds (UTC)
	int Type; //ZBerryEventType
	int Flags; //ZBerryEventFlags
	string ActorId; //Steam64, "" for none
	string ActorName;
	string TargetId;
	string TargetName;
	string Text;

	//As the log file writes it: players as "Name (Steam64 id)". Game names (ZBerryLocText tags) in this game's language,
	//or with translate false in the server's, as the event was logged
	string Format(bool translate = true) {
		string text = ZBerryLocText.Resolve(Text, translate); //Before the players: their names can't make tags
		text.Replace("{actor}", DescribeParty(ActorName, ActorId));
		text.Replace("{target}", DescribeParty(TargetName, TargetId));
		return text;
	}

	//Names only, for the log view's rows (the ids are in Format)
	string FormatNames() {
		string text = ZBerryLocText.Resolve(Text, true);
		text.Replace("{actor}", ActorName);
		text.Replace("{target}", TargetName);
		return text;
	}

	bool Involves(string steamId) {
		return steamId != "" && (ActorId == steamId || TargetId == steamId);
	}

	static string DescribeParty(string name, string id) {
		if (id == "") return name;
		return name + " (" + id + ")";
	}

	//Search: text, names and ids, case-insensitive (search already in lower case)
	bool Matches(string searchLower) {
		if (searchLower == "") return true;
		string all = Format();
		all.ToLower();
		return all.Contains(searchLower);
	}
};

typedef array<ref ZBerryLogEvent> ZBerryLogEventArray;

//An admin's client being sent the log: history first in batches, then whatever is new
class ZBerryLogSubscriber {
	PlayerIdentity Identity; //Weak. NULL in singleplayer
	int NextEventId;
	int NextDamageSeq;
	int SentPlayers; //Entries of the damage name tables this client has
	int SentWeapons;
	bool Reset = true; //The next batch tells the client to start over
};

//Hits between two players, merged while they keep coming
class ZBerryDamageBurst {
	int AttackerIndex, VictimIndex, WeaponIndex; //Into the player and weapon name tables
	int Hits;
	float Damage;
	float Distance;
	int StartTime; //ZBerryTime
	int LastHitMs; //GetGame().GetTime()
};

//The in-game admin log (server side kept in memory, client side the synced copy for the log view). Fed only through
//ZomberryBase.LogEvent/LogEventRaw (and the death and hit hooks). Two queues:
// - events: MAX_EVENTS typed events (admin actions, kicks, bans, logins, kills, refusals)
// - damage: MAX_DAMAGE compact records, player against player only. Hits are merged per attacker and victim until
//   BURST_MS passes without one, or until the attacker gets hit by a player themselves (so an exchange of fire stays
//   readable, one record per volley), then stored as 7 ints; player names and weapon names are kept once, in tables that
//   only grow, and a record refers to them by index. Damage never goes into the log file
//An admin with zomberry.logs.view gets everything once after joining (BATCH_* per tick, so the server never stalls),
//then only what's new.
//On a dedicated server both queues survive restarts (StartPersisting, OnServerStopping), in two files:
// - the snapshot (FILE_NAME): everything, written at startup, at a clean shutdown and when the journal is full. Writing
//   it takes about 0.2 s, too long to do regularly
// - the journal (FILE_NAME + JOURNAL_SUFFIX): every event and damage record appended as it's stored, under 1 ms each.
//   Loading replays it over the snapshot of the same generation; a newer snapshot already holds it
class ZBerryEventLog {
	static const int MAX_EVENTS = 2048;
	static const int MAX_DAMAGE = 10486;
	static const int DAMAGE_INTS = 7; //Time, attacker, victim, hits, damage x10, weapon, distance
	static const int MAX_TEXT = 1024; //Bytes of an event's text, so the file has an upper bound
	static const int BURST_MS = 3000;
	static const int TICK_MS = 250;
	static const int BATCH_EVENTS = 64; //Per client and tick
	static const int BATCH_DAMAGE = 256;

	static const string FILE_NAME = "ZomBerryEventLog.bin";
	static const string FILE_MAGIC = "ZBLOG";
	static const int FILE_VERSION = 1; //Fields are only ever appended, under a new version that still reads the old ones
	static const string JOURNAL_SUFFIX = ".journal";
	static const string JOURNAL_MAGIC = "ZBJRN";
	static const int JOURNAL_MAX = 4096; //Entries before the snapshot is rewritten and the journal starts over
	static const int JOURNAL_EVENT = 1;
	static const int JOURNAL_DAMAGE = 2;
	//After every journal entry and at the end of the snapshot. FileSerializer.Read doesn't fail at the end of a file, so
	//this is what tells a complete entry from one a crash cut short, and the end of the journal
	static const int END_MARK = 1515870810;

	//Server
	protected ref ZBerryLogEventArray m_Events = new ZBerryLogEventArray;
	protected int m_NextEventId = 1;
	protected ref TIntArray m_Damage = new TIntArray; //Ring of MAX_DAMAGE records
	protected int m_NextDamageSeq = 0; //Sequence number of the next record
	protected ref TStringArray m_Players = new TStringArray; //"Steam64|name": ids are digits, so the first | splits them
	protected ref map<string, int> m_PlayerIndex = new map<string, int>;
	protected ref TStringArray m_Weapons = new TStringArray;
	protected ref map<string, int> m_WeaponIndex = new map<string, int>;
	protected ref map<string, ref ZBerryDamageBurst> m_Bursts = new map<string, ref ZBerryDamageBurst>;
	protected ref map<int, ref ZBerryLogSubscriber> m_Subscribers = new map<int, ref ZBerryLogSubscriber>;
	protected ref map<string, int> m_LoginTimes = new map<string, int>; //Steam64 -> ZBerryTime, for logout lines
	protected ZBerryAuth m_Auth;
	protected bool m_Ticking;
	protected string m_SavePath = ""; //Snapshot path, "" while the log isn't persisted (singleplayer, tests)
	protected int m_Generation = 0; //Of the snapshot on disk: a journal of another generation is stale
	protected int m_JournalEntries = 0; //In the journal on disk
	//What the last Load found, for the startup event
	protected bool m_LoadedFile = false;
	protected bool m_LoadedCleanStop = false;
	protected int m_LoadedJournal = 0; //Entries replayed

	//Client
	protected ref ZBerryLogEventArray m_ClientEvents = new ZBerryLogEventArray;
	protected ref TIntArray m_ClientDamage = new TIntArray; //Records, oldest first
	protected ref TStringArray m_ClientPlayers = new TStringArray;
	protected ref TStringArray m_ClientWeapons = new TStringArray;
	ref ScriptInvoker OnEventsChanged = new ScriptInvoker(); //No arguments
	ref ScriptInvoker OnDamageChanged = new ScriptInvoker(); //No arguments

	//withRPC false: a detached copy (tests loading a file), it never receives the client half of the log
	void ZBerryEventLog(bool withRPC = true) {
		m_Damage.Resize(MAX_DAMAGE * DAMAGE_INTS);
		if (!withRPC) return;

		GetZBerryRPC().AddRPC( ZBerryRPCOp.LogEvents, "OnLogEvents", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LogDamage, "OnLogDamage", this, ZBerrySPExec.Client );
	}

	void ~ZBerryEventLog() {
		if (GetGame()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
	}

	//Server, called by ZomberryBase once auth exists
	void SetAuth(ZBerryAuth auth) {
		m_Auth = auth;
		if (m_Ticking) return;
		m_Ticking = true;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_MS, true);
	}

	//---------------------------------------------------------------- Server: recording

	//Through ZomberryBase.LogEvent/LogEventRaw
	ZBerryLogEvent Add(int type, string actorId, string actorName, string targetId, string targetName, string text, int flags = 0) {
		ZBerryLogEvent logEvent = new ZBerryLogEvent;
		logEvent.Id = m_NextEventId;
		++m_NextEventId;
		logEvent.Time = ZBerryTime.Now();
		logEvent.Type = type;
		logEvent.Flags = flags;
		logEvent.ActorId = actorId;
		logEvent.ActorName = actorName;
		logEvent.TargetId = targetId;
		logEvent.TargetName = targetName;
		logEvent.Text = text;
		if (text.Length() > MAX_TEXT) logEvent.Text = text.Substring(0, MAX_TEXT) + "...";

		InsertEvent(logEvent);
		if (m_SavePath != "") JournalEvent(logEvent);
		return logEvent;
	}

	//New and replayed events: the oldest drop out, ids keep increasing
	protected void InsertEvent(ZBerryLogEvent logEvent) {
		m_Events.Insert(logEvent);
		while (m_Events.Count() > MAX_EVENTS) m_Events.RemoveOrdered(0);
		if (logEvent.Id >= m_NextEventId) m_NextEventId = logEvent.Id + 1;
	}

	int GetEventCount() {
		return m_Events.Count();
	}

	//Newest first, for tests and server tools. typeMask: bit per ZBerryEventType
	ZBerryLogEventArray Query(int typeMask, string search, int maxResults = 200) {
		ZBerryLogEventArray result = new ZBerryLogEventArray;
		search.ToLower();
		for (int i = m_Events.Count() - 1; i >= 0 && result.Count() < maxResults; --i) {
			ZBerryLogEvent logEvent = m_Events[i];
			if ((typeMask & (1 << logEvent.Type)) == 0 || !logEvent.Matches(search)) continue;
			result.Insert(logEvent);
		}
		return result;
	}

	protected int GetTableIndex(TStringArray table, map<string, int> index, string key, string entry) {
		int found;
		if (index.Find(key, found)) return found;

		found = table.Insert(entry);
		index.Set(key, found);
		return found;
	}

	//Server, from the hit hook: merged into the attacker/victim burst
	void AddHit(PlayerBase victim, PlayerBase attacker, string weapon, float damage) {
		string victimKey = GetPlayerKey(victim);
		string attackerKey = GetPlayerKey(attacker);
		string key = attackerKey + ">" + victimKey;

		//Getting hit ends whatever the victim was dealing out: in an exchange of fire the records alternate
		//(A hit B x3, B hit A x2, A hit B x1...) instead of two bursts swallowing the whole fight
		if (victimKey != attackerKey) CloseBurstsOf(victimKey);

		ZBerryDamageBurst burst = m_Bursts.Get(key);
		if (!burst) {
			burst = new ZBerryDamageBurst;
			burst.AttackerIndex = GetTableIndex(m_Players, m_PlayerIndex, attackerKey, GetSteamId(attacker) + "|" + GetName(attacker));
			burst.VictimIndex = GetTableIndex(m_Players, m_PlayerIndex, victimKey, GetSteamId(victim) + "|" + GetName(victim));
			burst.StartTime = ZBerryTime.Now();
			m_Bursts.Set(key, burst);
		}

		burst.WeaponIndex = GetTableIndex(m_Weapons, m_WeaponIndex, weapon, weapon);
		burst.Hits = burst.Hits + 1;
		burst.Damage = burst.Damage + damage;
		burst.Distance = vector.Distance(victim.GetPosition(), attacker.GetPosition());
		burst.LastHitMs = GetGame().GetTime();
	}

	//Bursts that have been quiet for BURST_MS (all of them with force) become records
	void FlushBursts(bool force = false) {
		int now = GetGame().GetTime();
		TStringArray done = new TStringArray;
		foreach (string key, ZBerryDamageBurst burst : m_Bursts) {
			if (!force && now - burst.LastHitMs < BURST_MS) continue;

			StoreBurst(burst);
			done.Insert(key);
		}
		foreach (string doneKey : done) m_Bursts.Remove(doneKey);
	}

	//The open bursts this player is dealing out become records now
	protected void CloseBurstsOf(string attackerKey) {
		string prefix = attackerKey + ">";
		TStringArray done = new TStringArray;
		foreach (string key, ZBerryDamageBurst burst : m_Bursts) {
			if (key.IndexOf(prefix) != 0) continue;

			StoreBurst(burst);
			done.Insert(key);
		}
		foreach (string doneKey : done) m_Bursts.Remove(doneKey);
	}

	protected void StoreBurst(ZBerryDamageBurst burst) {
		float damageTenths = burst.Damage * 10;
		int damage = Math.Round(damageTenths);
		int distance = Math.Round(burst.Distance);
		StoreRecord(burst.StartTime, burst.AttackerIndex, burst.VictimIndex, burst.Hits, damage, burst.WeaponIndex, distance);
		if (m_SavePath != "") JournalDamage(burst.StartTime, burst.AttackerIndex, burst.VictimIndex, burst.Hits, damage, burst.WeaponIndex, distance);
	}

	//New and replayed records
	protected void StoreRecord(int time, int attackerIndex, int victimIndex, int hits, int damageTenths, int weaponIndex, int distance) {
		int slot = (m_NextDamageSeq % MAX_DAMAGE) * DAMAGE_INTS;
		m_Damage[slot] = time;
		m_Damage[slot + 1] = attackerIndex;
		m_Damage[slot + 2] = victimIndex;
		m_Damage[slot + 3] = hits;
		m_Damage[slot + 4] = damageTenths;
		m_Damage[slot + 5] = weaponIndex;
		m_Damage[slot + 6] = distance;
		++m_NextDamageSeq;
	}

	int GetDamageCount() {
		return Math.Min(m_NextDamageSeq, MAX_DAMAGE);
	}

	//Steam64, or the network id for characters without one (AI mods), so two of those never share a burst
	protected static string GetPlayerKey(PlayerBase player) {
		string steamId = GetSteamId(player);
		if (steamId != "" || !player) return steamId;

		int low, high;
		player.GetNetworkID(low, high);
		return "#" + low + ":" + high;
	}

	protected static string GetSteamId(PlayerBase player) {
		if (!player || !player.GetIdentity()) return "";
		return player.GetIdentity().GetPlainId();
	}

	protected static string GetName(PlayerBase player) {
		if (player && player.GetIdentity()) return player.GetIdentity().GetName();
		if (player && player.GetCachedName() != "") return player.GetCachedName();
		return "An unnamed character";
	}

	//---------------------------------------------------------------- Server: hooks

	//"{target} was killed by {actor} with KA-M from 154 m", "{target} was run over by {actor} in Ada 4x4", or the cause
	//ZBerryDeathCause finds: "{target} was killed by an infected", "{target} died from a fall"...
	static void OnPlayerKilled(PlayerBase victim, Object killer) {
		if (!GetGame().IsServer() || !victim) return;

		string where = " at " + ZBDescribePosition(victim.GetPosition());
		EntityAI killerEntity = EntityAI.Cast(killer);
		PlayerBase killerPlayer;
		if (killerEntity) killerPlayer = PlayerBase.Cast(killerEntity.GetHierarchyRootPlayer());

		Transport vehicle = Transport.Cast(killer);
		if (vehicle) {
			PlayerBase driver = PlayerBase.Cast(vehicle.CrewDriver());
			if (driver && driver != victim) {
				ZomberryBase.LogEventRaw(ZBerryEventType.KILL, GetSteamId(driver), GetName(driver), GetSteamId(victim), GetName(victim), "{target} was run over by {actor} in " + ZBerryLocText.Name(vehicle) + where);
				return;
			}
		}

		if (killerPlayer && killerPlayer != victim) {
			string text = "{target} was killed by {actor}";
			if (killerEntity != killerPlayer) text += " with " + ZBerryLocText.Name(killerEntity);
			float distance = vector.Distance(victim.GetPosition(), killerPlayer.GetPosition());
			ZomberryBase.LogEventRaw(ZBerryEventType.KILL, GetSteamId(killerPlayer), GetName(killerPlayer), GetSteamId(victim), GetName(victim), text + " from " + Math.Round(distance) + " m" + where);
			return;
		}

		ZomberryBase.LogEventRaw(ZBerryEventType.KILL, "", "", GetSteamId(victim), GetName(victim), "{target}" + ZBerryDeathCause.Describe(victim, killer) + where);
	}

	//Player against player only: infected, animals and falls aren't logged
	static void OnPlayerHit(PlayerBase victim, PlayerBase attacker, EntityAI source, float damage) {
		if (!GetGame().IsServer() || !victim || !attacker || victim == attacker) return;

		string weapon = "Fists";
		if (source && source != attacker) weapon = ZBerryLocText.Name(source); //A tag: resolved where the record is shown
		GetZBerryEventLog().AddHit(victim, attacker, weapon, damage);
	}

	//Also runs for each new character of a player who stayed connected (after dying): that's a respawn, and the
	//session goes on from the first join
	void OnPlayerConnected(PlayerIdentity identity) {
		if (!identity) return;

		if (m_LoginTimes.Contains(identity.GetPlainId())) {
			ZomberryBase.LogEvent(ZBerryEventType.LOGIN, NULL, identity, "{target} respawned");
			return;
		}
		m_LoginTimes.Set(identity.GetPlainId(), ZBerryTime.Now());
		ZomberryBase.LogEvent(ZBerryEventType.LOGIN, NULL, identity, "{target} joined");
	}

	void OnPlayerDisconnected(PlayerIdentity identity) {
		if (!identity) return;

		string text = "{target} left";
		int loginTime;
		if (m_LoginTimes.Find(identity.GetPlainId(), loginTime)) {
			text += " after " + ZBerryTime.FormatDuration(ZBerryTime.Now() - loginTime);
			m_LoginTimes.Remove(identity.GetPlainId());
		}
		ZomberryBase.LogEvent(ZBerryEventType.LOGOUT, NULL, identity, text);
	}

	//---------------------------------------------------------------- Server: syncing

	//After an admin authenticates (and when their rights change): if they may read the log and aren't synced yet,
	//they get the history from the oldest kept entry on
	void StartSync(PlayerIdentity identity) {
		int id = 0;
		if (identity) id = identity.GetPlayerId();
		if (m_Subscribers.Contains(id)) return;
		if (GetGame().IsMultiplayer() && (!m_Auth || !m_Auth.HasPermission(identity, ZBerryPermissions.LOGS_VIEW))) return;

		ZBerryLogSubscriber subscriber = new ZBerryLogSubscriber;
		subscriber.Identity = identity;
		subscriber.NextEventId = m_NextEventId - m_Events.Count();
		subscriber.NextDamageSeq = m_NextDamageSeq - GetDamageCount();
		m_Subscribers.Set(id, subscriber);
	}

	protected void Tick() {
		FlushBursts();

		if (m_SavePath != "" && m_JournalEntries >= JOURNAL_MAX) Save(m_SavePath); //Rare: once per JOURNAL_MAX entries

		if (m_Subscribers.Count() == 0) return;

		TIntArray gone = new TIntArray;
		foreach (int id, ZBerryLogSubscriber subscriber : m_Subscribers) {
			bool connected = !GetGame().IsMultiplayer() || subscriber.Identity;
			if (!connected || (GetGame().IsMultiplayer() && !m_Auth.HasPermission(subscriber.Identity, ZBerryPermissions.LOGS_VIEW))) {
				gone.Insert(id); //Gets everything again if they regain the right
				continue;
			}
			SendEvents(subscriber);
			SendDamage(subscriber);
			subscriber.Reset = false;
		}
		foreach (int goneId : gone) m_Subscribers.Remove(goneId);
	}

	protected void SendEvents(ZBerryLogSubscriber subscriber) {
		int firstId = m_NextEventId - m_Events.Count();
		if (subscriber.NextEventId < firstId) subscriber.NextEventId = firstId; //Fell out of the buffer meanwhile
		if (subscriber.NextEventId >= m_NextEventId && !subscriber.Reset) return;

		ZBerryLogEventArray batch = new ZBerryLogEventArray;
		for (int i = subscriber.NextEventId - firstId; i < m_Events.Count() && batch.Count() < BATCH_EVENTS; ++i) {
			batch.Insert(m_Events[i]);
		}
		subscriber.NextEventId += batch.Count();
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LogEvents, new Param2<bool, ref ZBerryLogEventArray>(subscriber.Reset, batch), true, subscriber.Identity );
	}

	protected void SendDamage(ZBerryLogSubscriber subscriber) {
		int firstSeq = m_NextDamageSeq - GetDamageCount();
		if (subscriber.NextDamageSeq < firstSeq) subscriber.NextDamageSeq = firstSeq;
		if (subscriber.NextDamageSeq >= m_NextDamageSeq && !subscriber.Reset) return;
		if (subscriber.Reset) {
			subscriber.SentPlayers = 0;
			subscriber.SentWeapons = 0;
		}

		TIntArray records = new TIntArray;
		while (subscriber.NextDamageSeq < m_NextDamageSeq && records.Count() < BATCH_DAMAGE * DAMAGE_INTS) {
			int slot = (subscriber.NextDamageSeq % MAX_DAMAGE) * DAMAGE_INTS;
			for (int k = 0; k < DAMAGE_INTS; ++k) records.Insert(m_Damage[slot + k]);
			++subscriber.NextDamageSeq;
		}

		//New table entries first, so the client can resolve the records' indices
		TStringArray players = new TStringArray;
		for (int p = subscriber.SentPlayers; p < m_Players.Count(); ++p) players.Insert(m_Players[p]);
		TStringArray weapons = new TStringArray;
		for (int w = subscriber.SentWeapons; w < m_Weapons.Count(); ++w) weapons.Insert(m_Weapons[w]);
		subscriber.SentPlayers = m_Players.Count();
		subscriber.SentWeapons = m_Weapons.Count();

		GetZBerryRPC().SendRPC( ZBerryRPCOp.LogDamage, new Param4<bool, ref TStringArray, ref TStringArray, ref TIntArray>(subscriber.Reset, players, weapons, records), true, subscriber.Identity );
	}

	//---------------------------------------------------------------- Server: persistence

	//Server, once at startup (before anyone joins): the last run's log, folded into a fresh snapshot, then the
	//startup event. Journals from here on
	void StartPersisting(string path) {
		Load(path);
		m_SavePath = path;
		Save(path);

		string text = "Server started (ZomBerry " + ZBerryVersion.GetFullVersion() + ")";
		if (m_LoadedFile) {
			text += ": restored " + m_Events.Count() + " events and " + GetDamageCount() + " damage records";
			if (!m_LoadedCleanStop) text += "; the previous run didn't shut down cleanly";
		}
		ZomberryBase.LogEventRaw(ZBerryEventType.SERVER, "", "", "", "", text);
	}

	//Server, from MissionServer.OnMissionFinish: open bursts become records, then the last snapshot
	void OnServerStopping() {
		if (m_SavePath == "") return;

		FlushBursts(true);
		ZomberryBase.LogEventRaw(ZBerryEventType.SERVER, "", "", "", "", "Server stopping");
		Save(m_SavePath, true);
	}

	//The snapshot: header (magic, version, generation, save time, clean stop), events field by field, the damage ring
	//as it is, the name tables. Written to path.tmp first, so a crash while writing keeps the last good file; the
	//journal it now holds is deleted afterwards
	bool Save(string path, bool cleanStop = false) {
		string tmpPath = path + ".tmp";
		FileSerializer file = new FileSerializer();
		if (!file.Open(tmpPath, FileMode.WRITE)) {
			ZomberryBase.Log( "ZomBerryEventLog", "ERROR: Can't write " + tmpPath + ", the event log snapshot isn't saved" );
			return false;
		}

		string magic = FILE_MAGIC;
		int version = FILE_VERSION;
		int generation = m_Generation + 1;
		int savedAt = ZBerryTime.Now();
		file.Write(magic);
		file.Write(version);
		file.Write(generation);
		file.Write(savedAt);
		file.Write(cleanStop);

		int eventCount = m_Events.Count();
		file.Write(m_NextEventId);
		file.Write(eventCount);
		foreach (ZBerryLogEvent logEvent : m_Events) WriteEvent(file, logEvent);

		//The whole ring in one write: faster than record by record, Load unwinds it
		int damageInts = DAMAGE_INTS;
		int ringSize = MAX_DAMAGE;
		int damageCount = GetDamageCount();
		file.Write(damageInts);
		file.Write(ringSize);
		file.Write(damageCount);
		file.Write(m_NextDamageSeq);
		file.Write(m_Damage);
		file.Write(m_Players);
		file.Write(m_Weapons);
		int endMark = END_MARK;
		file.Write(endMark);
		file.Close();

		//Deleted first in case CopyFile doesn't replace; Load falls back to the .tmp if a crash comes in between
		DeleteFile(path);
		if (!CopyFile(tmpPath, path)) {
			ZomberryBase.Log( "ZomBerryEventLog", "ERROR: Can't replace " + path + " with " + tmpPath );
			return false;
		}
		DeleteFile(tmpPath);
		m_Generation = generation;
		DeleteFile(path + JOURNAL_SUFFIX); //Left behind by a crash right here, it's ignored: its generation is older
		m_JournalEntries = 0;
		return true;
	}

	//False when there was a snapshot but it couldn't be read: it's kept as path.bad (with its journal) and the log
	//starts empty. Keeps the newest MAX_EVENTS/MAX_DAMAGE, and only the name table entries the kept records use
	bool Load(string path) {
		m_LoadedFile = false;
		m_LoadedJournal = 0;
		string source = path;
		if (!FileExist(source)) source = path + ".tmp";
		if (!FileExist(source)) return true; //First start

		FileSerializer file = new FileSerializer();
		bool loaded = file.Open(source, FileMode.READ);
		if (loaded) {
			loaded = ReadSnapshot(file);
			file.Close();
		}

		if (!loaded) {
			DeleteFile(path + ".bad");
			CopyFile(source, path + ".bad");
			if (FileExist(path + JOURNAL_SUFFIX)) {
				DeleteFile(path + JOURNAL_SUFFIX + ".bad");
				CopyFile(path + JOURNAL_SUFFIX, path + JOURNAL_SUFFIX + ".bad");
			}
			ZomberryBase.Log( "ZomBerryEventLog", "WARN: Can't read " + source + ", kept it as " + path + ".bad. The event log starts empty" );
			return false;
		}

		m_LoadedJournal = ReplayJournal(path + JOURNAL_SUFFIX);
		m_LoadedFile = true;
		ZomberryBase.Log( "ZomBerryEventLog", "INFO: Restored " + m_Events.Count() + " events and " + GetDamageCount() + " damage records from " + source + " (" + m_LoadedJournal + " of them from the journal)" );
		return true;
	}

	//Everything is read into locals first: the log only changes when the whole file was readable
	protected bool ReadSnapshot(FileSerializer file) {
		string magic = "";
		int version = 0;
		int generation = 0;
		int savedAt = 0;
		bool cleanStop = false;
		if (!file.Read(magic) || magic != FILE_MAGIC) return false;
		if (!file.Read(version) || version < 1 || version > FILE_VERSION) return false;
		if (!file.Read(generation) || !file.Read(savedAt) || !file.Read(cleanStop)) return false;

		int nextEventId = 0;
		int eventCount = 0;
		if (!file.Read(nextEventId) || !file.Read(eventCount) || eventCount < 0 || eventCount > 65536) return false; //Garbage: reads don't fail at the end
		ZBerryLogEventArray events = new ZBerryLogEventArray;
		for (int i = 0; i < eventCount; ++i) {
			ZBerryLogEvent logEvent = ReadEvent(file);
			if (!logEvent) return false;
			events.Insert(logEvent);
		}

		int damageInts = 0;
		int ringSize = 0;
		int damageCount = 0;
		int nextSeq = 0;
		if (!file.Read(damageInts) || !file.Read(ringSize) || !file.Read(damageCount) || !file.Read(nextSeq)) return false;
		if (damageInts != DAMAGE_INTS || ringSize <= 0 || ringSize > 1000000 || damageCount < 0 || damageCount > ringSize || nextSeq < damageCount) return false;

		TIntArray ring = new TIntArray;
		TStringArray players = new TStringArray;
		TStringArray weapons = new TStringArray;
		if (!file.Read(ring) || ring.Count() != ringSize * DAMAGE_INTS) return false;
		if (!file.Read(players) || !file.Read(weapons)) return false;
		int endMark = 0;
		if (!file.Read(endMark) || endMark != END_MARK) return false; //Cut short

		//Unwound oldest first, the newest MAX_DAMAGE only
		TIntArray records = new TIntArray;
		int firstSeq = nextSeq - Math.Min(damageCount, MAX_DAMAGE);
		for (int seq = firstSeq; seq < nextSeq; ++seq) {
			int slot = (seq % ringSize) * DAMAGE_INTS;
			for (int k = 0; k < DAMAGE_INTS; ++k) records.Insert(ring[slot + k]);
		}

		//The tables only grow while the server runs: without this they'd keep everyone from every earlier run
		TStringArray keptPlayers = new TStringArray;
		TStringArray keptWeapons = new TStringArray;
		map<int, int> playerRemap = new map<int, int>;
		map<int, int> weaponRemap = new map<int, int>;
		for (int r = 0; r < records.Count(); r += DAMAGE_INTS) {
			if (!RemapIndex(records, r + 1, players, keptPlayers, playerRemap)) return false;
			if (!RemapIndex(records, r + 2, players, keptPlayers, playerRemap)) return false;
			if (!RemapIndex(records, r + 5, weapons, keptWeapons, weaponRemap)) return false;
		}

		m_Events = new ZBerryLogEventArray;
		for (int e = Math.Max(0, events.Count() - MAX_EVENTS); e < events.Count(); ++e) m_Events.Insert(events[e]);
		m_NextEventId = nextEventId;
		if (m_Events.Count() > 0 && m_NextEventId <= m_Events[m_Events.Count() - 1].Id) m_NextEventId = m_Events[m_Events.Count() - 1].Id + 1;

		for (int d = 0; d < records.Count(); ++d) m_Damage[d] = records[d];
		m_NextDamageSeq = records.Count() / DAMAGE_INTS;

		m_Players = keptPlayers;
		m_PlayerIndex.Clear();
		for (int p = 0; p < m_Players.Count(); ++p) m_PlayerIndex.Set(GetEntryKey(m_Players[p]), p);
		m_Weapons = keptWeapons;
		m_WeaponIndex.Clear();
		for (int w = 0; w < m_Weapons.Count(); ++w) m_WeaponIndex.Set(m_Weapons[w], w);

		m_Generation = generation;
		m_LoadedCleanStop = cleanStop;
		return true;
	}

	//What was logged after the snapshot of this generation was written. A crash can cut the last entry short:
	//everything before it still counts. Returns the entries replayed
	protected int ReplayJournal(string journalPath) {
		if (!FileExist(journalPath)) return 0;

		FileSerializer file = new FileSerializer();
		if (!file.Open(journalPath, FileMode.READ)) return 0;

		string magic = "";
		int version = 0;
		int generation = 0;
		if (!file.Read(magic) || magic != JOURNAL_MAGIC || !file.Read(version) || version != FILE_VERSION || !file.Read(generation) || generation != m_Generation) {
			file.Close();
			return 0; //Stale (already in the snapshot) or not a journal
		}

		int replayed = 0;
		while (replayed < 1000000) { //Can't be reached by a real journal, it's rewritten every JOURNAL_MAX entries
			int kind = 0;
			file.Read(kind);
			if (kind == JOURNAL_EVENT) {
				ZBerryLogEvent logEvent = ReadEvent(file);
				if (!logEvent || !ReadEndMark(file)) break;
				InsertEvent(logEvent);
			} else if (kind == JOURNAL_DAMAGE) {
				if (!ReplayDamage(file)) break;
			} else {
				break; //The end, or an entry cut short
			}
			++replayed;
		}
		file.Close();
		return replayed;
	}

	protected static bool ReadEndMark(FileSerializer file) {
		int endMark = 0;
		return file.Read(endMark) && endMark == END_MARK;
	}

	protected bool ReplayDamage(FileSerializer file) {
		int time = 0;
		int hits = 0;
		int damage = 0;
		int distance = 0;
		string attacker = "";
		string victim = "";
		string weapon = "";
		if (!file.Read(time) || !file.Read(hits) || !file.Read(damage) || !file.Read(distance)) return false;
		if (!file.Read(attacker) || !file.Read(victim) || !file.Read(weapon)) return false;
		if (!ReadEndMark(file)) return false;

		int attackerIndex = GetTableIndex(m_Players, m_PlayerIndex, GetEntryKey(attacker), attacker);
		int victimIndex = GetTableIndex(m_Players, m_PlayerIndex, GetEntryKey(victim), victim);
		int weaponIndex = GetTableIndex(m_Weapons, m_WeaponIndex, weapon, weapon);
		StoreRecord(time, attackerIndex, victimIndex, hits, damage, weaponIndex, distance);
		return true;
	}

	//The player table's key for an entry from a file: the Steam64 id. Characters without one were keyed by their network
	//id, which doesn't survive a restart, so the entry itself stands in
	protected static string GetEntryKey(string entry) {
		int separator = entry.IndexOf("|");
		if (separator > 0) return entry.Substring(0, separator);
		return "entry:" + entry;
	}

	protected void JournalEvent(ZBerryLogEvent logEvent) {
		FileSerializer file = OpenJournal();
		if (!file) return;

		int kind = JOURNAL_EVENT;
		int endMark = END_MARK;
		file.Write(kind);
		WriteEvent(file, logEvent);
		file.Write(endMark);
		file.Close();
		++m_JournalEntries;
	}

	//Names instead of table indices: the tables are rebuilt on every load
	protected void JournalDamage(int time, int attackerIndex, int victimIndex, int hits, int damageTenths, int weaponIndex, int distance) {
		FileSerializer file = OpenJournal();
		if (!file) return;

		int kind = JOURNAL_DAMAGE;
		file.Write(kind);
		file.Write(time);
		file.Write(hits);
		file.Write(damageTenths);
		file.Write(distance);
		file.Write(GetPlayerEntry(attackerIndex));
		file.Write(GetPlayerEntry(victimIndex));
		file.Write(GetWeaponEntry(weaponIndex));
		int endMark = END_MARK;
		file.Write(endMark);
		file.Close();
		++m_JournalEntries;
	}

	//Open for appending, created with its header (the snapshot's generation) when there's none yet. NULL on failure
	protected FileSerializer OpenJournal() {
		string journalPath = m_SavePath + JOURNAL_SUFFIX;
		FileSerializer file = new FileSerializer();
		if (FileExist(journalPath)) {
			if (file.Open(journalPath, FileMode.APPEND)) return file;
		} else if (file.Open(journalPath, FileMode.WRITE)) {
			string magic = JOURNAL_MAGIC;
			int version = FILE_VERSION;
			int generation = m_Generation;
			file.Write(magic);
			file.Write(version);
			file.Write(generation);
			return file;
		}

		ZomberryBase.Log( "ZomBerryEventLog", "ERROR: Can't write " + journalPath + ", this entry won't survive a restart" );
		return NULL;
	}

	protected static void WriteEvent(FileSerializer file, ZBerryLogEvent logEvent) {
		file.Write(logEvent.Id);
		file.Write(logEvent.Time);
		file.Write(logEvent.Type);
		file.Write(logEvent.Flags);
		file.Write(logEvent.ActorId);
		file.Write(logEvent.ActorName);
		file.Write(logEvent.TargetId);
		file.Write(logEvent.TargetName);
		file.Write(logEvent.Text);
	}

	protected static ZBerryLogEvent ReadEvent(FileSerializer file) {
		int id = 0;
		int time = 0;
		int type = 0;
		int flags = 0;
		string actorId = "";
		string actorName = "";
		string targetId = "";
		string targetName = "";
		string text = "";
		if (!file.Read(id) || !file.Read(time) || !file.Read(type) || !file.Read(flags)) return NULL;
		if (!file.Read(actorId) || !file.Read(actorName) || !file.Read(targetId) || !file.Read(targetName) || !file.Read(text)) return NULL;

		ZBerryLogEvent logEvent = new ZBerryLogEvent;
		logEvent.Id = id;
		logEvent.Time = time;
		logEvent.Type = type;
		logEvent.Flags = flags;
		logEvent.ActorId = actorId;
		logEvent.ActorName = actorName;
		logEvent.TargetId = targetId;
		logEvent.TargetName = targetName;
		logEvent.Text = text;
		return logEvent;
	}

	//records[at] is an index into table: points it at the entry's place in kept instead (added on first use)
	protected static bool RemapIndex(TIntArray records, int at, TStringArray table, TStringArray kept, map<int, int> remap) {
		int oldIndex = records[at];
		if (oldIndex < 0 || oldIndex >= table.Count()) return false;

		int newIndex;
		if (!remap.Find(oldIndex, newIndex)) {
			newIndex = kept.Insert(table[oldIndex]);
			remap.Set(oldIndex, newIndex);
		}
		records[at] = newIndex;
		return true;
	}

	//Server, for tests
	int GetJournalEntries() {
		return m_JournalEntries;
	}

	//Server, for tests: journal to path's journal from now on, as StartPersisting does, without loading or the startup event
	void SetSavePath(string path) {
		m_SavePath = path;
	}

	//---------------------------------------------------------------- Client: the synced copy

	void OnLogEvents( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<bool, ref ZBerryLogEventArray> data; //Start over, events (oldest first)
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param2 ) return;

		if (data.param1) m_ClientEvents.Clear();
		foreach (ZBerryLogEvent logEvent : data.param2) m_ClientEvents.Insert(logEvent);
		int excess = m_ClientEvents.Count() - MAX_EVENTS;
		for (int i = 0; i < excess; ++i) m_ClientEvents.RemoveOrdered(0);
		OnEventsChanged.Invoke();
	}

	void OnLogDamage( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<bool, ref TStringArray, ref TStringArray, ref TIntArray> data; //Start over, new players, new weapons, records
		if (type != ZBerryCallType.Client) return;
		if ( !ctx.Read( data ) || !data.param2 || !data.param3 || !data.param4 ) return;

		if (data.param1) {
			m_ClientDamage.Clear();
			m_ClientPlayers.Clear();
			m_ClientWeapons.Clear();
		}
		m_ClientPlayers.InsertAll(data.param2);
		m_ClientWeapons.InsertAll(data.param3);
		m_ClientDamage.InsertAll(data.param4);

		//Trimmed in chunks: dropping one record at a time would shift the whole array every time
		int maxInts = MAX_DAMAGE * DAMAGE_INTS;
		if (m_ClientDamage.Count() > maxInts + 512 * DAMAGE_INTS) {
			TIntArray kept = new TIntArray;
			for (int i = m_ClientDamage.Count() - maxInts; i < m_ClientDamage.Count(); ++i) kept.Insert(m_ClientDamage[i]);
			m_ClientDamage = kept;
		}
		OnDamageChanged.Invoke();
	}

	//Client: oldest first
	ZBerryLogEventArray GetEvents() {
		return m_ClientEvents;
	}

	//Client: newest first. typeMask: bit per ZBerryEventType, search: text, name or Steam64 id
	ZBerryLogEventArray FilterEvents(int typeMask, string search) {
		ZBerryLogEventArray result = new ZBerryLogEventArray;
		search.ToLower();
		for (int i = m_ClientEvents.Count() - 1; i >= 0; --i) {
			ZBerryLogEvent logEvent = m_ClientEvents[i];
			if ((typeMask & (1 << logEvent.Type)) != 0 && logEvent.Matches(search)) result.Insert(logEvent);
		}
		return result;
	}

	//Client
	int GetClientDamageCount() {
		return m_ClientDamage.Count() / DAMAGE_INTS;
	}

	//Client: record index (0 oldest). False when out of range
	bool GetClientDamage(int index, out int time, out string attackerId, out string attackerName, out string victimId, out string victimName, out int hits, out float damage, out string weapon, out int distance) {
		int slot = index * DAMAGE_INTS;
		if (index < 0 || slot + DAMAGE_INTS > m_ClientDamage.Count()) return false;

		time = m_ClientDamage[slot];
		SplitPlayer(m_ClientPlayers, m_ClientDamage[slot + 1], attackerId, attackerName);
		SplitPlayer(m_ClientPlayers, m_ClientDamage[slot + 2], victimId, victimName);
		hits = m_ClientDamage[slot + 3];
		float damageTenths = m_ClientDamage[slot + 4];
		damage = damageTenths / 10.0;
		int weaponIndex = m_ClientDamage[slot + 5];
		weapon = "";
		if (weaponIndex >= 0 && weaponIndex < m_ClientWeapons.Count()) weapon = ZBerryLocText.Resolve(m_ClientWeapons[weaponIndex], true);
		distance = m_ClientDamage[slot + 6];
		return true;
	}

	//Client: who a record is between, as indices into the player table (for filtering without building strings)
	bool GetClientDamageParties(int index, out int attackerIndex, out int victimIndex) {
		int slot = index * DAMAGE_INTS;
		if (index < 0 || slot + DAMAGE_INTS > m_ClientDamage.Count()) return false;

		attackerIndex = m_ClientDamage[slot + 1];
		victimIndex = m_ClientDamage[slot + 2];
		return true;
	}

	int GetClientDamageTime(int index) {
		int slot = index * DAMAGE_INTS;
		if (index < 0 || slot + DAMAGE_INTS > m_ClientDamage.Count()) return 0;
		return m_ClientDamage[slot];
	}

	//Client: everyone who dealt or took player damage
	int GetClientPlayerCount() {
		return m_ClientPlayers.Count();
	}

	void GetClientPlayer(int index, out string id, out string name) {
		SplitPlayer(m_ClientPlayers, index, id, name);
	}

	//Client: the player table index of a Steam64 id, -1 when they're not in it
	int FindClientPlayer(string steamId) {
		if (steamId == "") return -1;
		string prefix = steamId + "|";
		for (int i = 0; i < m_ClientPlayers.Count(); ++i) {
			if (m_ClientPlayers[i].IndexOf(prefix) == 0) return i;
		}
		return -1;
	}

	protected static void SplitPlayer(TStringArray table, int index, out string id, out string name) {
		id = "";
		name = "";
		if (index < 0 || index >= table.Count()) return;

		string entry = table[index];
		int separator = entry.IndexOf("|");
		if (separator == -1) {
			name = entry;
			return;
		}
		id = entry.Substring(0, separator);
		name = entry.Substring(separator + 1, entry.Length() - separator - 1);
	}

	//Server, for tests
	TIntArray GetDamageRecord(int newestOffset) {
		TIntArray record = new TIntArray;
		int seq = m_NextDamageSeq - 1 - newestOffset;
		if (seq < 0 || seq < m_NextDamageSeq - GetDamageCount()) return record;

		int slot = (seq % MAX_DAMAGE) * DAMAGE_INTS;
		for (int k = 0; k < DAMAGE_INTS; ++k) record.Insert(m_Damage[slot + k]);
		return record;
	}

	string GetPlayerEntry(int index) {
		if (index < 0 || index >= m_Players.Count()) return "";
		return m_Players[index];
	}

	string GetWeaponEntry(int index) {
		if (index < 0 || index >= m_Weapons.Count()) return "";
		return m_Weapons[index];
	}
};

static ref ZBerryEventLog g_ZBerryEventLog;

static ZBerryEventLog GetZBerryEventLog() {
	if (!g_ZBerryEventLog) {
		g_ZBerryEventLog = new ZBerryEventLog();
		ZBerryPlayerFeed.OnPlayerKilled.Insert(ZBerryEventLog.OnPlayerKilled);
		ZBerryPlayerFeed.OnPlayerHit.Insert(ZBerryEventLog.OnPlayerHit);
	}

	return g_ZBerryEventLog;
}
