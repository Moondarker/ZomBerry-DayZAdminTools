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
