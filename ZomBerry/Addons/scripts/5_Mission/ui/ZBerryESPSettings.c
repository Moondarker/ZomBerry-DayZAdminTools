//What the ESP overlay draws, set on the ESP tab. Client-side only, saved per computer in ZomBerryESP.json next to
//ZomBerryConfig.json. A separate file on purpose: ZomBerryConfig.json is edited by hand and never written by ZomBerry
class ZBerryESPSettingsData {
	bool PlayerBox = true;
	bool PlayerSkeleton = true;
	bool PlayerName = true;
	bool PlayerDistance = true;
	bool PlayerHealth = true;
	bool FarPlayers = true; //Outside the network bubble: a label from the streamed position
	bool Vehicles = false;
	bool Infected = false;
	bool Animals = false;
	bool Loot = false; //Items lying on the ground
	int MaxDistance = 1000; //Metres, players and vehicles
	int AIDistance = 300; //Metres, infected and animals
	int LootDistance = 50; //Metres: a lot more of it is around than of anything else
};

class ZBerryESPSettings {
	static const string FILE_NAME = "ZomBerryESP.json";
	static const int MIN_DISTANCE = 50;
	static const int MAX_DISTANCE = 3000;
	static const int LOOT_MIN_DISTANCE = 10;
	static const int LOOT_MAX_DISTANCE = 300; //The overlay looks for it around the camera, which gets slow further out

	protected ref ZBerryESPSettingsData m_Data = new ZBerryESPSettingsData;
	protected bool m_LoadFailed; //An unreadable file is kept as it is: no saving over it
	protected bool m_Dirty;

	ref ScriptInvoker OnChanged = new ScriptInvoker();

	void ZBerryESPSettings() {
		Load();
	}

	ZBerryESPSettingsData Get() {
		return m_Data;
	}

	protected string GetPath() {
		return ZomberryBase.GetConfig().GetConfigDir() + FILE_NAME;
	}

	protected void Load() {
		string path = GetPath();
		if (!FileExist(path)) return; //Defaults until something is changed

		ZBerryESPSettingsData loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryESPSettingsData>.LoadFile(path, loaded, loadError) || !loaded) {
			m_LoadFailed = true;
			ZomberryBase.Log( "ZomBerryConfig", "ERROR: Can't read " + FILE_NAME + ", using default ESP settings and leaving the file as it is: " + loadError );
			return;
		}

		m_Data = loaded;
		m_Data.MaxDistance = Math.Clamp(m_Data.MaxDistance, MIN_DISTANCE, MAX_DISTANCE);
		m_Data.AIDistance = Math.Clamp(m_Data.AIDistance, MIN_DISTANCE, MAX_DISTANCE);
		m_Data.LootDistance = Math.Clamp(m_Data.LootDistance, LOOT_MIN_DISTANCE, LOOT_MAX_DISTANCE);
	}

	//After changing Get()'s fields: the overlay follows at once, the file is written by Save
	void Changed() {
		m_Dirty = true;
		OnChanged.Invoke();
	}

	void Save() {
		if (!m_Dirty) return;
		m_Dirty = false;
		if (m_LoadFailed) {
			ZomberryBase.Log( "ZomBerryConfig", "WARN: Not saving ESP settings, " + FILE_NAME + " couldn't be read (fix or delete it)" );
			return;
		}

		string saveError;
		if (!JsonFileLoader<ZBerryESPSettingsData>.SaveFile(GetPath(), m_Data, saveError)) {
			ZomberryBase.Log( "ZomBerryConfig", "ERROR: Can't save " + FILE_NAME + ": " + saveError );
		}
	}
};

static ref ZBerryESPSettings g_ZBerryESPSettings;

//Client and singleplayer
static ZBerryESPSettings GetZBerryESPSettings() {
	if (!g_ZBerryESPSettings) g_ZBerryESPSettings = new ZBerryESPSettings();

	return g_ZBerryESPSettings;
}
