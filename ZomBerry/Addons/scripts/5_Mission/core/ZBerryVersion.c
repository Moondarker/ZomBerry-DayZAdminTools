//Version strings look like "0.6.4", the server may append "CFGFailed" when admins.cfg is broken
class ZBerryVersion {
	//Same major.minor ("0.6"): client and server can talk to each other
	static bool IsCompatible(string a, string b) {
		return a.Substring(0, 3) == b.Substring(0, 3);
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

	//a's minor.patch ("6.4") is higher than b's
	static bool IsNewer(string a, string b) {
		return a.Substring(2, 3).ToFloat() > b.Substring(2, 3).ToFloat();
	}
};
