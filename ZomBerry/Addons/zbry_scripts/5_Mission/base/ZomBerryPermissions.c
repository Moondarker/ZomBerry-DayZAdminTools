class ZomberryPermissions {
	protected autoptr ZBerryPermGroupMap m_permGroups;
	protected autoptr ZBerryAdminMap m_adminsList;

	void ZomberryPermissions() {
		m_permGroups = new ZBerryPermGroupMap();
		m_adminsList = new ZBerryAdminMap();
	}

	bool CheckPermission(string plyId, string permName) {
		string tmpStr = "false";

		if (GetGame().IsClient() && !GetGame().IsMultiplayer()) return true;
		if (GetCLIParam("zbryGiveAdminRightsToEveryone", tmpStr))
			if (tmpStr == "true") return true;

		ZBerryAdmin admin = m_adminsList[plyId];
		if (!admin) return false;

		ZBerryPermGroup group = m_permGroups[admin.PermGroup];
		if (!group) return false;

		return group.HavePermission(permName);
	}

	int CountAdmins() {
		return m_adminsList.Count();
	}

	void SetConfigData(ZBerryPermGroupArray pGroups, ZBerryAdminArray aList) {
		foreach(ZBerryPermGroup tempGroup: pGroups)
			m_permGroups.Insert(tempGroup.GroupName, new ZBerryPermGroup(tempGroup.GroupId, tempGroup.GroupColor, "", tempGroup.GroupPerms));

		foreach(ZBerryAdmin tempAdmin: aList)
			m_adminsList.Insert(tempAdmin.AdminId, new ZBerryAdmin(tempAdmin.AdminName, "", tempAdmin.PermGroup));

		foreach(string adminId, ZBerryAdmin admin: m_adminsList)
			if (admin)
				ZomberryBase.Log("ZomBerryPermissions", "Added admin " + admin.AdminName + " (" + adminId + ") from group " + admin.PermGroup);
			else
				ZomberryBase.Log("ZomBerryPermissions", "Added NULL admin (WTF? Pls contact Vaker)");
	}

	void SaveChanges() {
		autoptr ZBerryPermGroupArray pGroups = new ZBerryPermGroupArray;
		autoptr ZBerryAdminArray aList = new ZBerryAdminArray;

		foreach(string groupName, ZBerryPermGroup tempGroup: m_permGroups)
			pGroups.Insert(new ZBerryPermGroup(tempGroup.GroupId, tempGroup.GroupColor, groupName, tempGroup.GroupPerms));

		foreach(string adminId, ZBerryAdmin tempAdmin: m_adminsList)
			aList.Insert(new ZBerryAdmin(tempAdmin.AdminName, adminId, tempAdmin.PermGroup));

		ZomberryBase.GetConfig().UpdatePermissions(pGroups, aList);
	}
}
