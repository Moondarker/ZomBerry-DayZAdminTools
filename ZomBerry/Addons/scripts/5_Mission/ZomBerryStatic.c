static ref ZomberryBase g_ZomberryBase;
static ref ZomberryCmdAPI g_ZomberryCmdAPI;
static ref ZomberryMenu g_ZomberryMenu;
static ref ZomberryLogger g_ZomberryLogger;
static ref ZomberryConfig g_ZomberryConfig;

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
		GetRPCManager().AddRPC( "ZomBerryAT", "SyncFunctions", g_ZomberryMenu, SingeplayerExecutionType.Client );
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

static ref ZomberryLogger GetZomberryLogger() {
	if ( !g_ZomberryLogger ) {
		g_ZomberryLogger = new ref ZomberryLogger;
	}

	return g_ZomberryLogger;
}

static ref ZomberryConfig GetZomberryConfig() {
	if ( !g_ZomberryConfig ) {
		g_ZomberryConfig = new ref ZomberryConfig;
	}

	return g_ZomberryConfig;
}
