static ref ZomberryBase g_ZomberryBase;
static ref ZomberryCmdAPI g_ZomberryCmdAPI;
static ref ZomberryMenu g_ZomberryMenu;

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
        g_ZomberryCmdAPI = new ref ZomberryCmdAPI;
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
	array<Man> players = new array<Man>;
	PlayerBase result = NULL;

	if (GetGame().IsMultiplayer()) {
		GetGame().GetPlayers(players);

		for (int i = 0; i < players.Count(); i++) {
			if (players.Get(i).GetIdentity().GetPlayerId() == plyId) {
				result = PlayerBase.Cast(players.Get(i));
			}
		}
	} else {
		result = PlayerBase.Cast(GetGame().GetPlayer());
	}

	return result;
}



//For internal purposes. Do not use unless REALLY needed!
static ref ZomberryMenu GetZomberryMenu() {
	if ( !g_ZomberryMenu ) {
		g_ZomberryMenu = new ref ZomberryMenu;
		g_ZomberryMenu.Init();
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncPlayers", g_ZomberryMenu, SingeplayerExecutionType.Client ); //TODO: Move this outta here
	}

	return g_ZomberryMenu;
}

//For internal purposes. Do not use unless REALLY needed!
static ref ZomberryBase GetZomberryBase() {
	if ( !g_ZomberryBase ) {
		g_ZomberryBase = new ref ZomberryBase;
	}

	return g_ZomberryBase;
}
