//Player list on the left side: selection (double click), health/blood readout, manual refresh, kick and ban.
//In permissions mode (Permissions tab) it lists online players plus offline known admins instead,
//in bans mode (Bans button) the ban list
class ZBerryUIPlayersPanel extends ZBerryUIComponent {
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_SELECTED = 0xFFFF751A;
	static const int COLOR_OFFLINE = 0xFF909090;

	protected TextListboxWidget m_PlayersList; //The visible variant of m_PlayersLists
	protected ref ZBerryScaledList m_PlayersLists;
	protected TextWidget m_PlyHealth, m_PlyBlood;
	protected ButtonWidget m_RefreshButton;
	protected ButtonWidget m_KickButton, m_BanButton, m_UnbanButton, m_BansButton;
	protected ZBerryUIKickBanDialog m_Dialog;

	protected int m_SelectedPlayerId = -1;
	protected ref ZBerryPlayerArray m_LastPlayers;

	protected bool m_PermsMode = false;
	protected ZBerryUIPermsTab m_PermsListener;
	protected ref ZBerryPermsSnapshot m_PermsSnapshot;
	protected ref TStringArray m_EntryIds = new TStringArray; //Permissions mode: admin id of each row
	protected ref TStringArray m_EntryNames = new TStringArray;
	protected ref TIntArray m_EntryColors = new TIntArray;
	protected string m_SelectedEntryId = "";

	protected bool m_BansMode = false;
	protected ref ZBerryBanEntryArray m_Bans;
	protected int m_BansTime; //Server time the ban list was sent at
	protected ref TStringArray m_BanIds = new TStringArray; //Bans mode: id of each row
	protected string m_SelectedBanId = "";

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/PlayersPanel.layout", parent)) return;

		m_PlayersLists = new ZBerryScaledList(m_Root, "PlayerList", this);
		m_PlayersLists.Pick();
		m_PlayersList = m_PlayersLists.GetActive();
		m_RefreshButton = ButtonWidget.Cast( BindWidget("PlayersRefresh") );
		m_KickButton = ButtonWidget.Cast( BindWidget("PlayersKick") );
		m_BanButton = ButtonWidget.Cast( BindWidget("PlayersBan") );
		m_UnbanButton = ButtonWidget.Cast( BindWidget("PlayersUnban") );
		m_BansButton = ButtonWidget.Cast( BindWidget("PlayersBans") );
		UpdateActionButtons();
		m_PlyHealth = TextWidget.Cast( m_Root.FindAnyWidget("PlayerHealth") );
		m_PlyBlood = TextWidget.Cast( m_Root.FindAnyWidget("PlayerBlood") );
	}

	//The list feeds both this panel and the map markers, so either right is enough to ask for it
	static void RequestSync() {
		if (!GetZBerryClientAuth().HasAny({ZBerryPermissions.PLAYERS_LIST, ZBerryPermissions.MAP_PLAYER_MARKERS})) return;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.SyncPlayersRequest, new Param1< int >( 0 ), true, NULL );
	}

	static void RequestBans() {
		if (!GetGame().IsMultiplayer() || !GetZBerryClientAuth().Has(ZBerryPermissions.PLAYERS_UNBAN)) return;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.BansRequest, new Param1< int >( 0 ), true, NULL );
	}

	void SetDialog(ZBerryUIKickBanDialog dialog) {
		m_Dialog = dialog;
	}

	//Menu shown: the resolution may have changed since
	void UpdateListScale() {
		if (!m_PlayersLists.Pick()) return;

		m_PlayersList = m_PlayersLists.GetActive();
		if (m_PermsMode) {
			if (m_PermsSnapshot) SetPermsEntries(m_PermsSnapshot);
		} else if (m_BansMode) {
			if (m_Bans) SetBans(m_Bans, m_BansTime);
		} else if (m_LastPlayers) {
			SetPlayers(m_LastPlayers);
		}
	}

	void OnPermissionsChanged() {
		if (m_BansMode && !GetZBerryClientAuth().Has(ZBerryPermissions.PLAYERS_UNBAN)) SetBansMode(false);
		UpdateActionButtons();
	}

	//Kick, ban and the ban list need a server and the matching right; none of them in permissions mode
	protected void UpdateActionButtons() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		bool shown = GetGame().IsMultiplayer() && !m_PermsMode;

		m_KickButton.Show(shown && !m_BansMode && auth.Has(ZBerryPermissions.PLAYERS_KICK));
		m_BanButton.Show(shown && !m_BansMode && auth.Has(ZBerryPermissions.PLAYERS_BAN));
		m_UnbanButton.Show(shown && m_BansMode);
		m_BansButton.Show(shown && auth.Has(ZBerryPermissions.PLAYERS_UNBAN));

		if (m_BansMode) {
			ZBerryUIUtils.SetButtonText(m_BansButton, "Players");
		} else {
			ZBerryUIUtils.SetButtonText(m_BansButton, "Bans");
		}

		//Without any of the buttons the list takes their row too
		bool anyShown = m_KickButton.IsVisible() || m_BanButton.IsVisible() || m_BansButton.IsVisible();
		float listHeight = 0.9;
		if (anyShown) listHeight = 0.84;

		array<TextListboxWidget> lists = m_PlayersLists.GetAll();
		foreach (TextListboxWidget list : lists) {
			float width, height;
			list.GetSize(width, height);
			if (height > 0 && height <= 1) list.SetSize(width, listHeight); //Relative height: a fraction of the panel
		}
	}

	void SetBansMode(bool bansMode) {
		if (m_BansMode == bansMode) return;
		m_BansMode = bansMode;

		m_SelectedBanId = "";
		m_PlyHealth.SetText("");
		m_PlyBlood.SetText("");
		m_PlayersList.ClearItems();
		UpdateActionButtons();

		if (m_BansMode) {
			if (m_Bans) SetBans(m_Bans, m_BansTime);
			RequestBans();
		} else if (m_LastPlayers) {
			SetPlayers(m_LastPlayers);
		}
	}

	void SetBans(ZBerryBanEntryArray bans, int serverTime) {
		m_Bans = bans;
		m_BansTime = serverTime;
		if (!m_BansMode || m_PermsMode) return;

		m_PlayersList.ClearItems();
		m_BanIds.Clear();

		foreach (ZBerryBanEntry ban : bans) {
			int row = m_PlayersList.AddItem(" " + ban.GetDisplayName() + " - " + DescribeRemaining(ban), NULL, 0);
			m_PlayersList.SetItemColor(row, 0, ZBerryUIUtils.TEXT_DEFAULT);
			m_BanIds.Insert(ban.GetDisplayId());
		}

		int selRow = m_BanIds.Find(m_SelectedBanId);
		if (selRow == -1) {
			m_SelectedBanId = "";
			m_PlyHealth.SetText("");
			m_PlyBlood.SetText("");
		} else {
			HighlightBan(selRow);
		}
	}

	protected string DescribeRemaining(ZBerryBanEntry ban) {
		if (ban.IsPermanent()) return "permanent";
		return ZBerryTime.FormatDuration(ban.Until - m_BansTime) + " left";
	}

	protected void HighlightBan(int row) {
		m_PlayersList.SetItemColor(row, 0, COLOR_SELECTED);

		ZBerryBanEntry ban = m_Bans[row];
		m_PlyHealth.SetText("  By " + ban.BannedBy + ", " + DescribeRemaining(ban));
		m_PlyBlood.SetText("  Reason: " + ban.Reason);
	}

	protected bool SelectBan() {
		int oldRow = m_BanIds.Find(m_SelectedBanId);
		if (oldRow != -1) m_PlayersList.SetItemColor(oldRow, 0, ZBerryUIUtils.TEXT_DEFAULT);

		int selRow = m_PlayersList.GetSelectedRow();
		if (selRow == -1 || selRow >= m_BanIds.Count()) return true;

		m_SelectedBanId = m_BanIds[selRow];
		HighlightBan(selRow);
		return true;
	}

	//Name shown in the list for this session id
	protected string GetPlayerName(int playerId) {
		if (!m_LastPlayers) return "";

		foreach (ZBerryPlayer ply : m_LastPlayers) {
			if (ply.m_PlayerID == playerId) return ply.m_PlayerName;
		}
		return "";
	}

	void SetPermsListener(ZBerryUIPermsTab tab) {
		m_PermsListener = tab;
	}

	void SetPermsMode(bool permsMode) {
		if (m_PermsMode == permsMode) return;
		if (permsMode) SetBansMode(false);
		m_PermsMode = permsMode;

		m_SelectedEntryId = "";
		m_PlyHealth.SetText("");
		m_PlyBlood.SetText("");
		m_PlayersList.ClearItems();
		UpdateActionButtons();

		if (!m_PermsMode && m_LastPlayers) SetPlayers(m_LastPlayers);
	}

	void SetPermsEntries(ZBerryPermsSnapshot snapshot) {
		m_PermsSnapshot = snapshot;
		if (!m_PermsMode) return;

		m_PlayersList.ClearItems();
		m_EntryIds.Clear();
		m_EntryNames.Clear();
		m_EntryColors.Clear();

		foreach (ZBerryOnlineIdentity online : snapshot.Online) {
			int color = ZBerryUIUtils.TEXT_DEFAULT;
			if (snapshot.FindAdmin(online.Id)) color = COLOR_ADMIN;
			AddEntry(online.Id, online.Name, online.Name, color);
		}

		foreach (ZBerryAdminEntry admin : snapshot.Admins) {
			if (m_EntryIds.Find(admin.Id) != -1) continue; //Online, already listed

			string name = admin.Name;
			if (name == "") name = admin.Id;
			AddEntry(admin.Id, name, name + " (offline)", COLOR_OFFLINE);
		}

		int selRow = m_EntryIds.Find(m_SelectedEntryId);
		if (selRow != -1) HighlightEntry(selRow);
	}

	protected void AddEntry(string id, string name, string text, int color) {
		int row = m_PlayersList.AddItem(" " + text, NULL, 0);
		m_PlayersList.SetItemColor(row, 0, color);
		m_EntryIds.Insert(id);
		m_EntryNames.Insert(name);
		m_EntryColors.Insert(color);
	}

	protected void HighlightEntry(int row) {
		m_PlayersList.SetItemColor(row, 0, COLOR_SELECTED);

		string group = "none";
		ZBerryAdminEntry admin;
		if (m_PermsSnapshot) admin = m_PermsSnapshot.FindAdmin(m_EntryIds[row]);
		if (admin) group = admin.Group;

		m_PlyHealth.SetText("  Group: " + group);
		m_PlyBlood.SetText("  Id: " + m_EntryIds[row]);
	}

	//Returns false if nobody is selected (or selected player has left). Kept while the ban list is shown
	bool GetSelectedPlayerId(out int playerId) {
		if (m_PermsMode || m_SelectedPlayerId < 0 || !m_LastPlayers) return false;

		foreach (ZBerryPlayer ply : m_LastPlayers) {
			if (ply.m_PlayerID != m_SelectedPlayerId) continue;

			playerId = m_SelectedPlayerId;
			return true;
		}
		return false;
	}

	void SetPlayers(ZBerryPlayerArray players) {
		m_LastPlayers = players;
		if (m_PermsMode || m_BansMode) return;

		m_PlayersList.ClearItems();
		if (!GetZBerryClientAuth().Has(ZBerryPermissions.PLAYERS_LIST)) return; //Map markers only

		for ( int i = 0; i < players.Count(); ++i ) {
			ZBerryPlayer ply = players.Get(i);
			m_PlayersList.AddItem(" " + i.ToString() + ": " + ply.m_PlayerName, new Param4<int, bool, int, int>(ply.m_PlayerID, ply.m_IsAdmin, ply.m_PlayerHealth, ply.m_PlayerBlood), 0);
			ResetRowColor(i);
		}

		int selRow = FindPlayerRow(m_SelectedPlayerId);
		if (selRow != -1) HighlightRow(selRow);
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_RefreshButton) {
			if (m_PermsMode && m_PermsListener) {
				m_PermsListener.RequestData();
			} else if (m_BansMode) {
				RequestBans();
			} else {
				RequestSync();
			}
			return true;
		}
		if (w == m_BansButton) {
			SetBansMode(!m_BansMode);
			return true;
		}
		if (w == m_KickButton || w == m_BanButton) {
			OpenDialog(w == m_BanButton);
			return true;
		}
		if (w == m_UnbanButton) {
			if (m_SelectedBanId == "") {
				ZBerryUIUtils.Message("Double-click a ban in the list first");
			} else {
				GetZBerryRPC().SendRPC( ZBerryRPCOp.Unban, new Param1<string>(m_SelectedBanId), true, NULL );
			}
			return true;
		}
		return false;
	}

	//Kick needs a selected player, ban without one asks for an id
	protected void OpenDialog(bool ban) {
		if (!m_Dialog) return;

		int playerId;
		bool selected = GetSelectedPlayerId(playerId);
		if (ban) {
			if (selected) {
				m_Dialog.OpenBan(playerId, GetPlayerName(playerId));
			} else {
				m_Dialog.OpenBan(-1, "");
			}
		} else if (selected) {
			m_Dialog.OpenKick(playerId, GetPlayerName(playerId));
		} else {
			ZBerryUIUtils.Message("Double-click a player in the list first");
		}
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w != m_PlayersList) return false;
		if (m_PermsMode) return SelectEntry();
		if (m_BansMode) return SelectBan();

		int oldRow = FindPlayerRow(m_SelectedPlayerId);
		if (oldRow != -1) ResetRowColor(oldRow);

		int selRow = m_PlayersList.GetSelectedRow();
		if (selRow == -1) return true;

		Param4<int, bool, int, int> plyData;
		m_PlayersList.GetItemData( selRow, 0, plyData );
		m_SelectedPlayerId = plyData.param1;

		HighlightRow(selRow);
		return true;
	}

	protected bool SelectEntry() {
		int oldRow = m_EntryIds.Find(m_SelectedEntryId);
		if (oldRow != -1) m_PlayersList.SetItemColor(oldRow, 0, m_EntryColors[oldRow]);

		int selRow = m_PlayersList.GetSelectedRow();
		if (selRow == -1 || selRow >= m_EntryIds.Count()) return true;

		m_SelectedEntryId = m_EntryIds[selRow];
		HighlightEntry(selRow);
		if (m_PermsListener) m_PermsListener.OnEntrySelected(m_SelectedEntryId, m_EntryNames[selRow]);
		return true;
	}

	protected void ResetRowColor(int row) {
		Param4<int, bool, int, int> plyData;
		m_PlayersList.GetItemData( row, 0, plyData );

		if ( plyData.param2 ) {
			m_PlayersList.SetItemColor( row, 0, COLOR_ADMIN );
		} else {
			m_PlayersList.SetItemColor( row, 0, ZBerryUIUtils.TEXT_DEFAULT );
		}
	}

	protected void HighlightRow(int row) {
		Param4<int, bool, int, int> plyData;
		m_PlayersList.GetItemData( row, 0, plyData );

		m_PlayersList.SetItemColor( row, 0, COLOR_SELECTED );
		m_PlyHealth.SetText("  Health: " + plyData.param3.ToString());
		m_PlyBlood.SetText("  Blood: " + plyData.param4.ToString());
	}

	protected int FindPlayerRow(int plyId) {
		Param4<int, bool, int, int> plyData;

		for (int idx = 0; idx < m_PlayersList.GetNumItems(); idx++) {
			m_PlayersList.GetItemData( idx, 0, plyData );

			if (plyData.param1 == plyId) return idx;
		}

		return -1;
	}
};
