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

	static string GetName(int type) {
		TStringArray names = {"", "Function (self)", "Function (player)", "Function (server)", "Spawn", "Teleport", "Permissions", "Kick", "Ban", "Unban", "Join refused", "Login", "Logout", "Kill", "Auth", "Refused"};
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

	//As the log file writes it: players as "Name (Steam64 id)"
	string Format() {
		string text = Text;
		text.Replace("{actor}", DescribeParty(ActorName, ActorId));
		text.Replace("{target}", DescribeParty(TargetName, TargetId));
		return text;
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
//then only what's new
class ZBerryEventLog {
	static const int MAX_EVENTS = 2048;
	static const int MAX_DAMAGE = 10486;
	static const int DAMAGE_INTS = 7; //Time, attacker, victim, hits, damage x10, weapon, distance
	static const int BURST_MS = 3000;
	static const int TICK_MS = 250;
	static const int BATCH_EVENTS = 64; //Per client and tick
	static const int BATCH_DAMAGE = 256;

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

	//Client
	protected ref ZBerryLogEventArray m_ClientEvents = new ZBerryLogEventArray;
	protected ref TIntArray m_ClientDamage = new TIntArray; //Records, oldest first
	protected ref TStringArray m_ClientPlayers = new TStringArray;
	protected ref TStringArray m_ClientWeapons = new TStringArray;
	ref ScriptInvoker OnEventsChanged = new ScriptInvoker(); //No arguments
	ref ScriptInvoker OnDamageChanged = new ScriptInvoker(); //No arguments

	void ZBerryEventLog() {
		m_Damage.Resize(MAX_DAMAGE * DAMAGE_INTS);
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

		m_Events.Insert(logEvent);
		while (m_Events.Count() > MAX_EVENTS) m_Events.RemoveOrdered(0);
		return logEvent;
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
		int slot = (m_NextDamageSeq % MAX_DAMAGE) * DAMAGE_INTS;
		float damageTenths = burst.Damage * 10;
		m_Damage[slot] = burst.StartTime;
		m_Damage[slot + 1] = burst.AttackerIndex;
		m_Damage[slot + 2] = burst.VictimIndex;
		m_Damage[slot + 3] = burst.Hits;
		m_Damage[slot + 4] = Math.Round(damageTenths);
		m_Damage[slot + 5] = burst.WeaponIndex;
		m_Damage[slot + 6] = Math.Round(burst.Distance);
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

	//"{target} was killed by {actor} with KA-M from 154 m", "{target} was killed by an infected", "{target} died"
	static void OnPlayerKilled(PlayerBase victim, Object killer) {
		if (!GetGame().IsServer() || !victim) return;

		string where = " at " + victim.GetPosition().ToString();
		EntityAI killerEntity = EntityAI.Cast(killer);
		PlayerBase killerPlayer;
		if (killerEntity) killerPlayer = PlayerBase.Cast(killerEntity.GetHierarchyRootPlayer());

		if (killerPlayer && killerPlayer != victim) {
			string text = "{target} was killed by {actor}";
			if (killerEntity != killerPlayer) text += " with " + killerEntity.GetDisplayName();
			float distance = vector.Distance(victim.GetPosition(), killerPlayer.GetPosition());
			ZomberryBase.LogEventRaw(ZBerryEventType.KILL, GetSteamId(killerPlayer), GetName(killerPlayer), GetSteamId(victim), GetName(victim), text + " from " + Math.Round(distance) + " m" + where);
			return;
		}

		string cause = " died";
		if (killerEntity && killerEntity != victim) {
			cause = " was killed by " + killerEntity.GetDisplayName();
			if (killerEntity.IsInherited(ZombieBase)) cause = " was killed by an infected";
			if (killerEntity.IsInherited(Transport)) cause = " was run over by a " + killerEntity.GetDisplayName();
		}
		ZomberryBase.LogEventRaw(ZBerryEventType.KILL, "", "", GetSteamId(victim), GetName(victim), "{target}" + cause + where);
	}

	//Player against player only: infected, animals and falls aren't logged
	static void OnPlayerHit(PlayerBase victim, PlayerBase attacker, EntityAI source, float damage) {
		if (!GetGame().IsServer() || !victim || !attacker || victim == attacker) return;

		string weapon = "Fists";
		if (source && source != attacker) weapon = source.GetDisplayName();
		GetZBerryEventLog().AddHit(victim, attacker, weapon, damage);
	}

	void OnPlayerConnected(PlayerIdentity identity) {
		if (!identity) return;
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
		if (weaponIndex >= 0 && weaponIndex < m_ClientWeapons.Count()) weapon = m_ClientWeapons[weaponIndex];
		distance = m_ClientDamage[slot + 6];
		return true;
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
