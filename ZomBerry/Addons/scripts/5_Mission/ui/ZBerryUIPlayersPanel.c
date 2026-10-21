//Player list on the left side: selection (double click), health/blood readout, kick and ban. Fed by the streams (GetZBerryStreams).
//In permissions mode (Permissions tab) it lists online players plus offline known admins instead,
//in bans mode (Bans button) the ban list
class ZBerryUIPlayersPanel extends ZBerryUIComponent {
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_SELECTED = 0xFFFF751A;
	static const int COLOR_OFFLINE = 0xFF909090;

	protected TextListboxWidget m_PlayersList; //The visible variant of m_PlayersLists
	protected ref ZBerryScaledList m_PlayersLists;
	static const float BANS_REFRESH_INTERVAL = 30; //Keeps "time left" current
	static const float BUTTON_WIDTH = 83; //Layout units: four buttons and three gaps fill the 350 wide panel
	static const float BUTTON_GAP = 6;

	static const int INFO_LINES = 4; //Under the player list: health, position, status, Steam64. Other modes use one
	static const float INFO_LINE_HEIGHT = 20;

	protected TextWidget m_PlyInfo; //First info line: health and blood, ban or admin details of the selected row
	protected ref array<TextWidget> m_InfoLines = new array<TextWidget>; //m_PlyInfo and the lines below it
	protected ButtonWidget m_CopyIdButton;
	protected string m_ShownSteamId = ""; //What Copy ID copies
	protected int m_LaidOutInfoLines = -1;
	protected float m_BansTimer = 0;
	protected float m_LaidOutHeight = -1;
	protected int m_LaidOutButtons = -1; //Bit per shown button of m_RowButtons
	protected ButtonWidget m_MessageButton, m_KickButton, m_BanButton, m_UnbanButton, m_BansButton;
	protected ref array<ButtonWidget> m_RowButtons = new array<ButtonWidget>; //Bottom row, left to right
	protected ZBerryUIKickBanDialog m_Dialog;
	protected ZBerryUIMessageDialog m_MessageDialog;

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
		m_MessageButton = ButtonWidget.Cast( BindWidget("PlayersMessage") );
		m_KickButton = ButtonWidget.Cast( BindWidget("PlayersKick") );
		m_BanButton = ButtonWidget.Cast( BindWidget("PlayersBan") );
		m_UnbanButton = ButtonWidget.Cast( BindWidget("PlayersUnban") );
		m_BansButton = ButtonWidget.Cast( BindWidget("PlayersBans") );
		m_RowButtons.Insert(m_MessageButton);
		m_RowButtons.Insert(m_UnbanButton);
		m_RowButtons.Insert(m_KickButton);
		m_RowButtons.Insert(m_BanButton);
		m_RowButtons.Insert(m_BansButton);
		UpdateActionButtons();
		m_PlyInfo = TextWidget.Cast( m_Root.FindAnyWidget("PlayerInfo") );
		m_InfoLines.Insert(m_PlyInfo);
		for (int line = 2; line <= INFO_LINES; ++line) m_InfoLines.Insert(TextWidget.Cast( m_Root.FindAnyWidget("PlayerInfo" + line) ));
		m_CopyIdButton = ButtonWidget.Cast( BindWidget("PlayersCopyId") );
		SetInfo("");
	}

	static void RequestBans() {
		if (!GetGame().IsMultiplayer() || !GetZBerryClientAuth().Has(ZBerryPermissions.PLAYERS_UNBAN)) return;

		GetZBerryRPC().SendRPC( ZBerryRPCOp.BansRequest, new Param1< int >( 0 ), true, NULL );
	}

	void SetDialog(ZBerryUIKickBanDialog dialog) {
		m_Dialog = dialog;
	}

	void SetMessageDialog(ZBerryUIMessageDialog dialog) {
		m_MessageDialog = dialog;
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

	//Kick, ban and the ban list need a server and the matching right; none of them in permissions mode.
	//Messages work in singleplayer too
	protected void UpdateActionButtons() {
		ZBerryClientAuth auth = GetZBerryClientAuth();
		bool shown = GetGame().IsMultiplayer() && !m_PermsMode;

		m_MessageButton.Show(!m_PermsMode && !m_BansMode && ZBerryUIMessageDialog.CanSend());
		m_KickButton.Show(shown && !m_BansMode && auth.Has(ZBerryPermissions.PLAYERS_KICK));
		m_BanButton.Show(shown && !m_BansMode && auth.Has(ZBerryPermissions.PLAYERS_BAN));
		m_UnbanButton.Show(shown && m_BansMode);
		m_BansButton.Show(shown && auth.Has(ZBerryPermissions.PLAYERS_UNBAN));

		if (m_BansMode) {
			ZBerryUIUtils.SetButtonText(m_BansButton, "Players");
		} else {
			ZBerryUIUtils.SetButtonText(m_BansButton, "Bans");
		}
	}

	//Layouts can't pin a widget to both the top and the bottom of its parent, so the bottom is laid out here, bottom up:
	//the button row (when any button is shown) 10 units above the bottom, the info line 8 above it, the list down to 8 above that.
	//The shown buttons are packed to the left, so hidden ones leave no gaps
	protected void LayoutRows() {
		float screenWidth, screenHeight;
		m_Root.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth <= 0 || screenHeight <= 0) return;

		int shownButtons = 0;
		for (int i = 0; i < m_RowButtons.Count(); ++i) {
			if (m_RowButtons[i].IsVisible()) shownButtons = shownButtons | (1 << i);
		}
		bool withButtons = shownButtons != 0;
		int infoLines = 1;
		if (!m_PermsMode && !m_BansMode) infoLines = INFO_LINES;
		if (Math.AbsFloat(screenHeight - m_LaidOutHeight) < 0.5 && shownButtons == m_LaidOutButtons && infoLines == m_LaidOutInfoLines) return;
		m_LaidOutHeight = screenHeight;
		m_LaidOutButtons = shownButtons;
		m_LaidOutInfoLines = infoLines;

		//SetPos/SetSize take physical pixels, the offsets below are layout units: the panel is exactly 350 units wide
		float u = screenWidth / 350;
		float buttonX = 0;
		foreach (ButtonWidget rowButton : m_RowButtons) {
			if (!rowButton.IsVisible()) continue;

			float oldX, buttonY;
			rowButton.GetPos(oldX, buttonY);
			rowButton.SetPos(buttonX, buttonY);
			buttonX += (BUTTON_WIDTH + BUTTON_GAP) * u;
		}

		float rowBottom = screenHeight - 10 * u;
		if (withButtons) rowBottom -= (36 + 8) * u;
		float infoY = rowBottom - infoLines * INFO_LINE_HEIGHT * u;
		float listY = 10 * u;
		float listHeight = Math.Max(60 * u, infoY - 8 * u - listY);

		for (int line = 0; line < m_InfoLines.Count(); ++line) {
			m_InfoLines[line].SetPos(4 * u, infoY + line * INFO_LINE_HEIGHT * u);
		}
		float copyWidth, copyHeight;
		m_CopyIdButton.GetSize(copyWidth, copyHeight);
		m_CopyIdButton.SetPos(screenWidth - copyWidth, infoY + (INFO_LINES - 1) * INFO_LINE_HEIGHT * u);

		array<TextListboxWidget> lists = m_PlayersLists.GetAll();
		foreach (TextListboxWidget list : lists) {
			float width, oldHeight;
			list.GetSize(width, oldHeight);
			list.SetSize(width, listHeight);
		}
	}

	void SetBansMode(bool bansMode) {
		if (m_BansMode == bansMode) return;
		m_BansMode = bansMode;

		m_SelectedBanId = "";
		SetInfo("");
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
			SetInfo("");
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
		SetInfo(ban.Reason + " (by " + ban.BannedBy + ")"); //Time left is in the row
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

	//Called by the menu every frame while it's open. Players arrive by themselves (streams), the ban list is asked for
	//every BANS_REFRESH_INTERVAL. Permissions mode has no timer: the tab reloads it on open and after saving
	void Update(float tDelta) {
		LayoutRows();
		if (!m_BansMode) return;

		m_BansTimer += tDelta;
		if (m_BansTimer >= BANS_REFRESH_INTERVAL) {
			m_BansTimer = 0;
			RequestBans();
		}
	}

	void SetPermsListener(ZBerryUIPermsTab tab) {
		m_PermsListener = tab;
	}

	void SetPermsMode(bool permsMode) {
		if (m_PermsMode == permsMode) return;
		if (permsMode) SetBansMode(false);
		m_PermsMode = permsMode;

		m_SelectedEntryId = "";
		SetInfo("");
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

		SetInfo("Group: " + group + " | Id: " + m_EntryIds[row]);
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

		if (!GetZBerryClientAuth().Has(ZBerryPermissions.PLAYERS_LIST)) { //Map markers only
			m_PlayersList.ClearItems();
			return;
		}

		bool inPlace = SameRows(players);
		if (!inPlace) m_PlayersList.ClearItems();

		for ( int i = 0; i < players.Count(); ++i ) {
			ZBerryPlayer ply = players.Get(i);
			string text = " " + i.ToString() + ": " + ply.m_PlayerName;
			Param4<int, bool, int, int> rowData = new Param4<int, bool, int, int>(ply.m_PlayerID, ply.m_IsAdmin, ply.m_PlayerHealth, ply.m_PlayerBlood);
			if (inPlace) {
				m_PlayersList.SetItem(i, text, rowData, 0);
			} else {
				m_PlayersList.AddItem(text, rowData, 0);
			}
			ResetRowColor(i);
		}

		int selRow = FindPlayerRow(m_SelectedPlayerId);
		if (selRow != -1) HighlightRow(selRow);
	}

	override bool OnClick(Widget w, int x, int y, int button) {
		if (w == m_BansButton) {
			SetBansMode(!m_BansMode);
			return true;
		}
		if (w == m_KickButton || w == m_BanButton) {
			OpenDialog(w == m_BanButton);
			return true;
		}
		if (w == m_MessageButton) {
			OpenMessageDialog();
			return true;
		}
		if (w == m_CopyIdButton) {
			if (m_ShownSteamId != "") {
				GetGame().CopyToClipboard(m_ShownSteamId);
				ZBerryUIUtils.Message("Copied " + m_ShownSteamId);
			}
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

	//To the selected player, or to everyone when nobody is selected
	protected void OpenMessageDialog() {
		if (!m_MessageDialog) return;

		int playerId;
		if (GetSelectedPlayerId(playerId)) {
			m_MessageDialog.Open(playerId, GetZBerryStreams().GetName(playerId)); //Without the item in hands
		} else {
			m_MessageDialog.Open(-1, "");
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
		GetZBerryStreams().SetSelected(m_SelectedPlayerId); //The server sends this player's health and blood

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
		ShowPlayerInfo(plyData.param1, plyData.param3, plyData.param4);
	}

	//One line of text in the first info line, the others empty
	protected void SetInfo(string text) {
		for (int i = 0; i < m_InfoLines.Count(); ++i) {
			if (i == 0) {
				m_InfoLines[i].SetText(text);
			} else {
				m_InfoLines[i].SetText("");
			}
		}
		m_ShownSteamId = "";
		m_CopyIdButton.Show(false);
	}

	//Health and blood come with the player states, the rest with the details the server sends for the selected player
	protected void ShowPlayerInfo(int playerId, int health, int blood) {
		ZBerryPlayerDetails details = GetZBerryStreams().GetDetails(playerId);
		string first = "Health: " + health.ToString() + " | Blood: " + blood.ToString();
		if (!details) {
			SetInfo(first);
			return;
		}
		if (GetGame().IsMultiplayer()) first += " | Ping: " + details.Ping.ToString() + " ms";

		string second = "";
		if (details.HasPosition) second = ZBDescribePosition(details.Position);
		if (details.OnlineSeconds >= 0) {
			if (second != "") second += " | ";
			second += "Online: " + ZBerryTime.FormatDuration(details.OnlineSeconds);
		}

		string third = ZBerryPlayerDetails.GetStatusName(details.Status);
		if (details.Vehicle != "") third += ", in " + details.Vehicle;

		string fourth = "";
		if (details.SteamId != "") fourth = "Steam64: " + details.SteamId;

		m_InfoLines[0].SetText(first);
		m_InfoLines[1].SetText(second);
		m_InfoLines[2].SetText(third);
		m_InfoLines[3].SetText(fourth);
		m_ShownSteamId = details.SteamId;
		m_CopyIdButton.Show(m_ShownSteamId != "");
	}

	protected bool SameRows(ZBerryPlayerArray players) {
		if (m_PlayersList.GetNumItems() != players.Count()) return false;

		Param4<int, bool, int, int> plyData;
		for (int row = 0; row < players.Count(); ++row) {
			m_PlayersList.GetItemData( row, 0, plyData );
			if (!plyData || plyData.param1 != players[row].m_PlayerID) return false;
		}
		return true;
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
