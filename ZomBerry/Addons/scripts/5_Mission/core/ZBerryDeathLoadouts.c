//One death's gear, under the id of its kill event in the log. The loadout's OwnerId and OwnerName are the victim's
class ZBerryDeathLoadout {
	int EventId;
	ref ZBerryLoadout Loadout;

	void ZBerryDeathLoadout(int eventId = 0, ZBerryLoadout loadout = NULL) {
		EventId = eventId;
		Loadout = loadout;
	}
};

typedef array<ref ZBerryDeathLoadout> ZBerryDeathLoadoutArray;

//Whether a death's victim can get their gear back right now. Values travel to clients
class ZBerryVictimState {
	static const int OFFLINE = 0;
	static const int ALIVE = 1;
	static const int DEAD = 2;
};

//Root of ZomBerryDeathLoadouts.json
class ZBerryDeathLoadoutsConfig {
	ref ZBerryDeathLoadoutArray Deaths = new ZBerryDeathLoadoutArray;
};

//What players wore and held when they died: their last MAX_PER_PLAYER deaths, each under its kill event, for the Logs
//tab's loadout overview. Kept in memory; written to disk only when the server stops cleanly (no writes while it runs),
//and read back and deleted at the next start
class ZBerryDeathLoadouts {
	static const string FILE_NAME = "ZomBerryDeathLoadouts.json";
	static const int MAX_PER_PLAYER = 5;
	static const int MAX_TOTAL = 500;

	protected ref ZBerryDeathLoadoutArray m_Deaths = new ZBerryDeathLoadoutArray; //Oldest first
	protected ref map<string, ref ZBerryLoadout> m_Pending = new map<string, ref ZBerryLoadout>; //Captured, kill not logged yet

	void ZBerryDeathLoadouts() {
		ZBerryPlayerFeed.OnPlayerDying.Insert(OnPlayerDying);
	}

	void ~ZBerryDeathLoadouts() {
		ZBerryPlayerFeed.OnPlayerDying.Remove(OnPlayerDying);
	}

	//"" without one (AI and characters without a player)
	string GetKey(PlayerBase player) {
		if (!GetGame().IsMultiplayer()) return "singleplayer";
		PlayerIdentity identity = ZBGetIdentity(player);
		if (!identity) return "";
		return identity.GetPlainId();
	}

	//Before vanilla's death handling: the hotbar is still set. The kill is logged right after (Attach)
	void OnPlayerDying(PlayerBase victim, Object killer) {
		if (!GetGame().IsServer() || !victim) return;

		string key = GetKey(victim);
		if (key == "") return;

		ZBerryLoadout loadout = ZBerryLoadoutKit.Capture(victim, "death");
		loadout.OwnerId = key;
		loadout.OwnerName = "Singleplayer";
		if (ZBGetIdentity(victim)) loadout.OwnerName = ZBGetIdentity(victim).GetName();
		loadout.Name = ZBerryLoadoutKit.CleanName(loadout.OwnerName + "'s death");
		m_Pending.Set(key, loadout);
	}

	//The victim's kill event was logged: their gear is kept under its id
	void Attach(PlayerBase victim, ZBerryLogEvent killEvent) {
		if (!victim || !killEvent) return;

		string key = GetKey(victim);
		ZBerryLoadout loadout = m_Pending.Get(key);
		if (!loadout) return;

		m_Pending.Remove(key);
		Add(new ZBerryDeathLoadout(killEvent.EventId, loadout));
	}

	void Add(ZBerryDeathLoadout death) {
		m_Deaths.Insert(death);

		int count = 0;
		for (int i = m_Deaths.Count() - 1; i >= 0; --i) { //Newest first: the ones past the cap are the oldest
			if (m_Deaths[i].Loadout.OwnerId != death.Loadout.OwnerId) continue;
			++count;
			if (count > MAX_PER_PLAYER) m_Deaths.RemoveOrdered(i);
		}
		while (m_Deaths.Count() > MAX_TOTAL) m_Deaths.RemoveOrdered(0);
	}

	//NULL when it isn't kept (an older death, from before a crash, or not a player's)
	ZBerryDeathLoadout Find(int eventId) {
		foreach (ZBerryDeathLoadout death : m_Deaths) {
			if (death.EventId == eventId) return death;
		}
		return NULL;
	}

	int Count() {
		return m_Deaths.Count();
	}

	//The victim's character, if they're online. NULL otherwise
	static PlayerBase FindVictim(ZBerryDeathLoadout death) {
		if (!GetGame().IsMultiplayer()) return PlayerBase.Cast(GetGame().GetPlayer());

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			PlayerIdentity identity = ZBGetIdentity(man);
			if (identity && identity.GetPlainId() == death.Loadout.OwnerId) return PlayerBase.Cast(man);
		}
		return NULL;
	}

	static int GetVictimState(ZBerryDeathLoadout death) {
		PlayerBase victim = FindVictim(death);
		if (!victim) return ZBerryVictimState.OFFLINE;
		if (!victim.IsAlive()) return ZBerryVictimState.DEAD;
		return ZBerryVictimState.ALIVE;
	}

	//Server, at a clean stop
	void Save(string dir) {
		string path = dir + FILE_NAME;
		if (m_Deaths.Count() == 0) {
			if (FileExist(path)) DeleteFile(path);
			return;
		}

		ZBerryDeathLoadoutsConfig config = new ZBerryDeathLoadoutsConfig;
		config.Deaths = m_Deaths;
		string saveError;
		if (!JsonFileLoader<ZBerryDeathLoadoutsConfig>.SaveFile(path, config, saveError)) {
			ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: Can't save " + FILE_NAME + ": " + saveError );
			return;
		}
		ZomberryBase.Log( "ZomBerryLoadouts", "INFO: Saved " + m_Deaths.Count() + " death loadouts to " + FILE_NAME );
	}

	//Server, at startup after the event log: the deaths saved at the last clean stop. The file is deleted, so after a
	//crash the next start doesn't bring back deaths the run in between never saved. Deaths whose kill event the log
	//doesn't have (it was reset) are dropped
	void Load(string dir, int nextEventId) {
		string path = dir + FILE_NAME;
		if (!FileExist(path)) return;

		ZBerryDeathLoadoutsConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryDeathLoadoutsConfig>.LoadFile(path, loaded, loadError) || !loaded || !loaded.Deaths) {
			ZomberryBase.Log( "ZomBerryLoadouts", "ERROR: " + FILE_NAME + " can't be read, the deaths before the restart are lost: " + loadError );
			DeleteFile(path);
			return;
		}

		foreach (ZBerryDeathLoadout death : loaded.Deaths) {
			if (!death || !death.Loadout || death.EventId <= 0 || death.EventId >= nextEventId) continue;
			if (death.Loadout.OwnerId == "" || Find(death.EventId)) continue;
			if (!death.Loadout.Items) death.Loadout.Items = new ZBerryLoadoutItemArray;
			ZBerryLoadoutKit.Sanitize(death.Loadout); //Empty is fine: they may have died naked
			Add(death);
		}
		DeleteFile(path);
		ZomberryBase.Log( "ZomBerryLoadouts", "INFO: Loaded " + m_Deaths.Count() + " death loadouts from " + FILE_NAME );
	}
};

static ref ZBerryDeathLoadouts g_ZBerryDeathLoadouts; //Server and singleplayer
