//Version strings look like "0.7.0": major.minor.patch, missing parts count as 0
class ZBerryVersion {
	static void Parse(string version, out int major, out int minor, out int patch) {
		TStringArray parts = new TStringArray;
		version.Split(".", parts);

		major = 0;
		minor = 0;
		patch = 0;
		if (parts.Count() > 0) major = parts[0].ToInt();
		if (parts.Count() > 1) minor = parts[1].ToInt();
		if (parts.Count() > 2) patch = parts[2].ToInt();
	}

	//Same major.minor ("0.7"): client and server can talk to each other
	static bool IsCompatible(string a, string b) {
		int aMajor, aMinor, aPatch, bMajor, bMinor, bPatch;
		Parse(a, aMajor, aMinor, aPatch);
		Parse(b, bMajor, bMinor, bPatch);

		return aMajor == bMajor && aMinor == bMinor;
	}

	//a is a later version than b
	static bool IsNewer(string a, string b) {
		int aMajor, aMinor, aPatch, bMajor, bMinor, bPatch;
		Parse(a, aMajor, aMinor, aPatch);
		Parse(b, bMajor, bMinor, bPatch);

		if (aMajor != bMajor) return aMajor > bMajor;
		if (aMinor != bMinor) return aMinor > bMinor;
		return aPatch > bPatch;
	}

	//Menu header: the commit for dev builds, "v0.7.0" otherwise (see ZBerryBuildInfo.c)
	static string GetDisplayVersion() {
		if (ZBERRY_BUILD_DEV && ZBERRY_BUILD_COMMIT != "") return ZBERRY_BUILD_COMMIT;

		return "v" + g_zbryVer;
	}

	//Logs: always the version, plus the commit when the build knows it
	static string GetFullVersion() {
		if (ZBERRY_BUILD_COMMIT == "") return "v" + g_zbryVer;

		return "v" + g_zbryVer + " (" + ZBERRY_BUILD_COMMIT + ")";
	}
};
