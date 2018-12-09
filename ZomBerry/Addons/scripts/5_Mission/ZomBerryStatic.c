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

/*
static int CountHEXDigits(int n) {
	int ret = -1;
	int j = 0;
	while(n != j) {
		j = n;
		n = n >> 4;
		++ret;
	}
	Print ("HEX Digits: " + ret.ToString());
	return ret;
}

static string IntToHex(int n) { //ToBeDebugged, doesn't work correctly
	string hexAlphabet = "0123456789abcdef";
	array<string> outArr = new array<string>;
	string output = "";
	int len = CountHEXDigits(n);
	if (len & 1) {
		outArr.Insert("0");
	}

	for (--len; len >= 0; --len) {
		n = n >> 4;
		outArr.InsertAt(hexAlphabet[(n & 0xf)], len);
	}

	for (int idx = 0; idx < outArr.Count(); ++idx) {
		output = output + outArr[idx];
	}
	return output;
}
*/
