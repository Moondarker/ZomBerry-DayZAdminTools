typedef array<ref ZBerryPermissionGroup> ZBerryPermissionGroupArray;

class ZBerryPermissionGroup {
	int GroupId;
	int GroupColor;
	string GroupName;
	ref TStringArray GroupPerms;

	void ZBerryPermGroup(int gId, int gColor, string gName, ref TStringArray aFuncs) {
		GroupId = gId;
		GroupColor = gColor;
		GroupName = gName;
		GroupPerms = new TStringArray;
		GroupPerms.InsertAll(aFuncs);
	}

	void AddPermission(string permName) {
		if (!HavePermission(permName)) GroupPerms.Insert(permName);
	}

	void RemovePermission(string permName) {
		foreach(int idx, string currentPerm: GroupPerms)
			if (currentPerm == permName) GroupPerms.Remove(idx);
	}

	bool HavePermission(string permName) {
		foreach(string currentPerm: GroupPerms)
			if (currentPerm == "*" || currentPerm == permName)
				return true;

		return false;
	}
}
