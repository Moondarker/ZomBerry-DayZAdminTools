//Permission nodes are dot-separated ("zomberry.map.teleport"). Patterns may use * and ?, "a.b.*" also matches "a.b",
//"*" alone grants everything, and a leading "-" denies (a deny always wins over a grant)
class ZBerryPermissions {
	static const string PLAYERS_LIST = "zomberry.players.list";
	static const string MAP_VIEW = "zomberry.map.view";
	static const string MAP_PLAYER_MARKERS = "zomberry.map.player_markers";
	static const string MAP_TELEPORT = "zomberry.map.teleport";
	static const string MAP_MARKERS_PREFIX = "zomberry.map.markers."; //+ vehicles, tents, storage, bases, bodies
	static const string SPAWN_GROUND = "zomberry.spawn.ground"; //Cursor and Ground targets
	static const string SPAWN_INVENTORY = "zomberry.spawn.inventory";
	static const string PERMISSIONS_MANAGE = "zomberry.permissions.manage";
	//Granted to a group, these shield its members from functions run by other admins (level 100 ignores them)
	static const string PROTECT_LOWER = "zomberry.protect.lower"; //Lower-ranked admins can't run any function on them
	static const string PROTECT_LOWER_HARM = "zomberry.protect.lower.harm"; //... no harmful ones
	static const string PROTECT_SAME = "zomberry.protect.same"; //Same-level admins can't run any function on them
	static const string PROTECT_SAME_HARM = "zomberry.protect.same.harm"; //... no harmful ones
	static const string PLAYERS_KICK = "zomberry.players.kick";
	static const string PLAYERS_BAN = "zomberry.players.ban";
	static const string PLAYERS_UNBAN = "zomberry.players.unban"; //Also shows the ban list
	static const string FUNCTIONS_PREFIX = "zomberry.functions.";

	static string FunctionNode(string funcName) {
		return FUNCTIONS_PREFIX + funcName;
	}

	//Map markers of one marker category (ZBerryEntityCategory below MARKER_COUNT)
	static string MapMarkerNode(int category) {
		return MAP_MARKERS_PREFIX + ZBerryEntityCategory.GetKey(category);
	}

	//Every fixed node; function nodes are added from the registered commands
	static TStringArray GetFixedNodes() {
		TStringArray nodes = {PLAYERS_LIST, PLAYERS_KICK, PLAYERS_BAN, PLAYERS_UNBAN, MAP_VIEW, MAP_PLAYER_MARKERS, MAP_TELEPORT};
		for (int category = 0; category < ZBerryEntityCategory.MARKER_COUNT; ++category) {
			nodes.Insert(MapMarkerNode(category));
		}
		nodes.InsertAll({SPAWN_GROUND, SPAWN_INVENTORY, PERMISSIONS_MANAGE, PROTECT_LOWER, PROTECT_LOWER_HARM, PROTECT_SAME, PROTECT_SAME_HARM});
		return nodes;
	}

	static bool Matches(string pattern, string node) {
		pattern.ToLower();
		node.ToLower();
		pattern = pattern.Trim();

		int len = pattern.Length();
		if (len >= 2 && pattern.Substring(len - 2, 2) == ".*" && node == pattern.Substring(0, len - 2)) return true;

		return Glob(pattern, node);
	}

	static bool IsGranted(TStringArray patterns, string node) {
		if (!patterns) return false;

		bool granted = false;
		foreach (string pattern : patterns) {
			if (pattern.IndexOf("-") == 0) {
				if (Matches(pattern.Substring(1, pattern.Length() - 1), node)) return false;
			} else if (Matches(pattern, node)) {
				granted = true;
			}
		}

		return granted;
	}

	//Rules: exact class name or base class (inheriting classes match too), * and ? wildcards on the class name, "-" denies
	static bool IsSpawnAllowed(TStringArray rules, string className) {
		if (!rules) return false;

		string classLower = className;
		classLower.ToLower();

		bool allowed = false;
		foreach (string rule : rules) {
			bool deny = rule.IndexOf("-") == 0;
			string body = rule;
			if (deny) body = rule.Substring(1, rule.Length() - 1);
			body = body.Trim();
			body.ToLower();
			if (body == "") continue;

			bool match;
			if (body.Contains("*") || body.Contains("?")) {
				match = Glob(body, classLower);
			} else {
				match = (classLower == body) || GetGame().IsKindOf(classLower, body);
			}

			if (!match) continue;
			if (deny) return false;
			allowed = true;
		}

		return allowed;
	}

	//Whole-string wildcard match: * is any run of characters, ? exactly one
	static bool Glob(string pattern, string text) {
		int patternLength = pattern.Length();
		int textLength = text.Length();
		int p = 0;
		int t = 0;
		int star = -1;
		int mark = 0;

		while (t < textLength) {
			if (p < patternLength && (pattern.Get(p) == "?" || pattern.Get(p) == text.Get(t))) {
				++p;
				++t;
			} else if (p < patternLength && pattern.Get(p) == "*") {
				star = p;
				mark = t;
				++p;
			} else if (star != -1) {
				p = star + 1;
				++mark;
				t = mark;
			} else {
				return false;
			}
		}

		while (p < patternLength && pattern.Get(p) == "*") ++p;
		return p == patternLength;
	}

	//"a, b ,c" -> {"a", "b", "c"}
	static TStringArray SplitList(string text) {
		TStringArray parts = new TStringArray;
		TStringArray result = new TStringArray;
		text.Split(",", parts);

		foreach (string part : parts) {
			part = part.Trim();
			if (part != "") result.Insert(part);
		}

		return result;
	}

	static string JoinList(TStringArray items) {
		string result = "";
		foreach (string item : items) {
			if (result != "") result += ", ";
			result += item;
		}

		return result;
	}
};

//Level orders the groups: managers may only change groups (and their admins) ranked below their own,
//except the superadmin level, which may change everything
class ZBerryPermGroup {
	static const int SUPERADMIN_LEVEL = 100;

	string Name;
	int Level;
	ref TStringArray Permissions;
	ref TStringArray SpawnAllowed;

	void ZBerryPermGroup(string name = "", int level = 0) {
		Name = name;
		Level = level;
		Permissions = new TStringArray;
		SpawnAllowed = new TStringArray;
	}

	//JSON may leave arrays out
	void Sanitize() {
		if (!Permissions) Permissions = new TStringArray;
		if (!SpawnAllowed) SpawnAllowed = new TStringArray;
		Level = Math.Clamp(Level, 0, SUPERADMIN_LEVEL);
	}

	//Can a manager of this level change it
	bool IsEditableBy(int editorLevel) {
		return editorLevel >= SUPERADMIN_LEVEL || Level < editorLevel;
	}

	bool IsSameAs(ZBerryPermGroup other) {
		return other && Name == other.Name && Level == other.Level && SameList(Permissions, other.Permissions) && SameList(SpawnAllowed, other.SpawnAllowed);
	}

	static bool SameList(TStringArray a, TStringArray b) {
		if (a.Count() != b.Count()) return false;
		foreach (string item : a) {
			if (b.Find(item) == -1) return false;
		}
		return true;
	}
};

class ZBerryAdminEntry {
	string Id; //Steam64 id (BattlEye GUID works too)
	string Name; //Last known name, for display only
	string Group;

	void ZBerryAdminEntry(string id = "", string name = "", string group = "") {
		Id = id;
		Name = name;
		Group = group;
	}
};

typedef array<ref ZBerryPermGroup> ZBerryPermGroupArray;
typedef array<ref ZBerryAdminEntry> ZBerryAdminEntryArray;

//Root of ZomBerryPermissions.json
class ZBerryPermsConfig {
	ref ZBerryPermGroupArray Groups;
	ref ZBerryAdminEntryArray Admins;

	void ZBerryPermsConfig() {
		Groups = new ZBerryPermGroupArray;
		Admins = new ZBerryAdminEntryArray;
	}

	void Sanitize() {
		if (!Groups) Groups = new ZBerryPermGroupArray;
		if (!Admins) Admins = new ZBerryAdminEntryArray;
		foreach (ZBerryPermGroup group : Groups) group.Sanitize();
	}

	ZBerryPermGroup FindGroup(string name) {
		foreach (ZBerryPermGroup group : Groups) {
			if (group.Name == name) return group;
		}
		return NULL;
	}

	ZBerryAdminEntry FindAdmin(string id) {
		foreach (ZBerryAdminEntry admin : Admins) {
			if (admin.Id == id) return admin;
		}
		return NULL;
	}
};

class ZBerryOnlineIdentity {
	int SessionId;
	string Id;
	string Name;

	void ZBerryOnlineIdentity(int sessionId = 0, string id = "", string name = "") {
		SessionId = sessionId;
		Id = id;
		Name = name;
	}
};

typedef array<ref ZBerryOnlineIdentity> ZBerryOnlineIdentityArray;

//What the Permissions tab edits: the whole config, plus the known nodes and who is online
class ZBerryPermsSnapshot {
	ref ZBerryPermGroupArray Groups;
	ref ZBerryAdminEntryArray Admins;
	ref TStringArray KnownNodes;
	ref ZBerryOnlineIdentityArray Online;
	int EditorLevel; //Level of the admin this snapshot was sent to

	void ZBerryPermsSnapshot() {
		Groups = new ZBerryPermGroupArray;
		Admins = new ZBerryAdminEntryArray;
		KnownNodes = new TStringArray;
		Online = new ZBerryOnlineIdentityArray;
	}

	ZBerryPermGroup FindGroup(string name) {
		foreach (ZBerryPermGroup group : Groups) {
			if (group.Name == name) return group;
		}
		return NULL;
	}

	ZBerryAdminEntry FindAdmin(string id) {
		foreach (ZBerryAdminEntry admin : Admins) {
			if (admin.Id == id) return admin;
		}
		return NULL;
	}
};
