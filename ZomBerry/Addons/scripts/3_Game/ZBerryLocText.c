//Game names in log texts, in each reader's language. The server writes "{#STR_key|Server name}" where a name comes from
//the string table; the log file resolves it to the server's own name (after the |), an admin's client translates the
//key. Names that aren't string table keys (mods with plain text) are written as they are
class ZBerryLocText {
	protected static ref map<string, string> s_Translations = new map<string, string>; //Key -> this client's text

	//An entity's name for a log text
	static string Name(EntityAI entity) {
		if (!entity) return "";
		return Tag(FindKey(entity.GetType()), entity.GetDisplayName(), entity.GetType());
	}

	//A class's name for a log text, from whichever config has it; the class name when there's none
	static string NameOfClass(string className) {
		string serverName = "";
		TStringArray configs = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
		foreach (string configPath : configs) {
			if (GetGame().ConfigGetText(configPath + " " + className + " displayName", serverName) && serverName != "") break;
		}
		return Tag(FindKey(className), serverName, className);
	}

	//"#STR_..." when the class's displayName is exactly one string table key, "" otherwise
	protected static string FindKey(string className) {
		TStringArray configs = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
		foreach (string configPath : configs) {
			string raw = "";
			if (!GetGame().ConfigGetTextRaw(configPath + " " + className + " displayName", raw) || raw == "") continue;

			if (raw.IndexOf("$STR_") != 0 || raw.Contains(" ")) return "";
			GetGame().FormatRawConfigStringKeys(raw);
			return raw;
		}
		return "";
	}

	protected static string Tag(string key, string serverName, string className) {
		if (serverName == "") serverName = className;
		if (key == "") return serverName;

		serverName.Replace("|", "/"); //The tag's own separators
		serverName.Replace("}", ")");
		return "{" + key + "|" + serverName + "}";
	}

	//Every well-formed tag replaced: translated into this game's language, or (translate false, or no translation) the
	//server's name. Anything else, a "{" without a "}" included, stays as it is
	static string Resolve(string text, bool translate) {
		int start = text.IndexOf("{#STR_");
		if (start == -1) return text;

		string result = "";
		while (start != -1) {
			int separator = text.IndexOfFrom(start, "|");
			int end = text.IndexOfFrom(start, "}");
			if (separator == -1 || end == -1 || separator > end) break;

			//Each piece in its own local: a Substring inside Translate overwrote a pending Substring of this expression
			string before = text.Substring(0, start);
			string key = text.Substring(start + 1, separator - start - 1);
			string serverName = text.Substring(separator + 1, end - separator - 1);
			string name = Translate(key, serverName, translate);
			result += before;
			result += name;
			text = text.Substring(end + 1, text.Length() - end - 1);
			start = text.IndexOf("{#STR_");
		}
		return result + text;
	}

	protected static string Translate(string key, string serverName, bool translate) {
		if (!translate) return serverName;

		string translated;
		if (s_Translations.Find(key, translated)) return translated;

		//A key this game doesn't know comes back as itself, without the #
		string bareKey = key.Substring(1, key.Length() - 1);
		translated = Widget.TranslateString(key);
		if (translated == "" || translated == key || translated == bareKey) translated = serverName;
		s_Translations.Set(key, translated);
		return translated;
	}
};
