static ref ZomberryCmdAPI g_ZomberryCmdAPI;

/**
\brief Get shared ZomberryCmdAPI instance
	\return \p ZomberryCmdAPI class
	@code
		GetZomberryCmdAPI().AddCategory("BadThings", COLOR_RED); //COLOR_RED = 0xFFF22613
		GetZomberryCmdAPI().AddCommand("Induce vomiting", "MyModExecEject", this, "OnTarget"); //'this' is a Class reference, examples in other files
		...
		void MyModExecEject( string funcName, int adminId, int targetId, vector cursor ) {
			ZBGetPlayerById(targetId).GetStatStomachVolume().Set(2500);
		}
	@endcode
*/
static ref ZomberryCmdAPI GetZomberryCmdAPI() {
    if ( !g_ZomberryCmdAPI ) {
        g_ZomberryCmdAPI = new ZomberryCmdAPI;
    }

    return g_ZomberryCmdAPI;
}


/**
\brief Get player by session Id
	\param plyId \p int PlayerIdentity session Id
	\return \p PlayerBase player entity
	@code
		int playerId = GetGame().GetPlayer().GetIdentity().GetPlayerId();
		PlayerBase player = ZBGetPlayerById(playerId);
	@endcode
*/
static PlayerBase ZBGetPlayerById (int plyId) {
	if (!GetGame().IsMultiplayer()) return PlayerBase.Cast(GetGame().GetPlayer());

	array<Man> players = new array<Man>;
	GetGame().GetPlayers(players);

	foreach (Man player : players) {
		if (!player.GetIdentity()) continue; //Connecting/disconnecting players have no identity

		if (player.GetIdentity().GetPlayerId() == plyId) return PlayerBase.Cast(player);
	}

	return NULL;
}


/**
\brief Get a connected player's identity by session id, also while they have no character (dead, respawning)
	\param sessionId \p int PlayerIdentity session Id
	\return \p PlayerIdentity, NULL if nobody with this id is connected (and always in singleplayer)
*/
static PlayerIdentity ZBGetIdentityById(int sessionId) {
	array<PlayerIdentity> identities = new array<PlayerIdentity>;
	GetGame().GetPlayerIndentities(identities);

	foreach (PlayerIdentity identity : identities) {
		if (identity && identity.GetPlayerId() == sessionId) return identity;
	}
	return NULL;
}

/**
\brief How ZomBerry names a player in logs: "Name (Steam64 id)", so log lines can be matched by id
	\param identity \p PlayerIdentity, NULL in singleplayer
*/
static string ZBDescribePlayer(PlayerIdentity identity) {
	if (!identity) return "Singleplayer";

	return identity.GetName() + " (" + identity.GetPlainId() + ")";
}

/**
\brief How ZomBerry writes a position in logs: map coordinates in whole metres, like the map tab shows them ("X 9871 Y 2144")
	\param position \p vector world position (the height is left out)
*/
static string ZBDescribePosition(vector position) {
	int x = Math.Round(position[0]);
	int y = Math.Round(position[2]);
	return "X " + x.ToString() + " Y " + y.ToString();
}
