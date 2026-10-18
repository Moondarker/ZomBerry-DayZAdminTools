//Permission editor: the players panel lists online players and known admins, this tab edits groups and
//assigns the selected entry to one. Works on a local copy of the server's data until Save (header button)
class ZBerryUIPermsTab extends ZBerryUITab {
	static const int COLOR_ALLOWED = 0xFF47EB00;
	static const int COLOR_DENIED = 0xFFFF5A5A;
	static const int COLOR_WILDCARD = 0xFFA0D890; //Granted by a wildcard pattern
	static const int COLOR_SELECTED = 0xFFFF751A;

	protected TextWidget m_SelectedText;
	protected ButtonWidget m_GroupButton, m_AddGroupButton, m_DeleteGroupButton, m_AddPermButton;
	protected TextListboxWidget m_GroupsList, m_PermsList;
	protected EditBoxWidget m_GroupNameInput, m_PermInput, m_SpawnRulesInput;

	protected ZBerryUIPlayersPanel m_PlayersPanel;
	protected ref ZBerryPermsSnapshot m_Snapshot;
	protected string m_EditGroup = "";
	protected string m_SelectedId = "";
	protected string m_SelectedName = "";
	protected bool m_Dirty = false;
	protected bool m_ConfirmDelete = false;
	protected int m_MouseOnInputs = 0; //Typing must not close the menu

	protected override void OnInit() {
		m_SelectedText = TextWidget.Cast( m_Root.FindAnyWidget("SelectedText") );
		m_GroupButton = ButtonWidget.Cast( BindWidget("GroupButton") );
		m_AddGroupButton = ButtonWidget.Cast( BindWidget("AddGroupButton") );
		m_DeleteGroupButton = ButtonWidget.Cast( BindWidget("DeleteGroupButton") );
		m_AddPermButton = ButtonWidget.Cast( BindWidget("AddPermButton") );
		m_GroupsList = TextListboxWidget.Cast( BindWidget("GroupsList") );
		m_PermsList = TextListboxWidget.Cast( BindWidget("PermsList") );
		m_GroupNameInput = EditBoxWidget.Cast( BindWidget("GroupNameInput") );
		m_PermInput = EditBoxWidget.Cast( BindWidget("PermInput") );
		m_SpawnRulesInput = EditBoxWidget.Cast( BindWidget("SpawnRulesInput") );
	}

	void SetPlayersPanel(ZBerryUIPlayersPanel panel) {
		m_PlayersPanel = panel;
		m_PlayersPanel.SetPermsListener(this);
	}

	protected override void OnActivate() {
		m_PlayersPanel.SetPermsMode(true);
		if (m_Snapshot) m_PlayersPanel.SetPermsEntries(m_Snapshot);
		UpdateActionButton();
		RequestData();
	}

	protected override void OnDeactivate() {
		if (m_PlayersPanel) m_PlayersPanel.SetPermsMode(false);
		m_MouseOnInputs = 0;
	}

	override void OnMenuShow() {
		if (IsVisible()) RequestData();
	}

	override void OnMenuHide() {
		m_MouseOnInputs = 0;
	}

	override bool BlocksClose() {
		return m_MouseOnInputs > 0;
	}

	void RequestData() {
		GetZBerryRPC().SendRPC( ZBerryRPCOp.PermsRequest, new Param1<int>(0), true, NULL );
	}

	//Fresh data from the server (also its answer to Save), local changes are dropped
	void SetSnapshot(ZBerryPermsSnapshot snapshot) {
		m_Snapshot = snapshot;
		m_Dirty = false;
		m_ConfirmDelete = false;

		if (!m_Snapshot.FindGroup(m_EditGroup)) {
			m_EditGroup = "";
			if (m_Snapshot.Groups.Count() > 0) m_EditGroup = m_Snapshot.Groups[0].Name;
		}

		if (IsVisible()) m_PlayersPanel.SetPermsEntries(m_Snapshot);
		RefreshAll();
	}

	//From the players panel
	void OnEntrySelected(string id, string name) {
		m_SelectedId = id;
		m_SelectedName = name;
		RefreshSelected();
	}

	protected void MarkDirty() {
		m_Dirty = true;
		UpdateActionButton();
	}

	protected void UpdateActionButton() {
		if (!IsVisible()) return;

		if (m_Dirty) {
			SetActionButton("Save*", COLOR_SELECTED);
		} else {
			SetActionButton("Save", ZBerryUIUtils.TEXT_DEFAULT);
		}
	}

	override void OnActionButtonClick() {
		if (!m_Snapshot) return;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.PermsUpdate, new Param1<ref ZBerryPermsSnapshot>(m_Snapshot), true, NULL );
	}

	protected void RefreshAll() {
		RefreshSelected();
		RefreshGroups();
		RefreshPerms();
		RefreshSpawnRules();
		UpdateActionButton();
	}

	protected void RefreshSelected() {
		if (m_SelectedId == "" || !m_Snapshot) {
			m_SelectedText.SetText("Double-click a player on the left");
			m_GroupButton.Show(false);
			return;
		}

		m_SelectedText.SetText("Selected: " + m_SelectedName);
		m_GroupButton.Show(true);

		ZBerryAdminEntry admin = m_Snapshot.FindAdmin(m_SelectedId);
		if (admin) {
			m_GroupButton.SetText("Group: " + admin.Group);
		} else {
			m_GroupButton.SetText("Group: none");
		}
	}

	protected void RefreshGroups() {
		m_GroupsList.ClearItems();
		if (!m_Snapshot) return;

		for (int i = 0; i < m_Snapshot.Groups.Count(); ++i) {
			ZBerryPermGroup group = m_Snapshot.Groups[i];
			m_GroupsList.AddItem(" " + group.Name + " (" + CountAdmins(group.Name) + ")", new Param1<string>(group.Name), 0);
			if (group.Name == m_EditGroup) m_GroupsList.SetItemColor(i, 0, COLOR_SELECTED);
		}
	}

	protected int CountAdmins(string groupName) {
		int count = 0;
		foreach (ZBerryAdminEntry admin : m_Snapshot.Admins) {
			if (admin.Group == groupName) ++count;
		}
		return count;
	}

	//Known nodes first, then the group's own patterns (wildcards, custom nodes)
	protected void RefreshPerms() {
		m_PermsList.ClearItems();
		if (!m_Snapshot) return;

		ZBerryPermGroup group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) return;

		TStringArray rows = new TStringArray;
		rows.InsertAll(m_Snapshot.KnownNodes);
		foreach (string pattern : group.Permissions) {
			string node = pattern;
			if (node.IndexOf("-") == 0) node = node.Substring(1, node.Length() - 1);
			if (rows.Find(node) == -1) rows.Insert(node);
		}

		foreach (string row : rows) {
			string mark = "[  ] ";
			int color = ZBerryUIUtils.TEXT_DEFAULT;

			if (group.Permissions.Find(row) != -1) {
				mark = "[x] ";
				color = COLOR_ALLOWED;
			} else if (group.Permissions.Find("-" + row) != -1) {
				mark = "[-] ";
				color = COLOR_DENIED;
			} else if (ZBerryPermissions.IsGranted(group.Permissions, row)) {
				mark = "(x) ";
				color = COLOR_WILDCARD;
			}

			int index = m_PermsList.AddItem(" " + mark + row, new Param1<string>(row), 0);
			m_PermsList.SetItemColor(index, 0, color);
		}
	}

	protected void RefreshSpawnRules() {
		ZBerryPermGroup group;
		if (m_Snapshot) group = m_Snapshot.FindGroup(m_EditGroup);

		if (!group) {
			m_SpawnRulesInput.SetText("");
			return;
		}

		m_SpawnRulesInput.SetText(ZBerryPermissions.JoinList(group.SpawnAllowed));
	}

	//unset -> allow -> deny -> unset
	protected void TogglePermission(string node) {
		ZBerryPermGroup group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) return;

		bool allowed = group.Permissions.Find(node) != -1;
		bool denied = group.Permissions.Find("-" + node) != -1;
		group.Permissions.RemoveItem(node);
		group.Permissions.RemoveItem("-" + node);

		if (!allowed && !denied) {
			group.Permissions.Insert(node);
		} else if (allowed) {
			group.Permissions.Insert("-" + node);
		}

		MarkDirty();
		RefreshPerms();
	}

	protected void AddPattern() {
		ZBerryPermGroup group = m_Snapshot.FindGroup(m_EditGroup);
		string pattern = m_PermInput.GetText().Trim();
		if (!group || pattern == "") return;

		if (group.Permissions.Find(pattern) == -1) group.Permissions.Insert(pattern);
		m_PermInput.SetText("");
		MarkDirty();
		RefreshPerms();
	}

	protected void AddGroup() {
		string name = m_GroupNameInput.GetText().Trim();
		if (name == "") {
			ZBerryUIUtils.Message("Type the new group's name first");
			return;
		}
		if (m_Snapshot.FindGroup(name)) {
			ZBerryUIUtils.Message("Group " + name + " already exists");
			return;
		}

		m_Snapshot.Groups.Insert(new ZBerryPermGroup(name));
		m_EditGroup = name;
		m_GroupNameInput.SetText("");
		MarkDirty();
		RefreshAll();
	}

	//Asks once, admins of the group lose their rights
	protected void DeleteGroup() {
		ZBerryPermGroup group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) return;

		if (!m_ConfirmDelete) {
			m_ConfirmDelete = true;
			m_DeleteGroupButton.SetText("Sure?");
			return;
		}

		for (int i = m_Snapshot.Admins.Count() - 1; i >= 0; --i) {
			if (m_Snapshot.Admins[i].Group == group.Name) m_Snapshot.Admins.Remove(i);
		}
		m_Snapshot.Groups.RemoveItem(group);

		m_EditGroup = "";
		if (m_Snapshot.Groups.Count() > 0) m_EditGroup = m_Snapshot.Groups[0].Name;
		ResetDeleteConfirm();
		MarkDirty();
		m_PlayersPanel.SetPermsEntries(m_Snapshot);
		RefreshAll();
	}

	protected void ResetDeleteConfirm() {
		m_ConfirmDelete = false;
		m_DeleteGroupButton.SetText("Delete");
	}

	//none -> first group -> ... -> last group -> none
	protected void CycleSelectedGroup() {
		if (m_SelectedId == "") return;

		TStringArray options = {""};
		foreach (ZBerryPermGroup group : m_Snapshot.Groups) options.Insert(group.Name);

		ZBerryAdminEntry admin = m_Snapshot.FindAdmin(m_SelectedId);
		string current = "";
		if (admin) current = admin.Group;

		int next = (options.Find(current) + 1) % options.Count();
		string nextGroup = options[next];

		if (nextGroup == "") {
			m_Snapshot.Admins.RemoveItem(admin);
		} else if (admin) {
			admin.Group = nextGroup;
		} else {
			m_Snapshot.Admins.Insert(new ZBerryAdminEntry(m_SelectedId, m_SelectedName, nextGroup));
		}

		MarkDirty();
		m_PlayersPanel.SetPermsEntries(m_Snapshot);
		RefreshSelected();
		RefreshGroups();
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (!m_Snapshot) return false;
		if (w != m_DeleteGroupButton && m_ConfirmDelete) ResetDeleteConfirm();

		if (w == m_GroupsList) {
			int row = m_GroupsList.GetSelectedRow();
			if (row == -1) return true;

			Param1<string> groupData;
			m_GroupsList.GetItemData(row, 0, groupData);
			m_EditGroup = groupData.param1;
			RefreshGroups();
			RefreshPerms();
			RefreshSpawnRules();
			return true;
		}
		if (w == m_GroupButton) {
			CycleSelectedGroup();
			return true;
		}
		if (w == m_AddGroupButton) {
			AddGroup();
			return true;
		}
		if (w == m_DeleteGroupButton) {
			DeleteGroup();
			return true;
		}
		if (w == m_AddPermButton) {
			AddPattern();
			return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (!m_Snapshot || w != m_PermsList) return false;

		int row = m_PermsList.GetSelectedRow();
		if (row == -1) return true;

		Param1<string> nodeData;
		m_PermsList.GetItemData(row, 0, nodeData);
		TogglePermission(nodeData.param1);
		return true;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (!m_Snapshot) return false;

		if (w == m_SpawnRulesInput) {
			ZBerryPermGroup group = m_Snapshot.FindGroup(m_EditGroup);
			if (group) {
				group.SpawnAllowed = ZBerryPermissions.SplitList(m_SpawnRulesInput.GetText());
				MarkDirty();
			}
			return true;
		}
		if (w == m_PermInput && finished) {
			AddPattern();
			return true;
		}
		if (w == m_GroupNameInput && finished) {
			AddGroup();
			return true;
		}
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y) {
		if (w == m_SpawnRulesInput || w == m_PermInput || w == m_GroupNameInput) {
			++m_MouseOnInputs;
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_SpawnRulesInput || w == m_PermInput || w == m_GroupNameInput) {
			m_MouseOnInputs = Math.Max(0, m_MouseOnInputs - 1);
			return true;
		}
		return false;
	}
};
