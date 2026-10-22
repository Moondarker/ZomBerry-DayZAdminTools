//Seconds since 1970 (UTC) from the date functions: script has no unix clock of its own
class ZBerryTime {
	static int Now() {
		int year, month, day, hour, minute, second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		return FromDate(year, month, day, hour, minute, second);
	}

	//Days since 1970-01-01 of a proleptic Gregorian date (Howard Hinnant's days_from_civil)
	static int DaysFromCivil(int year, int month, int day) {
		if (month <= 2) year -= 1;

		int era = year / 400;
		if (year < 0) era = (year - 399) / 400;
		int yearOfEra = year - era * 400;
		int shiftedMonth = month - 3;
		if (month <= 2) shiftedMonth = month + 9;
		int dayOfYear = (153 * shiftedMonth + 2) / 5 + day - 1;
		int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
		return era * 146097 + dayOfEra - 719468;
	}

	static void CivilFromDays(int days, out int year, out int month, out int day) {
		days += 719468;
		int era = days / 146097;
		if (days < 0) era = (days - 146096) / 146097;
		int dayOfEra = days - era * 146097;
		int yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
		int dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
		int shiftedMonth = (5 * dayOfYear + 2) / 153;

		day = dayOfYear - (153 * shiftedMonth + 2) / 5 + 1;
		month = shiftedMonth + 3;
		if (shiftedMonth >= 10) month = shiftedMonth - 9;
		year = yearOfEra + era * 400;
		if (month <= 2) year += 1;
	}

	static int FromDate(int year, int month, int day, int hour = 0, int minute = 0, int second = 0) {
		int days = DaysFromCivil(year, month, day);
		return days * 86400 + hour * 3600 + minute * 60 + second;
	}

	//"2026-10-18 21:05 UTC"
	static string Format(int time) {
		int days = time / 86400;
		int secondsOfDay = time - days * 86400;
		int year, month, day;
		CivilFromDays(days, year, month, day);

		int hour = secondsOfDay / 3600;
		int minute = (secondsOfDay / 60) % 60;
		return year.ToString() + "-" + month.ToStringLen(2) + "-" + day.ToStringLen(2) + " " + hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + " UTC";
	}

	//Seconds this machine's clock is ahead of UTC (time zone and daylight saving), to the nearest quarter hour
	static int GetLocalOffset() {
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);
		int localNow = FromDate(year, month, day, hour, minute, second);
		int utcNow = Now();
		float difference = localNow - utcNow; //Typed locals: the same maths in one expression came out 0
		float quarters = difference / 900.0;
		int rounded = Math.Round(quarters);
		return rounded * 900;
	}

	//"21:05:33" in the local time of this machine (offset from GetLocalOffset)
	static string FormatLocalClock(int time, int offset) {
		int localTime = time + offset;
		int secondsOfDay = localTime % 86400;
		if (secondsOfDay < 0) secondsOfDay += 86400;
		int hour = secondsOfDay / 3600;
		int minute = (secondsOfDay / 60) % 60;
		int second = secondsOfDay % 60;
		return hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);
	}

	//"2026-10-18 21:05:33" in the local time of this machine (offset from GetLocalOffset)
	static string FormatLocal(int time, int offset) {
		int localTime = time + offset;
		int days = localTime / 86400;
		int year, month, day;
		CivilFromDays(days, year, month, day);
		return year.ToString() + "-" + month.ToStringLen(2) + "-" + day.ToStringLen(2) + " " + FormatLocalClock(time, offset);
	}

	//"3d 4h", "2h 5min", "45min"
	static string FormatDuration(int seconds) {
		if (seconds < 60) return "<1min";

		int days = seconds / 86400;
		int hours = (seconds / 3600) % 24;
		int minutes = (seconds / 60) % 60;
		if (days > 0) return days.ToString() + "d " + hours.ToString() + "h";
		if (hours > 0) return hours.ToString() + "h " + minutes.ToString() + "min";
		return minutes.ToString() + "min";
	}
};

class ZBerryBanEntry {
	string Id; //Steam64 id, empty if only the GUID is known
	string Guid; //BattlEye GUID, empty for offline bans by Steam64 id
	string Name; //Last known name, for display only
	string Reason;
	string BannedBy;
	int Since; //ZBerryTime seconds
	int Until; //0: permanent

	void ZBerryBanEntry(string id = "", string guid = "", string name = "", string reason = "", string bannedBy = "", int since = 0, int until = 0) {
		Id = id;
		Guid = guid;
		Name = name;
		Reason = reason;
		BannedBy = bannedBy;
		Since = since;
		Until = until;
	}

	bool IsPermanent() {
		return Until == 0;
	}

	bool IsActive(int now) {
		return IsPermanent() || Until > now;
	}

	bool Matches(string id) {
		return id != "" && (id == Id || id == Guid);
	}

	//Shown in lists and logs
	string GetDisplayName() {
		if (Name != "") return Name;
		return GetDisplayId();
	}

	string GetDisplayId() {
		if (Id != "") return Id;
		return Guid;
	}

	string DescribeUntil() {
		if (IsPermanent()) return "permanent";
		return "until " + ZBerryTime.Format(Until);
	}
};

typedef array<ref ZBerryBanEntry> ZBerryBanEntryArray;

//Root of ZomBerryBans.json
class ZBerryBansConfig {
	ref ZBerryBanEntryArray Bans;

	void ZBerryBansConfig() {
		Bans = new ZBerryBanEntryArray;
	}

	void Sanitize() {
		if (!Bans) Bans = new ZBerryBanEntryArray;

		for (int i = Bans.Count() - 1; i >= 0; --i) {
			ZBerryBanEntry ban = Bans[i];
			if (!ban || (ban.Id == "" && ban.Guid == "")) Bans.Remove(i);
		}
	}
};

//ZomBerry's own ban list (ZomBerryBans.json). Banned players are turned away while they connect (see MissionServer below),
//players online at the time of the ban are kicked
class ZBerryBans {
	static const string CONFIG_FILE = "ZomBerryBans.json";
	static const int KICK_DELAY = 2000; //ms between the chat message and the disconnect, so the reason can be read
	static const int MAX_REASON_LENGTH = 200;
	static const int MAX_DURATION = 315360000; //10 years: the seconds stay within int range for a long while

	protected ref ZBerryBansConfig m_Config = new ZBerryBansConfig;
	protected string m_Path;
	protected bool m_Writable = true; //False while the file on disk is broken: it is never overwritten

	void Load(string path) {
		m_Path = path;
		m_Writable = true;
		m_Config = new ZBerryBansConfig;
		if (!FileExist(m_Path)) return;

		ZBerryBansConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryBansConfig>.LoadFile(m_Path, loaded, loadError) || !loaded) {
			m_Writable = false;
			ZomberryBase.Log( "ZomBerryBans", "ERROR: " + CONFIG_FILE + " can't be read, nobody is banned and no bans can be added until it's fixed: " + loadError );
			return;
		}

		m_Config = loaded;
		m_Config.Sanitize();

		int pruned = Prune(ZBerryTime.Now());
		if (pruned > 0) Save();
		ZomberryBase.Log( "ZomBerryBans", "INFO: Loaded " + m_Config.Bans.Count() + " bans from " + CONFIG_FILE + ", removed " + pruned + " expired" );
	}

	bool IsWritable() {
		return m_Writable;
	}

	bool Save() {
		if (!m_Writable) return false;

		string saveError;
		if (JsonFileLoader<ZBerryBansConfig>.SaveFile(m_Path, m_Config, saveError)) return true;

		ZomberryBase.Log( "ZomBerryBans", "ERROR: Can't save " + CONFIG_FILE + ": " + saveError );
		return false;
	}

	//Removes expired bans, returns how many
	int Prune(int now) {
		int removed = 0;
		for (int i = m_Config.Bans.Count() - 1; i >= 0; --i) {
			if (m_Config.Bans[i].IsActive(now)) continue;

			m_Config.Bans.Remove(i);
			++removed;
		}
		return removed;
	}

	ZBerryBanEntry FindActive(string plainId, string guid, int now) {
		foreach (ZBerryBanEntry ban : m_Config.Bans) {
			if (!ban.IsActive(now)) continue;
			if (ban.Matches(plainId) || ban.Matches(guid)) return ban;
		}
		return NULL;
	}

	ZBerryBanEntry FindActiveFor(PlayerIdentity identity) {
		if (!identity) return NULL;

		return FindActive(identity.GetPlainId(), identity.GetId(), ZBerryTime.Now());
	}

	//Replaces earlier bans of the same player. Returns false when it couldn't be saved
	bool Add(ZBerryBanEntry entry) {
		if (!m_Writable) return false;

		RemoveMatching(entry.Id);
		RemoveMatching(entry.Guid);
		m_Config.Bans.Insert(entry);
		return Save();
	}

	//Unban by Steam64 id or GUID. Returns how many entries were removed
	int Remove(string id) {
		if (!m_Writable) return 0;

		int removed = RemoveMatching(id);
		if (removed > 0) Save();
		return removed;
	}

	protected int RemoveMatching(string id) {
		int removed = 0;
		for (int i = m_Config.Bans.Count() - 1; i >= 0; --i) {
			if (!m_Config.Bans[i].Matches(id)) continue;

			m_Config.Bans.Remove(i);
			++removed;
		}
		return removed;
	}

	ZBerryBanEntryArray GetAll() {
		return m_Config.Bans;
	}

	//"76561198000000000"
	static bool IsSteamId(string id) {
		if (id.Length() != 17) return false;
		for (int i = 0; i < id.Length(); ++i) {
			if ("0123456789".IndexOf(id.Get(i)) == -1) return false;
		}
		return true;
	}

	//BattlEye GUID as DayZ shows it (PlayerIdentity.GetId)
	static bool IsGuid(string id) {
		return id.Length() == 44 && id.IndexOf(" ") == -1;
	}

	static string CleanReason(string reason) {
		reason = reason.Trim();
		if (reason.Length() > MAX_REASON_LENGTH) reason = reason.Substring(0, MAX_REASON_LENGTH);
		if (reason == "") reason = "no reason given";
		return reason;
	}

	//Server: why they're being disconnected, for their game to show once it's back in the main menu (the engine's own
	//disconnect screen can't show a reason). until: ban end (ZBerryTime), 0 permanent; ignored for kicks
	static void SendKickNotice(PlayerIdentity identity, string reason, bool banned, int until) {
		if (!identity) return;

		int remaining = 0;
		if (banned && until > 0) remaining = Math.Max(0, until - ZBerryTime.Now());
		Param4<bool, string, int, int> notice = new Param4<bool, string, int, int>(banned, reason, until, remaining);
		GetZBerryRPC().SendRPC( ZBerryRPCOp.KickNotice, notice, true, identity );
	}

	//Chat message first, the disconnect a moment later
	void KickPlayer(PlayerBase player, string message) {
		if (!player || !ZBGetIdentity(player)) return;

		ZomberryStockFunctions.MessagePlayer(player, message);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(FinishKick, KICK_DELAY, false, player);
	}

	protected void FinishKick(PlayerBase player) {
		if (!player || !ZBGetIdentity(player)) return; //Left by themselves meanwhile

		MissionServer mission = MissionServer.Cast(GetGame().GetMission());
		if (!mission) return;

		//Vanilla logout: saves the character and removes it from the world before disconnecting
		PlayerIdentity identity = ZBGetIdentity(player);
		mission.PlayerDisconnected(player, identity, identity.GetId());
	}
};
