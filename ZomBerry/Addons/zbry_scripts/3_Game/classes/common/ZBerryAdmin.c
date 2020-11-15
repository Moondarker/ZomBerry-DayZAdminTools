typedef array<ref ZBerryAdmin> ZBerryAdminArray;

class ZBerryAdmin {
	string AdminName;
	string AdminId;
	string PermGroup;
	bool IsActive;

	void ZBerryAdmin(string admName, string admId, string pGroup, bool isActive) {
		AdminName = admName;
		AdminId = admId;
		PermGroup = pGroup;
		IsActive = isActive;
	}
}
