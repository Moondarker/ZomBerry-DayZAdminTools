class DayZSurvival: MissionServer
{
	/*
	* ZomBerry example part 1 start
	*
	* Please note that in customised missions this part might be placed in different file using "include" macros. Be careful!
	*/
	void DayZSurvival() {
		GetZomberryCmdAPI().AddCategory("MyCustomCategory", 0xFFFFF44F); //Category name, color #FFF44F
		GetZomberryCmdAPI().AddCommand("Bloodbath", "BloodBathFnc", this, "MyCustomCategory"); //Command name, function name, class instance, category name
	}
	
	void BloodBathFnc( string funcName, int adminId, int targetId, vector cursor ) { //Function name, admin and target session id, admin cursor pos
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		Man temp_player;

		for (int i = 0; i < players.Count(); ++i) {
			temp_player = players.Get(i);
			temp_player.SetHealth(0);
		}

		SendMsg(ZBGetPlayerById(adminId), "[ZBerryCustom] Everybody is dead.");
	}
	
	void SendMsg(PlayerBase player, string msg) {
		Param1<string> msgParam = new Param1<string>( msg );
		GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, msgParam, true, player.GetIdentity());
	}
	// ZomBerry example part 1 end
	
	void SetRandomHealth(EntityAI itemEnt)
	{
		int rndHlt = Math.RandomInt(40,100);
		itemEnt.SetHealth("","",rndHlt);
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer(identity, characterName, pos, 0, "NONE");//Creates random player
		Class.CastTo(m_player, playerEnt);
		
		GetGame().SelectPlayer(identity, m_player);
		
		return m_player;
	}
	
	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		TStringArray tops = {"Hoodie_Blue","Hoodie_Black","Hoodie_Brown","Hoodie_Green","Hoodie_Grey","Hoodie_Red"};
		TStringArray pants = {"Jeans_Black","Jeans_BlueDark","Jeans_Blue","Jeans_Brown","Jeans_Green","Jeans_Grey","CanvasPants_Beige","CanvasPants_Blue","CanvasPants_Grey"};
		TStringArray shoes = {"AthleticShoes_Black","AthleticShoes_Blue","AthleticShoes_Brown","AthleticShoes_Green","AthleticShoes_Grey"};
		
		EntityAI itemEnt;
		ItemBase itemBs;
		
		player.GetInventory().CreateInInventory(tops.GetRandomElement());
		player.GetInventory().CreateInInventory(pants.GetRandomElement());
		player.GetInventory().CreateInInventory(shoes.GetRandomElement());
		
		itemEnt = player.GetInventory().CreateInInventory("Rag");
		itemBs = ItemBase.Cast(itemEnt);
		itemBs.SetQuantity(4);
		SetRandomHealth(itemEnt);
	}
};
 
Mission CreateCustomMission(string path)
{
	//ZomBerry example part 2 start
	Print("Loaded mission: " + path);
	g_Game.SetMissionPath(path);
	//ZomBerry example part 2 end

	return new DayZSurvival();
}

void main()
{
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	Weather weather = g_Game.GetWeather();

	weather.GetOvercast().SetLimits( 0.0 , 1.0 );
	weather.GetRain().SetLimits( 0.0 , 1.0 );
	weather.GetFog().SetLimits( 0.0 , 0.25 );

	weather.GetOvercast().SetForecastChangeLimits( 0.0, 0.2 );
	weather.GetRain().SetForecastChangeLimits( 0.0, 0.1 );
	weather.GetFog().SetForecastChangeLimits( 0.15, 0.45 );

	weather.GetOvercast().SetForecastTimeLimits( 1800 , 1800 );
	weather.GetRain().SetForecastTimeLimits( 600 , 600 );
	weather.GetFog().SetForecastTimeLimits( 1800 , 1800 );

	weather.GetOvercast().Set( Math.RandomFloatInclusive(0.0, 0.3), 0, 0);
	weather.GetRain().Set( Math.RandomFloatInclusive(0.0, 0.2), 0, 0);
	weather.GetFog().Set( Math.RandomFloatInclusive(0.0, 0.1), 0, 0);
	
	weather.SetWindMaximumSpeed(30);
	weather.SetWindFunctionParams(0.1, 1.0, 50);
}