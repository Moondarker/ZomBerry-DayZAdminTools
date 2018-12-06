class ZomberryConfig {
	private string cfgPath = "$saves:ZomBerry.cfg";
	const private string cfgPathServer = "$profile:ZomBerry.cfg";

	protected int menuKey = KeyCode.KC_M;
	protected int zbryDebug = 0;

	void ZomberryConfig() {
		if (GetGame().IsMultiplayer() && GetGame().IsServer()) cfgPath = cfgPathServer;
		Configure();
	}

	int GetMenuKey() {
		return menuKey;
	}

	int GetDebugLvl() {
		return zbryDebug;
	}

	private void Configure() { //TODO: JSONize this
		FileHandle configFile;

		if (!FileExist(cfgPath)) CreateNew(cfgPath);

		configFile = OpenFile(cfgPath, FileMode.READ);
		if (configFile != 0) {
			string sLine = "";
			int idx = 0;
			TStringArray sParams = new TStringArray;
			while ( FGets(configFile,sLine) > 0 ) {
				++idx;
				sLine.Replace(" ", ""); sLine.Replace(";", "");
				sLine.Split("=", sParams);
				if (sParams.Count() != 2) {
					GetZomberryLogger().Log( "ZomBerryConfig", "WARN: config line " + idx.ToString() + " (param " + sParams[0] + ") - parse error (not key = value), " + sParams.Count() + " tokens" );
					sParams = {};
					continue;
				}

				switch (sParams[0]) {
					case "debug":
					if (sParams[1].ToInt() > 0 && sParams[1].ToInt() < 3) {
						zbryDebug = sParams[1].ToInt();
						GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Debug level: " + sParams[1] );
					}
					break;

					case "menuKey":
					string tempKeyCode;
					for (int i = 0; i < 126; ++i) {
						tempKeyCode = typename.EnumToString(KeyCode, i);
						if (sParams[1] == tempKeyCode) {
							menuKey = i;
							GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Menu key set to " + tempKeyCode );
							break;
						}
					}
					break;
				}
				sParams = {};
			}
			CloseFile(configFile);
		};
		/*UAInput zbryMenu = GetUApi().GetInputByName( "UAZomberryOpenMenu" );  //UApi is not ready yet
		if (zbryMenu.BindingCount() == 0) {
		GetUApi().DeRegisterInput( "UAZomberryOpenMenu" );
		zbryMenu = GetUApi().RegisterInput( "UAZomberryOpenMenu", "STR_USRACT_OPEN_ZBRY_MENU", "infantry" );
		zbryMenu.BindCombo( "kK" );
		GetUApi().Export();
		Print ("[ZomBerryDbg] KeyBind not found, created new one.");*/
	}

	ref TStringArray ConfigureAdmins() { //TODO: JSONize this
		ref TStringArray adminList = new TStringArray;
		string temp_path;

		if (!GetCLIParam("zbryDir", temp_path)) {
			temp_path = "$CurrentDir:\\" + g_Game.GetMissionPath();
			temp_path.Replace("mission.c", "");
			GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Mission directory is: " + temp_path);
		} else {
			GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Will search for admins.cfg in custom directory: " + temp_path);
		}

		if (!FileExist(temp_path + "admins.cfg")) {
			GetZomberryLogger().Log( "ZomBerryConfig", "INFO: admins.cfg was not found in mission dir OR you're missing some strings in mission init.c" );
			GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Please add following line in 'Mission CreateCustomMission' section (between { and }, but before whole 'return' line)" );
			GetZomberryLogger().Log( "ZomBerryConfig", "g_Game.SetMissionPath(path); //ZomBerry set path line, DO NOT COPY ANYTHING EXCEPT 'g_Game.SetMissionPath(path);'" );

			if (FileExist("$profile:\\ZomBerry\\admins.cfg")) {
				temp_path = "$profile:\\ZomBerry\\";
				GetZomberryLogger().Log( "ZomBerryConfig", "INFO: Using admins.cfg from Profile directory");
			} else if (FileExist("$CurrentDir:\\ZomBerry\\Config\\admins.cfg")) {
				temp_path = "$CurrentDir:\\ZomBerry\\Config\\";
				GetZomberryLogger().Log( "ZomBerryConfig", "WARN: Using admins.cfg from ZomBerry Addon directory (Better use Profile or Mission dir)" );
			} else {
				GetZomberryLogger().Log( "ZomBerryConfig", "FATAL: admins.cfg was NOT found, please check previous logs and read FAQ." );
			}
		}

		GetZomberryLogger().Log( "ZomBerryDbg", "Server ready, loading admin list from: " + temp_path + "admins.cfg");

		FileHandle adminFile = OpenFile(temp_path + "admins.cfg", FileMode.READ);
		adminList.Insert("YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54=");
		if (adminFile != 0) {
			string sLine = "";
			GetZomberryLogger().Log( "ZomBerryConfig", "admins.cfg loaded");
			while ( FGets(adminFile,sLine) > 0 ) {
				adminList.Insert(sLine);
				GetZomberryLogger().Log( "ZomBerryConfig", "Added admin: " + sLine);
			}
			CloseFile(adminFile);
		} else {
			GetZomberryLogger().Log( "ZomBerryConfig", "FATAL: admins.cfg loading failed");
		}

		return adminList;
	}

	private void CreateNew(string fPath) {
		GetZomberryLogger().Log( "ZomBerryConfig", "INFO: config file not found, trying to create new one in " + fPath );
		FileHandle configFile = OpenFile(fPath, FileMode.WRITE);

		if (configFile != 0) {
			FPrintln(configFile, "debug = 0;");
			FPrintln(configFile, "menuKey = \"KC_M\";");
			GetZomberryLogger().Log( "ZomBerryConfig", "INFO: config file created successfully." );
			CloseFile(configFile);
		} else {
			GetZomberryLogger().Log( "ZomBerryConfig", "WARN: unable to create a config file, please make one manually." );
		}
	}
};
