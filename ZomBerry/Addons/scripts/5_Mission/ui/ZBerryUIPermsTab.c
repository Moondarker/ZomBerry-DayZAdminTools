//Permission editor: the players panel lists online players and known admins, this tab edits groups and
//assigns the selected entry to one. Works on a local copy of the server's data until Save (header button).
//Groups ranked at or above the editor's own level are read-only (the server enforces the same rule)
class ZBerryUIPermsTab extends ZBerryUITab {
	static const int COLOR_ALLOWED = 0xFF47EB00;
	static const int COLOR_DENIED = 0xFFFF5A5A;
	static const int COLOR_WILDCARD = 0xFFA0D890; //Granted by a wildcard pattern
	static const int COLOR_SELECTED = 0xFFFF751A;
	static const int COLOR_LOCKED = 0xFF909090;
	static const int PREVIEW_ROWS = 500;
	static const string PERMS_TITLE = "Permissions (double-click: allow / deny / unset)";

	protected TextWidget m_SelectedText, m_PermsTitle;
	protected ButtonWidget m_GroupButton, m_AddGroupButton, m_DeleteGroupButton, m_AddPermButton, m_SpawnPreviewButton;
	protected TextListboxWidget m_GroupsList, m_PermsList; //Visible variants of m_GroupsLists, m_PermsLists
	protected ref ZBerryScaledList m_GroupsLists, m_PermsLists;
	protected EditBoxWidget m_GroupNameInput, m_PermInput, m_SpawnRulesInput, m_LevelInput;

	protected ZBerryUIPlayersPanel m_PlayersPanel;
	protected ref ZBerryPermsSnapshot m_Snapshot;
	protected string m_EditGroup = "";
	protected string m_SelectedId = "";
	protected string m_SelectedName = "";
	protected bool m_Dirty = false;
	protected bool m_ConfirmDelete = false;
	protected bool m_PreviewMode = false; //Right list shows what the spawn rules allow instead of permissions
	protected string m_PermsListGroup = ""; //Whose permissions the list shows: same group and rows are updated in place, which keeps the scroll
	protected string m_ToggledNode = ""; //Kept in view when toggling it rebuilt the list
	protected float m_LaidOutHeight = -1;
	protected int m_MouseOnInputs = 0; //Typing must not close the menu

	protected override void OnInit() {
		m_SelectedText = TextWidget.Cast( m_Root.FindAnyWidget("SelectedText") );
		m_PermsTitle = TextWidget.Cast( m_Root.FindAnyWidget("PermsTitle") );
		m_SpawnPreviewButton = ButtonWidget.Cast( BindWidget("SpawnPreviewButton") );
		m_GroupButton = ButtonWidget.Cast( BindWidget("GroupButton") );
		m_AddGroupButton = ButtonWidget.Cast( BindWidget("AddGroupButton") );
		m_DeleteGroupButton = ButtonWidget.Cast( BindWidget("DeleteGroupButton") );
		m_AddPermButton = ButtonWidget.Cast( BindWidget("AddPermButton") );
		m_GroupsLists = new ZBerryScaledList(m_Root, "GroupsList", this);
		m_PermsLists = new ZBerryScaledList(m_Root, "PermsList", this);
		UpdateListScale();
		m_GroupNameInput = EditBoxWidget.Cast( BindWidget("GroupNameInput") );
		m_PermInput = EditBoxWidget.Cast( BindWidget("PermInput") );
		m_SpawnRulesInput = EditBoxWidget.Cast( BindWidget("SpawnRulesInput") );
		m_LevelInput = EditBoxWidget.Cast( BindWidget("LevelInput") );
	}

	protected bool CanEdit(ZBerryPermGroup group) {
		return group && m_Snapshot && group.IsEditableBy(m_Snapshot.EditorLevel);
	}

	//Tells why when it can't
	protected ZBerryPermGroup GetEditableGroup() {
		ZBerryPermGroup group;
		if (m_Snapshot) group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) return NULL;

		if (!CanEdit(group)) {
			ZBerryUIUtils.Message("Your group can't change group " + group.Name + " (level " + group.Level + ", yours is " + m_Snapshot.EditorLevel + ")");
			return NULL;
		}
		return group;
	}

	//Admins whose group is out of reach can't be moved
	protected bool CanEditAdmin(ZBerryAdminEntry admin) {
		if (!admin) return true;
		return CanEdit(m_Snapshot.FindGroup(admin.Group));
	}

	//Layouts can't pin a widget to both the top and the bottom of its parent, so the vertical layout is set here:
	//title row and lists at fixed offsets from the top, input rows at fixed offsets from the bottom, lists in between
	protected void LayoutRows() {
		float screenWidth, screenHeight;
		m_Root.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth <= 0 || screenHeight <= 0) return;
		if (Math.AbsFloat(screenHeight - m_LaidOutHeight) < 0.5) return;
		m_LaidOutHeight = screenHeight;

		//SetPos/SetSize take physical pixels, the offsets below are layout units: the wrapper is exactly 600 units wide
		float u = screenWidth / 600;
		float listTop = 70 * u;
		float spawnInputY = screenHeight - (6 + 24) * u;
		float spawnTitleY = spawnInputY - (4 + 22) * u;
		float inputRowY = spawnTitleY - (10 + 24) * u;
		float listHeight = Math.Max(60 * u, inputRowY - 4 * u - listTop);

		array<TextListboxWidget> lists = {};
		lists.InsertAll(m_GroupsLists.GetAll());
		lists.InsertAll(m_PermsLists.GetAll());
		foreach (TextListboxWidget list : lists) {
			SetWidgetY(list, listTop);
			SetWidgetHeight(list, listHeight);
		}

		TStringArray inputRow = {"GroupNameInput", "AddGroupButton", "DeleteGroupButton", "PermInput", "AddPermButton"};
		foreach (string inputName : inputRow) SetRowY(inputName, inputRowY);

		SetRowY("SpawnRulesTitle", spawnTitleY);
		SetRowY("SpawnPreviewButton", spawnTitleY);
		SetRowY("SpawnRulesInput", spawnInputY);
	}

	protected void SetRowY(string widgetName, float y) {
		SetWidgetY(m_Root.FindAnyWidget(widgetName), y);
	}

	protected void SetWidgetY(Widget w, float y) {
		if (!w) return;

		float x, oldY;
		w.GetPos(x, oldY);
		w.SetPos(x, y);
	}

	protected void SetWidgetHeight(Widget w, float height) {
		if (!w) return;

		float width, oldHeight;
		w.GetSize(width, oldHeight);
		w.SetSize(width, height);
	}

	//The tab's height is only known once it is shown, and changes with the resolution
	override void Update(float tDelta) {
		if (IsVisible()) LayoutRows();
	}

	void SetPlayersPanel(ZBerryUIPlayersPanel panel) {
		m_PlayersPanel = panel;
		m_PlayersPanel.SetPermsListener(this);
	}

	protected override void OnActivate() {
		LayoutRows();
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
		UpdateListScale();
		if (IsVisible()) RequestData();
	}

	protected void UpdateListScale() {
		bool groupsChanged = m_GroupsLists.Pick();
		bool permsChanged = m_PermsLists.Pick();
		m_GroupsList = m_GroupsLists.GetActive();
		m_PermsList = m_PermsLists.GetActive();

		if (groupsChanged) RefreshGroups();
		if (permsChanged) {
			m_PermsListGroup = ""; //The new variant is empty
			RefreshPerms();
		}
		m_LaidOutHeight = -1; //New variants need their position too
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
		RefreshLevel();
		UpdateActionButton();
	}

	protected void RefreshLevel() {
		ZBerryPermGroup group;
		if (m_Snapshot) group = m_Snapshot.FindGroup(m_EditGroup);

		if (!group) {
			m_LevelInput.SetText("");
			return;
		}
		m_LevelInput.SetText(group.Level.ToString());
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
			ZBerryUIUtils.SetButtonText(m_GroupButton, "Group: " + admin.Group);
		} else {
			ZBerryUIUtils.SetButtonText(m_GroupButton, "Group: none");
		}
	}

	protected void RefreshGroups() {
		m_GroupsList.ClearItems();
		if (!m_Snapshot) return;

		for (int i = 0; i < m_Snapshot.Groups.Count(); ++i) {
			ZBerryPermGroup group = m_Snapshot.Groups[i];
			m_GroupsList.AddItem(" " + group.Name + " [" + group.Level + "] (" + CountAdmins(group.Name) + ")", new Param1<string>(group.Name), 0);
			if (group.Name == m_EditGroup) {
				m_GroupsList.SetItemColor(i, 0, COLOR_SELECTED);
			} else if (!CanEdit(group)) {
				m_GroupsList.SetItemColor(i, 0, COLOR_LOCKED);
			}
		}
	}

	protected int CountAdmins(string groupName) {
		int count = 0;
		foreach (ZBerryAdminEntry admin : m_Snapshot.Admins) {
			if (admin.Group == groupName) ++count;
		}
		return count;
	}

	protected void SetPreviewMode(bool previewMode) {
		m_PreviewMode = previewMode;
		if (m_PreviewMode) {
			ZBerryUIUtils.SetButtonText(m_SpawnPreviewButton, "Permissions");
		} else {
			ZBerryUIUtils.SetButtonText(m_SpawnPreviewButton, "Preview");
		}
		RefreshPerms();
	}

	//The classes the edited group's spawn rules allow, from the same configs the Spawn tab lists
	protected void RefreshPreview() {
		m_PermsList.ClearItems();
		m_PermsListGroup = "";

		ZBerryPermGroup group;
		if (m_Snapshot) group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) {
			m_PermsTitle.SetText("Spawn preview: select a group");
			return;
		}

		TStringArray configs = { CFG_VEHICLESPATH, CFG_WEAPONSPATH, CFG_MAGAZINESPATH };
		TStringArray allowed = new TStringArray;
		int total = 0;

		foreach (string configPath : configs) {
			int classCount = GetGame().ConfigGetChildrenCount(configPath);
			for (int i = 0; i < classCount; ++i) {
				string className;
				GetGame().ConfigGetChildName(configPath, i, className);
				if (GetGame().ConfigGetInt(configPath + " " + className + " scope") == 0) continue;

				++total;
				if (ZBerryPermissions.IsSpawnAllowed(group.SpawnAllowed, className)) allowed.Insert(className);
			}
		}

		allowed.Sort();
		m_PermsTitle.SetText("Spawn preview of " + group.Name + ": " + allowed.Count() + " of " + total + " allowed");

		int shown = Math.Min(allowed.Count(), PREVIEW_ROWS);
		for (int row = 0; row < shown; ++row) {
			int index = m_PermsList.AddItem(" " + allowed[row], new Param1<string>(""), 0);
			m_PermsList.SetItemColor(index, 0, COLOR_ALLOWED);
		}
		if (allowed.Count() > shown) m_PermsList.AddItem(" ... and " + (allowed.Count() - shown) + " more", new Param1<string>(""), 0);
		if (allowed.Count() == 0) m_PermsList.AddItem(" Nothing: this group can't spawn anything", new Param1<string>(""), 0);
	}

	//Known nodes first, then the group's own patterns (wildcards, custom nodes)
	protected void RefreshPerms() {
		if (m_PreviewMode) {
			RefreshPreview();
			return;
		}

		m_PermsTitle.SetText(PERMS_TITLE);
		ZBerryPermGroup group;
		if (m_Snapshot) group = m_Snapshot.FindGroup(m_EditGroup);
		if (!group) {
			m_PermsList.ClearItems();
			m_PermsListGroup = "";
			return;
		}

		TStringArray rows = new TStringArray;
		rows.InsertAll(m_Snapshot.KnownNodes);
		foreach (string pattern : group.Permissions) {
			string node = pattern;
			if (node.IndexOf("-") == 0) node = node.Substring(1, node.Length() - 1);
			if (rows.Find(node) == -1) rows.Insert(node);
		}

		//Clearing the list scrolls it to the top: same group, same rows, so update them in place
		bool inPlace = m_PermsListGroup == m_EditGroup && m_PermsList.GetNumItems() == rows.Count();
		if (!inPlace) m_PermsList.ClearItems();
		m_PermsListGroup = m_EditGroup;

		for (int index = 0; index < rows.Count(); ++index) {
			string row = rows[index];
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

			if (inPlace) {
				m_PermsList.SetItem(index, " " + mark + row, new Param1<string>(row), 0);
			} else {
				m_PermsList.AddItem(" " + mark + row, new Param1<string>(row), 0);
			}
			m_PermsList.SetItemColor(index, 0, color);
		}

		//Rebuilt after a toggle added or dropped a row: back to the toggled one
		int toggledRow = rows.Find(m_ToggledNode);
		if (!inPlace && toggledRow != -1) {
			m_PermsList.SelectRow(toggledRow);
			m_PermsList.EnsureVisible(toggledRow);
		}
		m_ToggledNode = "";
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
		ZBerryPermGroup group = GetEditableGroup();
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
		ZBerryPermGroup group = GetEditableGroup();
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

		//One below the editor, so they can keep editing it
		int level = 50;
		if (m_Snapshot.EditorLevel < ZBerryPermGroup.SUPERADMIN_LEVEL) level = Math.Max(0, m_Snapshot.EditorLevel - 1);
		m_Snapshot.Groups.Insert(new ZBerryPermGroup(name, level));
		m_EditGroup = name;
		m_GroupNameInput.SetText("");
		MarkDirty();
		RefreshAll();
	}

	//Asks once, admins of the group lose their rights
	protected void DeleteGroup() {
		ZBerryPermGroup group = GetEditableGroup();
		if (!group) return;

		if (!m_ConfirmDelete) {
			m_ConfirmDelete = true;
			ZBerryUIUtils.SetButtonText(m_DeleteGroupButton, "Sure?");
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
		ZBerryUIUtils.SetButtonText(m_DeleteGroupButton, "Delete");
	}

	//none -> first group -> ... -> last group -> none, only through groups the editor may assign
	protected void CycleSelectedGroup() {
		if (m_SelectedId == "") return;

		ZBerryAdminEntry admin = m_Snapshot.FindAdmin(m_SelectedId);
		if (!CanEditAdmin(admin)) {
			ZBerryUIUtils.Message("Your group can't change admins of group " + admin.Group);
			return;
		}

		TStringArray options = {""};
		foreach (ZBerryPermGroup group : m_Snapshot.Groups) {
			if (CanEdit(group)) options.Insert(group.Name);
		}
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
			RefreshLevel();
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
			if (m_PreviewMode) SetPreviewMode(false);
			AddPattern();
			return true;
		}
		if (w == m_SpawnPreviewButton) {
			SetPreviewMode(!m_PreviewMode);
			return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (!m_Snapshot || w != m_PermsList) return false;
		if (m_PreviewMode) { //Back to the permissions
			SetPreviewMode(false);
			return true;
		}

		int row = m_PermsList.GetSelectedRow();
		if (row == -1) return true;

		Param1<string> nodeData;
		m_PermsList.GetItemData(row, 0, nodeData);
		m_ToggledNode = nodeData.param1;
		TogglePermission(nodeData.param1);
		return true;
	}

	override bool OnChange(Widget w, int x, int y, bool finished) {
		if (!m_Snapshot) return false;

		if (w == m_SpawnRulesInput) {
			ZBerryPermGroup group = GetEditableGroup();
			if (group) {
				group.SpawnAllowed = ZBerryPermissions.SplitList(m_SpawnRulesInput.GetText());
				MarkDirty();
				if (finished && m_PreviewMode) RefreshPreview(); //Rescanning the config on every key would stutter
			} else {
				RefreshSpawnRules();
			}
			return true;
		}
		if (w == m_LevelInput) {
			ZBerryPermGroup levelGroup = GetEditableGroup();
			if (!levelGroup) {
				RefreshLevel();
				return true;
			}

			int maxLevel = ZBerryPermGroup.SUPERADMIN_LEVEL;
			if (m_Snapshot.EditorLevel < ZBerryPermGroup.SUPERADMIN_LEVEL) maxLevel = m_Snapshot.EditorLevel - 1;
			levelGroup.Level = Math.Clamp(m_LevelInput.GetText().ToInt(), 0, maxLevel);
			if (finished) RefreshLevel();
			MarkDirty();
			RefreshGroups();
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
		if (w == m_SpawnRulesInput || w == m_PermInput || w == m_GroupNameInput || w == m_LevelInput) {
			++m_MouseOnInputs;
			return true;
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) {
		if (w == m_SpawnRulesInput || w == m_PermInput || w == m_GroupNameInput || w == m_LevelInput) {
			m_MouseOnInputs = Math.Max(0, m_MouseOnInputs - 1);
			if (w == m_SpawnRulesInput && m_PreviewMode) RefreshPreview();
			return true;
		}
		return false;
	}
};
