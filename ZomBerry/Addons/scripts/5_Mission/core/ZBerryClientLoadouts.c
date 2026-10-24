//Client side of loadouts: the admin's own (ZomBerryMyLoadouts.json, in the client's profile) and what the server says
//about the shared ones. The Spawn tab listens to OnChanged
class ZBerryClientLoadouts {
	static const string LOCAL_FILE = "ZomBerryMyLoadouts.json";
	static const int MAX_LOCAL = 100;

	ref ScriptInvoker OnChanged = new ScriptInvoker;

	protected ref ZBerryLoadoutsConfig m_Local = new ZBerryLoadoutsConfig;
	protected bool m_LocalLoaded;
	protected bool m_LocalWritable = true; //False while the file is broken: it is never overwritten
	protected ref ZBerryLoadoutHeaderArray m_Shared = new ZBerryLoadoutHeaderArray;
	protected ref map<string, ref ZBerryLoadout> m_Details = new map<string, ref ZBerryLoadout>; //Shared loadouts' items, by key
	protected bool m_OriginalSaved; //The server keeps the gear from before the first try

	void ZBerryClientLoadouts() {
		GetZBerryRPC().AddRPC( ZBerryRPCOp.Loadouts, "Loadouts", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutDetails, "LoadoutDetails", this, ZBerrySPExec.Client );
		GetZBerryRPC().AddRPC( ZBerryRPCOp.LoadoutCaptured, "LoadoutCaptured", this, ZBerrySPExec.Client );
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

	protected bool SaveLocal() {
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
			if (ZBerryLoadoutKit.GetKey(header.Name) == key) return header;
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

	bool DeleteLocal(string name) {
		ZBerryLoadout loadout = FindLocal(name);
		if (!loadout) return false;

		int index = m_Local.Loadouts.Find(loadout);
		m_Local.Loadouts.RemoveOrdered(index);
		if (!SaveLocal()) {
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
		OnChanged.Invoke();
	}

	//Saved under its name, replacing the one of the same name
	void LoadoutCaptured( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param1<ref ZBerryLoadout> data;
		if (type != ZBerryCallType.Client || !ctx.Read(data) || !ZBerryLoadoutKit.Sanitize(data.param1)) return;

		ZBerryLoadout loadout = data.param1;
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
		if (!SaveLocal()) {
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
