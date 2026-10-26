//Client side of loadouts: the admin's own (ZomBerryMyLoadouts.json, in the client's profile) and what the server says
//about the shared ones. The Spawn tab listens to OnChanged
class ZBerryClientLoadouts {
	static const string LOCAL_FILE = "ZomBerryMyLoadouts.json";
	static const int MAX_LOCAL = 100;

	ref ScriptInvoker OnChanged = new ScriptInvoker; //The lists or the saved original
	ref ScriptInvoker OnDetailsChanged = new ScriptInvoker; //A shared loadout's or a player's items arrived
	ref ScriptInvoker OnDeathLoadout = new ScriptInvoker; //(int eventId): the server answered RequestDeath

	protected ref ZBerryLoadoutsConfig m_Local = new ZBerryLoadoutsConfig;
	protected bool m_LocalLoaded;
	protected bool m_LocalWritable = true; //False while the file is broken: it is never overwritten
	protected ref ZBerryLoadoutHeaderArray m_Shared = new ZBerryLoadoutHeaderArray;
	protected ref map<string, ref ZBerryLoadout> m_Details = new map<string, ref ZBerryLoadout>; //Shared loadouts' items, by key
	protected bool m_OriginalSaved; //The server keeps the gear from before the first try

	//Players' current gear (RequestPlayer), by session id: the gear, or why it can't be copied
	protected ref map<int, ref ZBerryLoadout> m_PlayerGear = new map<int, ref ZBerryLoadout>;
	protected ref map<int, string> m_PlayerReasons = new map<int, string>;

	//The last death asked about (RequestDeath)
	protected int m_DeathEventId = -1;
	protected bool m_DeathAnswered;
	protected string m_DeathReason; //Why it isn't kept, "" if it is
	protected ref ZBerryLoadout m_DeathLoadout;
	protected int m_DeathVictimState; //ZBerryVictimState

	void ZBerryClientLoadouts() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Loadouts, "Loadouts", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutDetails, "LoadoutDetails", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutCaptured, "LoadoutCaptured", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.DeathLoadout, "DeathLoadout", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutPlayer, "LoadoutPlayer", this, ZBerrySPExec.Client );
	}

	protected string GetPath() {
		return ZomberryBase.GetConfig().GetConfigDir() + LOCAL_FILE;
	}

	protected void LoadLocal() {
		if (m_LocalLoaded) return;
		m_LocalLoaded = true;
		if (!FileExist(GetPath())) return;

		ZBerryLoadoutsConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryLoadoutsConfig>.LoadFile(GetPath(), loaded, loadError) || !loaded) {
			m_LocalWritable = false;
			ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: " + LOCAL_FILE + " can't be read, no loadouts can be saved until it's fixed: " + loadError );
			return;
		}

		m_Local = loaded;
		if (!m_Local.Loadouts) m_Local.Loadouts = new ZBerryLoadoutArray;
		for (int i = 0; i < m_Local.Loadouts.Count(); ) {
			ZBerryLoadout loadout = m_Local.Loadouts[i];
			if (!ZBerryLoadoutKit.Sanitize(loadout) || loadout.Name == "" || ZBerryLoadoutKit.Find(m_Local.Loadouts, loadout.Name) != loadout) {
				m_Local.Loadouts.RemoveOrdered(i);
				continue;
			}
			++i;
		}
	}

	protected bool WriteLocal() {
		if (!m_LocalWritable) {
			ZBerryUIUtils.Message(LOCAL_FILE + " is broken, see your ZomBerry log");
			return false;
		}

		string saveError;
		if (JsonFileLoader<ZBerryLoadoutsConfig>.SaveFile(GetPath(), m_Local, saveError)) return true;

		ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: Can't save " + LOCAL_FILE + ": " + saveError );
		ZBerryUIUtils.Message("Can't save " + LOCAL_FILE + ", see your ZomBerry log");
		return false;
	}

	ZBerryLoadoutArray GetLocal() {
		LoadLocal();
		return m_Local.Loadouts;
	}

	ZBerryLoadout FindLocal(string name) {
		return ZBerryLoadoutKit.Find(GetLocal(), name);
	}

	ZBerryLoadoutHeaderArray GetShared() {
		return m_Shared;
	}

	ZBerryLoadoutHeader FindShared(string name) {
		string key = ZBerryLoadoutKit.GetKey(name);
		foreach (ZBerryLoadoutHeader header : m_Shared) {
			if (ZBerryLoadoutKit.GetKey(header.LoadoutName) == key) return header;
		}
		return NULL;
	}

	//NULL until the server sent it (RequestDetails)
	ZBerryLoadout GetSharedDetails(string name) {
		return m_Details.Get(ZBerryLoadoutKit.GetKey(name));
	}

	bool IsOriginalSaved() {
		return m_OriginalSaved;
	}

	void RequestList() {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutsRequest, NULL, true );
	}

	void RequestDetails(string name) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutDetailsRequest, new Param1<string>(name), true );
	}

	//The server sends the admin's gear back (LoadoutCaptured), it's saved then
	void SaveCurrentGear(string name) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutCapture, new Param1<string>(name), true );
	}

	void TryLocal(string name) {
		ZBerryLoadout loadout = FindLocal(name);
		if (!loadout) return;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutTry, new Param3<bool, string, ref ZBerryLoadout>(false, loadout.Name, loadout), true );
	}

	//Any loadout the admin has the items of (their own, or a player's gear)
	void TryLoadout(ZBerryLoadout loadout) {
		if (!loadout) return;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutTry, new Param3<bool, string, ref ZBerryLoadout>(false, loadout.Name, loadout), true );
	}

	void TryShared(string name) {
		ZBerryLoadout empty = new ZBerryLoadout(name); //A local: a temporary in the Param would be freed before it's sent
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutTry, new Param3<bool, string, ref ZBerryLoadout>(true, name, empty), true );
	}

	void Restore() {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutRestore, NULL, true );
	}

	void Share(string name) {
		ZBerryLoadout loadout = FindLocal(name);
		if (!loadout) return;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutShare, new Param1<ref ZBerryLoadout>(loadout), true );
	}

	void DeleteShared(string name) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutDelete, new Param1<string>(name), true );
	}

	//---------------------------------------------------------------- Other players

	//Asked again on every call: what they wear changes
	void RequestPlayer(int sessionId) {
		m_PlayerGear.Remove(sessionId);
		m_PlayerReasons.Remove(sessionId);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutPlayerRequest, new Param1<int>(sessionId), true );
	}

	//False while the server hasn't answered. loadout NULL with a reason when it can't be copied
	bool GetPlayerGear(int sessionId, out ZBerryLoadout loadout, out string reason) {
		if (!m_PlayerReasons.Contains(sessionId)) return false;

		reason = m_PlayerReasons.Get(sessionId);
		loadout = m_PlayerGear.Get(sessionId);
		return true;
	}

	void LoadoutPlayer( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param3<int, string, ref ZBerryLoadout> data; //Session id, why it can't be copied, the gear
		if (type != ZBerryCallType.Client || !ctx.Read(data)) return;

		ZBerryLoadout loadout = data.param3;
		if (data.param2 != "" || !loadout) {
			m_PlayerGear.Remove(data.param1);
		} else {
			if (!loadout.Items) loadout.Items = new ZBerryLoadoutItemArray;
			ZBerryLoadoutKit.Sanitize(loadout);
			m_PlayerGear.Set(data.param1, loadout);
		}
		m_PlayerReasons.Set(data.param1, data.param2);
		OnDetailsChanged.Invoke();
	}

	//shared: by name, the server has it. Otherwise the loadout's items are sent
	void Give(bool shared, string name, ZBerryLoadout loadout, TIntArray sessionIds) {
		ZBerryLoadout sent = loadout;
		if (shared || !sent) sent = new ZBerryLoadout(name); //A local: a temporary in the Param would be freed before it's sent
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutGive, new Param4<bool, string, ref ZBerryLoadout, ref TIntArray>(shared, name, sent, sessionIds), true );
	}

	void Undo(TIntArray sessionIds) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.LoadoutUndo, new Param1<ref TIntArray>(sessionIds), true );
	}

	//---------------------------------------------------------------- Deaths (Logs tab)

	void RequestDeath(int eventId) {
		m_DeathEventId = eventId;
		m_DeathAnswered = false;
		m_DeathLoadout = NULL;
		GetZBerryRPC().SendRPC( ZBerryRPCOp.DeathLoadoutRequest, new Param1<int>(eventId), true );
	}

	//False while the server hasn't answered for this event
	bool GetDeath(int eventId, out string reason, out ZBerryLoadout loadout, out int victimState) {
		if (eventId != m_DeathEventId || !m_DeathAnswered) return false;

		reason = m_DeathReason;
		loadout = m_DeathLoadout;
		victimState = m_DeathVictimState;
		return true;
	}

	void TryDeath(int eventId) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.DeathLoadoutTry, new Param1<int>(eventId), true );
	}

	void RestoreDeath(int eventId) {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.DeathLoadoutRestore, new Param1<int>(eventId), true );
	}

	void DeathLoadout( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<int, string, ref ZBerryLoadout, int> data; //Event id, why it isn't kept, the gear, ZBerryVictimState
		if (type != ZBerryCallType.Client || !ctx.Read(data) || data.param1 != m_DeathEventId) return;

		m_DeathAnswered = true;
		m_DeathReason = data.param2;
		m_DeathLoadout = data.param3;
		if (m_DeathLoadout) {
			if (!m_DeathLoadout.Items) m_DeathLoadout.Items = new ZBerryLoadoutItemArray;
			ZBerryLoadoutKit.Sanitize(m_DeathLoadout);
		}
		m_DeathVictimState = data.param4;
		OnDeathLoadout.Invoke(m_DeathEventId);
	}

	bool DeleteLocal(string name) {
		ZBerryLoadout loadout = FindLocal(name);
		if (!loadout) return false;

		int index = m_Local.Loadouts.Find(loadout);
		m_Local.Loadouts.RemoveOrdered(index);
		if (!WriteLocal()) {
			m_Local.Loadouts.InsertAt(loadout, index);
			return false;
		}
		OnChanged.Invoke();
		return true;
	}

	void Loadouts( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param2<bool, ref ZBerryLoadoutHeaderArray> data; //Original gear saved, shared loadouts
		if (type != ZBerryCallType.Client || !ctx.Read(data)) return;

		m_OriginalSaved = data.param1;
		m_Shared = data.param2;
		if (!m_Shared) m_Shared = new ZBerryLoadoutHeaderArray;
		m_Details.Clear(); //Any of them may have changed
		OnChanged.Invoke();
	}

	void LoadoutDetails( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryLoadout> data;
		if (type != ZBerryCallType.Client || !ctx.Read(data) || !data.param1) return;

		m_Details.Set(ZBerryLoadoutKit.GetKey(data.param1.Name), data.param1);
		OnDetailsChanged.Invoke();
	}

	//Saved under its name, replacing the one of the same name
	void LoadoutCaptured( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryLoadout> data;
		if (type != ZBerryCallType.Client || !ctx.Read(data) || !ZBerryLoadoutKit.Sanitize(data.param1)) return;

		SaveLocal(data.param1);
	}

	//A copy among the admin's own loadouts, e.g. the gear of a death. Replaces the one of the same name
	void SaveCopy(ZBerryLoadout source, string name) {
		ZBerryLoadout loadout = ZBerryLoadoutKit.Copy(source); //The overview keeps showing the original
		if (!loadout || !ZBerryLoadoutKit.Sanitize(loadout)) {
			ZBerryUIUtils.Message("Loadout not saved: there's nothing in it");
			return;
		}
		loadout.Name = ZBerryLoadoutKit.CleanName(name);
		SaveLocal(loadout);
	}

	protected void SaveLocal(ZBerryLoadout loadout) {
		loadout.OwnerId = "";
		loadout.OwnerName = "";
		ZBerryLoadoutArray mine = GetLocal();
		ZBerryLoadout previous = FindLocal(loadout.Name);
		int index = mine.Find(previous);
		if (index == -1 && mine.Count() >= MAX_LOCAL) {
			ZBerryUIUtils.Message("Loadout not saved: you already have " + MAX_LOCAL + ", delete some first");
			return;
		}

		if (index == -1) {
			mine.Insert(loadout);
		} else {
			loadout.Name = previous.Name;
			mine.Set(index, loadout);
		}
		if (!WriteLocal()) {
			if (index == -1) {
				mine.Remove(mine.Count() - 1);
			} else {
				mine.Set(index, previous);
			}
			return;
		}

		string verb = "Saved ";
		if (index != -1) verb = "Replaced ";
		ZBerryUIUtils.Message(verb + loadout.Name + " (" + loadout.CountAll() + " items)");
		OnChanged.Invoke();
	}
};

static ref ZBerryClientLoadouts g_ZBerryClientLoadouts;

//Client and singleplayer only
static ZBerryClientLoadouts GetZBerryClientLoadouts() {
	if (!g_ZBerryClientLoadouts) g_ZBerryClientLoadouts = new ZBerryClientLoadouts();

	return g_ZBerryClientLoadouts;
}
