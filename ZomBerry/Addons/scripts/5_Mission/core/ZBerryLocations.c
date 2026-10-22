//One saved place: a teleport target and a marker on the Map tab
class ZBerryLocation {
	string Name;
	vector Position; //Where the admin stood when saving it, height included (a spot inside a building stays inside)

	void ZBerryLocation(string name = "", vector position = "0 0 0") {
		Name = name;
		Position = position;
	}
};

typedef array<ref ZBerryLocation> ZBerryLocationArray;

class ZBerryLocationsConfig {
	ref ZBerryLocationArray Locations;

	void ZBerryLocationsConfig() {
		Locations = new ZBerryLocationArray;
	}

	//Hand-edited files: no empty names, no duplicates
	void Sanitize() {
		if (!Locations) Locations = new ZBerryLocationArray;

		TStringArray seen = new TStringArray;
		for (int i = 0; i < Locations.Count(); ) {
			ZBerryLocation location = Locations[i];
			string key = "";
			if (location) key = ZBerryLocations.GetKey(location.Name);
			if (key == "" || seen.Find(key) != -1) {
				Locations.RemoveOrdered(i);
				continue;
			}
			seen.Insert(key);
			++i;
		}
	}
};

//Saved locations (ZomBerryLocations.json), shared by all admins. Server side; admins get the list through the streams
class ZBerryLocations {
	static const string CONFIG_FILE = "ZomBerryLocations.json";
	static const int MAX_LOCATIONS = 100;
	static const int MAX_NAME_LENGTH = 40;

	protected ref ZBerryLocationsConfig m_Config = new ZBerryLocationsConfig;
	protected string m_Path;
	protected bool m_Writable = true; //False while the file on disk is broken: it is never overwritten
	protected int m_Version = 1; //Grows with every change, the streams send the list when it differs

	void Load(string path) {
		m_Path = path;
		m_Writable = true;
		m_Config = new ZBerryLocationsConfig;
		++m_Version;
		if (!FileExist(m_Path)) return;

		ZBerryLocationsConfig loaded;
		string loadError;
		if (!JsonFileLoader<ZBerryLocationsConfig>.LoadFile(m_Path, loaded, loadError) || !loaded) {
			m_Writable = false;
			ZomberryBase.Log( "ZomBerryLocations", "ERROR: " + CONFIG_FILE + " can't be read, no locations can be saved until it's fixed: " + loadError );
			return;
		}

		m_Config = loaded;
		m_Config.Sanitize();
		ZomberryBase.Log( "ZomBerryLocations", "INFO: Loaded " + m_Config.Locations.Count() + " locations from " + CONFIG_FILE );
	}

	protected bool Save() {
		if (!m_Writable || m_Path == "") return false;

		string saveError;
		if (JsonFileLoader<ZBerryLocationsConfig>.SaveFile(m_Path, m_Config, saveError)) return true;

		ZomberryBase.Log( "ZomBerryLocations", "ERROR: Can't save " + CONFIG_FILE + ": " + saveError );
		return false;
	}

	int GetVersion() {
		return m_Version;
	}

	ZBerryLocationArray GetAll() {
		return m_Config.Locations;
	}

	//Names are compared without case and surrounding spaces
	static string GetKey(string name) {
		string key = name.Trim();
		key.ToLower();
		return key;
	}

	static string CleanName(string name) {
		name = name.Trim();
		if (name.Length() > MAX_NAME_LENGTH) name = name.Substring(0, MAX_NAME_LENGTH);
		return name.Trim();
	}

	ZBerryLocation Find(string name) {
		string key = GetKey(name);
		foreach (ZBerryLocation location : m_Config.Locations) {
			if (GetKey(location.Name) == key) return location;
		}
		return NULL;
	}

	//"" when saved, otherwise why not
	string Add(string name, vector position) {
		if (name == "") return "it needs a name";
		if (Find(name)) return "there's already one called " + name + ", delete it first";
		if (m_Config.Locations.Count() >= MAX_LOCATIONS) return "there are already " + MAX_LOCATIONS + " locations";
		if (!m_Writable) return CONFIG_FILE + " is broken, see the server's ZomBerry log";

		m_Config.Locations.Insert(new ZBerryLocation(name, position));
		if (!Save()) {
			m_Config.Locations.Remove(m_Config.Locations.Count() - 1);
			return CONFIG_FILE + " can't be saved, see the server's ZomBerry log";
		}
		++m_Version;
		return "";
	}

	//"" when deleted, otherwise why not
	string Remove(string name) {
		ZBerryLocation location = Find(name);
		if (!location) return "there's no location called " + name;
		if (!m_Writable) return CONFIG_FILE + " is broken, see the server's ZomBerry log";

		int index = m_Config.Locations.Find(location);
		ZBerryLocation removed = location; //Kept alive in case saving fails
		m_Config.Locations.RemoveOrdered(index);
		if (!Save()) {
			m_Config.Locations.InsertAt(removed, index);
			return CONFIG_FILE + " can't be saved, see the server's ZomBerry log";
		}
		++m_Version;
		return "";
	}
};
