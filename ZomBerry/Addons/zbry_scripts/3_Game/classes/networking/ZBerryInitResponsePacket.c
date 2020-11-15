class ZBerryInitResponsePacket {
	string localVersion = g_zbryVer;

	bool readyToWork;
	string groupName;

	ref ZBerryPermissionGroupArray permGroups;
}
