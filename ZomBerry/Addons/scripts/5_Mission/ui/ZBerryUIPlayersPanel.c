//Player list on the left side: selection (double click), health/blood readout and manual refresh
class ZBerryUIPlayersPanel extends ZBerryUIComponent {
	static const int COLOR_ADMIN = 0xFF47EB00;
	static const int COLOR_SELECTED = 0xFFFF751A;

	protected TextListboxWidget m_PlayersList;
	protected TextWidget m_PlyHealth, m_PlyBlood;
	protected ButtonWidget m_RefreshButton;

	protected int m_SelectedPlayerId = -1;

	void Init(Widget parent) {
		if (!LoadLayout("ZomBerry/AT/scripts/5_Mission/layouts/PlayersPanel.layout", parent)) return;

		m_PlayersList = TextListboxWidget.Cast( BindWidget("PlayerList") );
		m_RefreshButton = ButtonWidget.Cast( BindWidget("PlayersRefresh") );
		m_PlyHealth = TextWidget.Cast( m_Root.FindAnyWidget("PlayerHealth") );
		m_PlyBlood = TextWidget.Cast( m_Root.FindAnyWidget("PlayerBlood") );
	}

	static void RequestSync() {
		GetRPCManager().SendRPC( "ZomBerryAT", "SyncPlayersRequest", new Param1< int >( 0 ), true, NULL );
	}

	//Returns false if nobody is selected (or selected player has left)
	bool GetSelectedPlayerId(out int playerId) {
		if (FindPlayerRow(m_SelectedPlayerId) == -1) return false;

		playerId = m_SelectedPlayerId;
		return true;
	}

	void SetPlayers(ZBerryPlayerArray players) {
		m_PlayersList.ClearItems();

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
			RequestSync();
			return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button) {
		if (w != m_PlayersList) return false;

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
