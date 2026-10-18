// Minimal offline (singleplayer) mission for testing ZomBerry without a server.
// No loot, no infected, no persistence: just a character on the map.
// Launch DayZ_x64.exe directly (not through BattlEye), e.g.:
//   DayZ_x64.exe "-mod=@CF;@ZomBerry" "-mission=.\Missions\offlineTest.chernarusplus" -window -noPause
// The folder suffix (.chernarusplus) selects the map.

Mission CreateCustomMission(string path) {
	return new MissionGameplay();
}

void main() {
	GetGame().GetWorld().SetDate(2019, 6, 1, 12, 0);

	PlayerBase player = PlayerBase.Cast( GetGame().CreatePlayer(NULL, "SurvivorM_Mirek", "12800 0 8600", 0, "NONE") );
	if (!player) {
		Print("[offlineTest] ERROR: Can't create the player");
		return;
	}

	vector pos = player.GetPosition();
	pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
	player.SetPosition(pos);

	player.GetInventory().CreateInInventory("TShirt_Blue");
	player.GetInventory().CreateInInventory("CargoPants_Blue");
	player.GetInventory().CreateInInventory("AthleticShoes_Blue");

	GetGame().SelectPlayer(NULL, player);
}
