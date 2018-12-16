class DayZSurvival: MissionServer
{
	/*
	* ZomBerry example part 1 start
	*
	* Please note that in customised missions this part might be placed in different file using "include" macros. Be careful!
	*/
	void DayZSurvival() {
		GetZomberryCmdAPI().AddCategory("ServerCustomCategory", 0xFFFFF44F); //Category name, color #FFF44F
		GetZomberryCmdAPI().AddCommand("Cast bloodbath", "BloodBathFnc", this, "ServerCustomCategory", false); //Command name, function name, class instance, category name, target required
		GetZomberryCmdAPI().AddCommand("Heal (all players)", "HealAll", this, "ServerCustomCategory", false, {
			new ZBerryFuncParam("D-Size", {1, 5000, 1337,}),
		});
	}

	void BloodBathFnc( string funcName, int adminId, int targetId, vector cursor ) { //Function name, admin and target session id, admin cursor pos
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		Man temp_player;
		int rndRoll = Math.RandomInt(0, 5001);

		if (rndRoll == 1337) {
			for (int i = 0; i < players.Count(); ++i) {
				temp_player = players.Get(i);
				temp_player.SetHealth(0);
			}
			SendMsg(ZBGetPlayerById(adminId), "[ZBerryCustom] You've successfully killed everyone and sacrificed yourself.");
		} else SendMsg(ZBGetPlayerById(adminId), "[ZBerryCustom] Not enough mana!");
	}

	void HealAll( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		BleedingSourcesManagerServer BSMgr;
		PlayerBase target;

		for (int i = 0; i < players.Count(); ++i) {
			target = ZBGetPlayerById(targetId);
			BSMgr = target.GetBleedingManagerServer();

			target.SetHealth(target.GetMaxHealth());
			target.SetHealth("", "Blood", target.GetMaxHealth("", "Blood"));
			target.GetStatStamina().Set(1000);
			target.GetStatEnergy().Set(1000);
			target.GetStatWater().Set(1000);
			BSMgr.RemoveAllSources();
		}

		SendMsg(ZBGetPlayerById(adminId), "[ZBerryCustom] Healed everyone. D-Size = " + fValues[0].ToString());
	}

	void SendMsg(PlayerBase player, string msg) {
		Param1<string> msgParam = new Param1<string>( msg );
		GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, msgParam, true, player.GetIdentity());
	}
	// ZomBerry example part 1 end

	void SetRandomHealth(EntityAI itemEnt)
	{
		if ( itemEnt )
		{
			int rndHlt = Math.RandomInt(55,100);
			itemEnt.SetHealth("","",rndHlt);
		}
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
/*
		player.RemoveAllItems();

		EntityAI item = player.GetInventory().CreateInInventory(topsMissionArray.GetRandomElement());
		EntityAI item2 = player.GetInventory().CreateInInventory(pantsArray.GetRandomElement());
		EntityAI item3 = player.GetInventory().CreateInInventory(shoesArray.GetRandomElement());
*/
		EntityAI itemTop;
		EntityAI itemEnt;
		ItemBase itemBs;
		float rand;

		itemTop = player.FindAttachmentBySlotName("Body");

		if ( itemTop )
		{
			itemEnt = itemTop.GetInventory().CreateInInventory("Rag");
			if ( Class.CastTo(itemBs, itemEnt ) )
		itemBs.SetQuantity(4);

			SetRandomHealth(itemEnt);

			itemEnt = itemTop.GetInventory().CreateInInventory("RoadFlare");
			SetRandomHealth(itemEnt);

			itemEnt = itemTop.GetInventory().CreateInInventory("StoneKnife");
			SetRandomHealth(itemEnt);
		}

		rand = Math.RandomFloatInclusive(0.0, 1.0);
		if ( rand < 0.25 )
			itemEnt = player.GetInventory().CreateInInventory("SodaCan_Cola");
		else if ( rand > 0.75 )
			itemEnt = player.GetInventory().CreateInInventory("SodaCan_Spite");
		else
			itemEnt = player.GetInventory().CreateInInventory("SodaCan_Pipsi");

		SetRandomHealth(itemEnt);

		rand = Math.RandomFloatInclusive(0.0, 1.0);
		if ( rand < 0.35 )
			itemEnt = player.GetInventory().CreateInInventory("Apple");
		else if ( rand > 0.65 )
			itemEnt = player.GetInventory().CreateInInventory("Pear");
		else
			itemEnt = player.GetInventory().CreateInInventory("Plum");

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
	//INIT WEATHER BEFORE ECONOMY INIT------------------------
	Weather weather = g_Game.GetWeather();

    weather.MissionWeather(false);    // false = use weather controller from Weather.c

    weather.GetOvercast().Set( Math.RandomFloatInclusive(0.4, 0.6), 1, 0);
    weather.GetRain().Set( 0, 0, 1);
    weather.GetFog().Set( Math.RandomFloatInclusive(0.05, 0.1), 1, 0);

	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year;
	int month;
	int day;
	int hour;
	int minute;

	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

    if (((month <= 9) && (day < 20)) || ((month >= 10) && (day > 20)))
    {
        month = 9;
        day = 20;

		GetGame().GetWorld().SetDate(year, month, day, hour, minute);
	}
}
