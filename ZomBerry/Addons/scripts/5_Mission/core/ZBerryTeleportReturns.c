//Where one player was before a teleport, kept under the teleport's log event so it can be reverted from the Logs tab
class ZBerryTeleportRecord {
	string SteamId;
	string Name;
	ref TIntArray CharacterId; //GetPersistentID: a new character after a death doesn't go back to the old one's spot
	vector From;
	vector To; //Where the move put them
	bool Reverted;

	void ZBerryTeleportRecord(string steamId = "", string name = "", TIntArray characterId = NULL, vector from = "0 0 0") {
		SteamId = steamId;
		Name = name;
		CharacterId = characterId;
		From = from;
	}
};

typedef array<ref ZBerryTeleportRecord> ZBerryTeleportRecordArray;

class ZBerryTeleportEvent {
	int EventId;
	ref ZBerryTeleportRecordArray Records;

	void ZBerryTeleportEvent(int eventId = 0, ZBerryTeleportRecordArray records = NULL) {
		EventId = eventId;
		Records = records;
	}
};

//One player of a teleport as the Logs tab shows it. Travels to clients
class ZBerryTeleportRow {
	string SteamId;
	string Name;
	vector From;
	int Status; //ZBerryTeleportStatus

	void ZBerryTeleportRow(string steamId = "", string name = "", vector from = "0 0 0", int status = 0) {
		SteamId = steamId;
		Name = name;
		From = from;
		Status = status;
	}
};

typedef array<ref ZBerryTeleportRow> ZBerryTeleportRowArray;

//Whether a recorded player can be moved back now. Values travel to clients
class ZBerryTeleportStatus {
	static const int PENDING = 0;
	static const int OFFLINE = 1;
	static const int IN_VEHICLE = 2;
	static const int DIED = 3; //A new character since: not moved back
	static const int REVERTED = 4;
};

//Root of ZomBerryTeleportReturns.json
class ZBerryTeleportReturnsConfig {
	ref array<ref ZBerryTeleportEvent> Events = new array<ref ZBerryTeleportEvent>;
};

//Server: the last MAX_EVENTS teleports' records, by event id. Kept in memory; written only when the server stops cleanly,
//and read back and deleted at the next start (as ZBerryDeathLoadouts)
class ZBerryTeleportReturns {
	static const string FILE_NAME = "ZomBerryTeleportReturns.json";
	static const int MAX_EVENTS = 200;
	static const int MAX_PLAYERS = 100; //Per event
	static const string NOT_KEPT = "This teleport can't be reverted any more";

	protected ref array<ref ZBerryTeleportEvent> m_Events = new array<ref ZBerryTeleportEvent>; //Oldest first

	//After the move: the player's position now is where it put them
	static ZBerryTeleportRecord MakeRecord(PlayerBase player, vector from) {
		TIntArray characterId = ZomberryStockFunctions.GetCharacterId(player); //Local: a temporary argument may be freed early
		ZBerryTeleportRecord record = new ZBerryTeleportRecord(ZomberryStockFunctions.GetSteamId(player), ZomberryStockFunctions.GetPlayerName(player), characterId, from);
		record.To = ZomberryStockFunctions.GetPosSafe(player);
		return record;
	}

	//Where a teleport went, for its log event: the one player's destination, or the middle of several (a ring's centre)
	static bool GetDestination(ZBerryTeleportRecordArray records, out vector destination) {
		destination = "0 0 0";
		if (!records || records.Count() == 0) return false;
		vector sum = "0 0 0";
		foreach (ZBerryTeleportRecord record : records) sum = sum + record.To;
		float count = records.Count();
		destination = sum * (1.0 / count);
		return true;
	}

	void Attach(int eventId, ZBerryTeleportRecordArray records) {
		if (!records || records.Count() == 0) return;

		ZBerryTeleportRecordArray kept = new ZBerryTeleportRecordArray;
		for (int i = 0; i < records.Count() && i < MAX_PLAYERS; ++i) kept.Insert(records[i]);
		m_Events.Insert(new ZBerryTeleportEvent(eventId, kept));
		while (m_Events.Count() > MAX_EVENTS) m_Events.RemoveOrdered(0);
	}

	//NULL when it isn't kept (pushed out, from before a crash, or no teleport)
	ZBerryTeleportEvent Find(int eventId) {
		foreach (ZBerryTeleportEvent teleport : m_Events) {
			if (teleport.EventId == eventId) return teleport;
		}
		return NULL;
	}

	int Count() {
		return m_Events.Count();
	}

	//What the Logs tab shows of an event's players
	ZBerryTeleportRowArray GetRows(ZBerryTeleportEvent teleport) {
		ZBerryTeleportRowArray rows = new ZBerryTeleportRowArray;
		foreach (ZBerryTeleportRecord record : teleport.Records) rows.Insert(new ZBerryTeleportRow(record.SteamId, record.Name, record.From, GetStatus(record)));
		return rows;
	}

	//The player's character if they're online (detached bodies included), NULL otherwise
	protected PlayerBase FindOnline(string steamId) {
		if (steamId == "") return NULL;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			PlayerIdentity identity = ZBGetIdentity(man);
			if (identity && identity.GetPlainId() == steamId) return PlayerBase.Cast(man);
		}
		return NULL;
	}

	int GetStatus(ZBerryTeleportRecord record) {
		if (record.Reverted) return ZBerryTeleportStatus.REVERTED;
		PlayerBase player = FindOnline(record.SteamId);
		if (!player || !player.IsAlive()) return ZBerryTeleportStatus.OFFLINE;
		TIntArray characterId = ZomberryStockFunctions.GetCharacterId(player);
		if (!ZomberryStockFunctions.SameCharacter(record.CharacterId, characterId)) return ZBerryTeleportStatus.DIED;
		if (player.GetCommand_Vehicle()) return ZBerryTeleportStatus.IN_VEHICLE;
		return ZBerryTeleportStatus.PENDING;
	}

	//Moves the given players (Steam64 ids, empty: everyone) of an event back to where they were. Returns a record for
	//each player it moved, from where it moved them, for the revert's own event; summary is what to tell the admin.
	//Players in vehicles, offline or protected from the admin's rank stay pending; a new character is dropped
	ZBerryTeleportRecordArray Revert(PlayerIdentity admin, int eventId, TStringArray steamIds, out string summary) {
		ZBerryTeleportRecordArray moved = new ZBerryTeleportRecordArray;
		ZBerryTeleportEvent teleport = Find(eventId);
		if (!teleport) {
			summary = NOT_KEPT;
			return moved;
		}

		int alreadyBack = 0;
		int offline = 0;
		int inVehicles = 0;
		int protectedCount = 0;
		int died = 0;
		foreach (ZBerryTeleportRecord record : teleport.Records) {
			if (steamIds && steamIds.Count() > 0 && steamIds.Find(record.SteamId) == -1) continue;

			int status = GetStatus(record);
			if (status == ZBerryTeleportStatus.REVERTED) {
				++alreadyBack;
				continue;
			}
			if (status == ZBerryTeleportStatus.OFFLINE) {
				++offline;
				continue;
			}
			if (status == ZBerryTeleportStatus.IN_VEHICLE) {
				++inVehicles;
				continue;
			}
			if (status == ZBerryTeleportStatus.DIED) {
				++died;
				record.Reverted = true; //Never offered again
				continue;
			}

			PlayerBase player = FindOnline(record.SteamId);
			PlayerIdentity identity = ZBGetIdentity(player);
			if (admin && identity && !GetZomberryPermAPI().CanTarget(admin, identity, true)) {
				ZomberryBase.LogEvent( ZBerryEventType.REFUSED, admin, identity, "{actor} tried to teleport {target} back, whose group is protected from their rank" );
				++protectedCount;
				continue;
			}

			vector leftAt = ZomberryStockFunctions.GetPosSafe(player);
			if (!ZomberryStockFunctions.SetPosSafe(player, record.From)) {
				++inVehicles;
				continue;
			}
			record.Reverted = true;
			moved.Insert(MakeRecord(player, leftAt));
		}

		summary = "Teleported " + moved.Count() + " players back";
		if (alreadyBack > 0) summary += ", " + alreadyBack + " already back";
		if (inVehicles > 0) summary += ", " + inVehicles + " skipped (in vehicles)";
		if (protectedCount > 0) summary += ", " + protectedCount + " skipped (protected from your rank)";
		if (died > 0) summary += ", " + died + " forgotten (died meanwhile)";
		if (offline > 0) summary += ", " + offline + " offline";
		return moved;
	}

	//Server, at a clean stop
	void Save(string dir) {
		string path = dir + FILE_NAME;
		if (m_Events.Count() == 0) {
			if (FileExist(path)) DeleteFile(path);
			return;
		}

		ZBerryTeleportReturnsConfig config = new ZBerryTeleportReturnsConfig;
		config.Events = m_Events;
		string saveError;
		if (!JsonFileLoader<ZBerryTeleportReturnsConfig>.SaveFile(path, config, saveError)) {
			ZomberryBase.Log( "ZomBerryTeleports", "ERROR: Can't save " + FILE_NAME + ": " + saveError );
			return;
		}
		ZomberryBase.Log( "ZomBerryTeleports", "INFO: Saved " + m_Events.Count() + " revertable teleports to " + FILE_NAME );
	}

	//Server, at startup after the event log. The file is deleted, so after a crash the next start doesn't bring back
	//teleports the run in between never saved. Events the log doesn't have (it was reset) are dropped
	void Load(string dir, int nextEventId) {
		string path = dir + FILE_NAME;
		if (!FileExist(path)) return;

		ZBerryTeleportReturnsConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryTeleportReturnsConfig>.LoadFile(path, loaded, loadError) || !loaded || !loaded.Events) {
			ZomberryBase.Log( "ZomBerryTeleports", "ERROR: " + FILE_NAME + " can't be read, teleports from before the restart can't be reverted: " + loadError );
			DeleteFile(path);
			return;
		}

		foreach (ZBerryTeleportEvent teleport : loaded.Events) {
			if (!teleport || !teleport.Records || teleport.EventId <= 0 || teleport.EventId >= nextEventId || Find(teleport.EventId)) continue;
			ZBerryTeleportRecordArray records = new ZBerryTeleportRecordArray;
			foreach (ZBerryTeleportRecord record : teleport.Records) {
				if (!record || record.SteamId == "") continue;
				if (!record.CharacterId || record.CharacterId.Count() != 4) record.CharacterId = {0, 0, 0, 0};
				records.Insert(record);
			}
			Attach(teleport.EventId, records);
		}
		DeleteFile(path);
		ZomberryBase.Log( "ZomBerryTeleports", "INFO: Loaded " + m_Events.Count() + " revertable teleports from " + FILE_NAME );
	}
};

static ref ZBerryTeleportReturns g_ZBerryTeleportReturns; //Server and singleplayer
