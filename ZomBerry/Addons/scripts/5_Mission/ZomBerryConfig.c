class ZomberryConfig {
	private string cfgPath = "$saves:";
	const private string cfgPathServer = "$profile:";
	protected string realProfilesPath = "";

	protected int menuKey = KeyCode.KC_M;
	protected int zbryDebug = 0;
	protected bool defaultIOFlag = false;
	protected autoptr TStringIntMap m_keyBindList = new TStringIntMap;
	protected autoptr ZBerryJsonSpawnMenuGroupArray m_spawnMenuGroups;

	void ZomberryConfig() {
		if (GetGame().IsMultiplayer() && GetGame().IsServer()) {
			if (!GetCLIParam("profiles", realProfilesPath)) ZomberryBase.Log( "ZomBerryConfig", "WARN: \"-profiles=<FOLDERNAME>\" launch parameter is NOT set!!!" );
				else ZomberryBase.Log( "ZomBerryConfig", "INFO: Profile dir is: " + realProfilesPath);

			cfgPath = cfgPathServer;

			if (!FileExist(cfgPath + "ZomBerry\\")) MakeDirectory(cfgPath + "\\ZomBerry\\");
			if (FileExist(cfgPath + "ZomBerry\\")) {
				if (FileExist(cfgPath + "ZomBerry.cfg")) CopyFile(cfgPath + "ZomBerry.cfg", cfgPath + "ZomBerry\\ZomBerry.cfg");
				if (FileExist(cfgPath + "ZomBerryConfig.json")) CopyFile(cfgPath + "ZomBerryConfig.json", cfgPath + "ZomBerry\\ZomBerryConfig.json");
				DeleteFile(cfgPath + "ZomBerry.cfg");
				DeleteFile(cfgPath + "ZomBerryConfig.json");

				cfgPath += "ZomBerry\\";
			} else {
				if (realProfilesPath != "") ZomberryBase.Log( "ZomBerryConfig", "WARN: Can't create ZomBerry subfolder (" + realProfilesPath + "\\ZomBerry\\), please create one manually!" );
					else ZomberryBase.Log( "ZomBerryConfig", "WARN: Can't create ZomBerry subfolder (" + cfgPath + "ZomBerry\\), please create one manually!" );
			}
		}

		if (realProfilesPath != "") ZomberryBase.Log( "ZomBerryConfig", "INFO: Config files will be loaded from: " + realProfilesPath );
			else ZomberryBase.Log( "ZomBerryConfig", "INFO: Config files will be loaded from: " + cfgPath );
		Configure();
	}

	int GetMenuKey() {
		return menuKey;
	}

	int GetDebugLvl() {
		return zbryDebug;
	}

	string GetProfilesPath() {
		return realProfilesPath;
	}

	ref ZBerryJsonSpawnMenuGroupArray GetSpawnMenuData() {
		return m_spawnMenuGroups;
	}

	bool IsDefaultIO() {
		return defaultIOFlag;
	}

	int GetKeyByFunc(string fName) {
		if (m_keyBindList.Contains(fName)) return m_keyBindList.Get(fName);

		return -1;
	}

	string GetFuncByKey(int key) {
		string fName = m_keyBindList.GetKeyByValue(key);
		if (fName) return fName;

		return "";
	}

	void UpdateKeyBind(string fName, int key) {
		FileSerializer kBindsFile = new FileSerializer();

		string toBeDeleted = m_keyBindList.GetKeyByValue(key);
		if (toBeDeleted) m_keyBindList.Remove(toBeDeleted);
		if (m_keyBindList.Contains(fName)) m_keyBindList.Remove(fName);

		if (fName != "REMOVE") m_keyBindList.Insert(fName, key);
		if (fName == "ALL" && key == -2) m_keyBindList.Clear();

		if (kBindsFile.Open(cfgPath + "ZomBerryKeybinds.bin", FileMode.WRITE)) {
			kBindsFile.Write(m_keyBindList);
			kBindsFile.Close();

			if (fName != "REMOVE" && fName != "ALL") {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: Updated KeyBinds file: " + fName + " bind to " + key );
			} else {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: Updated KeyBinds file: unbind functions on " + key + " key(s) " );
			}
		}
	}

	private void Configure() { //TODO: JSONize this

		if (FileExist(cfgPath + "ZomBerry.cfg") && !FileExist(cfgPath + "ZomBerryConfig.json")) ReadOldConfig(); //Read an old config, new one will be created with old params

		if (!FileExist(cfgPath + "ZomBerryConfig.json") || !FileExist(cfgPath + "ZomBerryKeybinds.bin")) CreateNew(cfgPath);

		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) ConfigureKeybinds();

		ConfigureMain();

		/*UAInput zbryMenu = GetUApi().GetInputByName( "UAZomberryOpenMenu" );  //UApi is not ready yet
		if (zbryMenu.BindingCount() == 0) {
		GetUApi().DeRegisterInput( "UAZomberryOpenMenu" );
		zbryMenu = GetUApi().RegisterInput( "UAZomberryOpenMenu", "STR_USRACT_OPEN_ZBRY_MENU", "infantry" );
		zbryMenu.BindCombo( "kK" );
		GetUApi().Export();
		Print ("[ZomBerryDbg] KeyBind not found, created new one.");*/
	}

	private void ConfigureMain() {
		ref ZBerryJsonConfig newConfigData;

		JsonFileLoader<ZBerryJsonConfig>.JsonLoadFile(cfgPath + "ZomBerryConfig.json", newConfigData);
		if (!newConfigData) {
			ZomberryBase.Log( "ZomBerryConfig", "ERROR: Unable to find config file, will continue with default settings (MenuKey is M, debug disabled, separate config files)" );
			return;
		}

		zbryDebug = newConfigData.DebugLevel;
		defaultIOFlag = newConfigData.UseScriptLog;

		if (zbryDebug != 0) ZomberryBase.Log( "ZomBerryConfig", "INFO: Debug level " + zbryDebug );
		if (defaultIOFlag) ZomberryBase.Log( "ZomBerryConfig", "INFO: Will use script log files..." );

		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) { //TO BE CHANGED v0.6 - GUI CFG
			string tempKeyCode;

			for (int i = 0; i < 126; ++i) {
				tempKeyCode = typename.EnumToString(KeyCode, i);
				if (newConfigData.MenuKey == tempKeyCode) {
					menuKey = i;
					ZomberryBase.Log( "ZomBerryConfig", "INFO: Menu key set to " + tempKeyCode );
					break;
				}
			}

			if ((menuKey == KeyCode.KC_M) && (newConfigData.MenuKey != "KC_M"))
				ZomberryBase.Log( "ZomBerryConfig", "WARN: Cannot set menu key to " + newConfigData.MenuKey + " - unknown keycode" );

			m_spawnMenuGroups = newConfigData.SpawnMenuSorting;
		}
	}

	private void ConfigureKeybinds() {
		FileSerializer kBindsFile = new FileSerializer();
		int idx = 0;

		if (kBindsFile.Open(cfgPath + "ZomBerryKeybinds.bin", FileMode.READ)) {
			kBindsFile.Read(m_keyBindList);
			kBindsFile.Close();

			ZomberryBase.Log( "ZomBerryConfig", "INFO: Loaded KeyBinds file with " + m_keyBindList.Count().ToString() + " entries" );

			for (idx = 0; idx < m_keyBindList.Count(); idx++) {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: Key " + typename.EnumToString(KeyCode, m_keyBindList.GetElement(idx)) + " - " + m_keyBindList.GetKey(idx));
			}
		}
	}

	private void ReadOldConfig() {
		FileHandle configFile = OpenFile(cfgPath + "ZomBerry.cfg", FileMode.READ);
		int idx = 0;

		ZomberryBase.Log( "ZomBerryConfig", "INFO: Seems like we found an old config file, will try to convert into JSON format (use ZomBerryConfig.json from now on!)" );

		if (configFile != 0) {
			string sLine = "";
			TStringArray sParams = new TStringArray;

			while ( FGets(configFile,sLine) > 0 ) {
				++idx;
				sLine.Replace(" ", ""); sLine.Replace(";", "");
				sLine.Split("=", sParams);
				if (sParams.Count() != 2) {
					ZomberryBase.Log( "ZomBerryConfig", "WARN: config line " + idx.ToString() + " (param " + sParams[0] + ") - parse error (not key = value), " + sParams.Count() + " tokens" );
					sParams = {};
					continue;
				}

				switch (sParams[0]) {
					case "debug":
					if (sParams[1].ToInt() > 0 && sParams[1].ToInt() < 3) {
						zbryDebug = sParams[1].ToInt();
						ZomberryBase.Log( "ZomBerryConfig", "INFO: Debug level: " + sParams[1] );
					}
					break;

					case "menuKey":
					string tempKeyCode;
					if (GetGame().IsMultiplayer() && GetGame().IsServer()) continue;
					for (int i = 0; i < 126; ++i) {
						tempKeyCode = typename.EnumToString(KeyCode, i);
						if (sParams[1] == tempKeyCode) {
							menuKey = i;
							ZomberryBase.Log( "ZomBerryConfig", "INFO: Menu key set to " + tempKeyCode );
							break;
						}
					}
					if (sParams[1] != tempKeyCode)
						ZomberryBase.Log( "ZomBerryConfig", "WARN: Cannot set menu key to " + sParams[1] + " - unknown keycode" );
					break;
				}
				sParams = {};
			}
			CloseFile(configFile);
			DeleteFile(cfgPath + "ZomBerry.cfg");
		}
	}

	ref TStringArray ConfigureAdmins() { //TODO: JSONize this
		ref TStringArray adminList = new TStringArray;
		string temp_path;

		if (GetCLIParam("zbryGiveAdminRightsToEveryone", temp_path)) {
			if (temp_path == "true") {
				ZomberryBase.Log( "ZomBerryConfig", "WARN: WARNING! EVERY PLAYER JOINED TO THIS SERVER WILL HAVE FULL ADMIN RIGHTS DUE TO '-zbryGiveAdminRightsToEveryone=true' LAUNCH PARAMETER!" );
				return {};
			}
		}

		if (!GetCLIParam("zbryInstallMode", temp_path)) adminList.Insert("YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54=");
		temp_path = FindAdmins();
		ZomberryBase.Log( "ZomBerryDbg", "Server ready, loading admin list from: " + temp_path + "admins.cfg");

		FileHandle adminFile = OpenFile(temp_path + "admins.cfg", FileMode.READ);
		if (adminFile != 0) {
			string sLine = "";
			ZomberryBase.Log( "ZomBerryConfig", "admins.cfg loaded");
			while ( FGets(adminFile,sLine) > 0 ) {
				adminList.Insert(sLine);
				ZomberryBase.Log( "ZomBerryConfig", "Added admin: " + sLine);
			}
			CloseFile(adminFile);
		} else {
			ZomberryBase.Log( "ZomBerryConfig", "FATAL: admins.cfg loading failed");
		}

		return adminList;
	}

	private string FindAdmins() {
		string temp_path = "$CurrentDir:\\" + g_Game.GetMissionPath();

		temp_path.Replace("mission.c", "");

		if (FileExist(temp_path + "admins.cfg")) {
			ZomberryBase.Log( "ZomBerryConfig", "WARN !!!: Using admins.cfg from MISSION directory, this directory WON'T BE USED after v0.6 release!");
		} else if (FileExist("$profile:ZomBerry\\admins.cfg")) {
			temp_path = "$profile:ZomBerry\\";
			ZomberryBase.Log( "ZomBerryConfig", "INFO: Using admins.cfg from Profile directory");
		} else if (FileExist("$CurrentDir:admins.cfg")) {
			temp_path = "$CurrentDir:";
			ZomberryBase.Log( "ZomBerryConfig", "WARN: Using admins.cfg from server root directory (Better use Profile dir!)" );
		} else if (FileExist("$CurrentDir:\\ZomBerry\\Config\\admins.cfg")) {
			temp_path = "$CurrentDir:ZomBerry\\Config\\";
			ZomberryBase.Log( "ZomBerryConfig", "WARN: Using admins.cfg from ZomBerry Addon directory (Better use Profile dir!)" );
		} else {
			temp_path = "";
			ZomberryBase.Log( "ZomBerryConfig", "FATAL: admins.cfg was NOT found, please check previous logs and read FAQ." );
		}
		if ((temp_path == "" || temp_path.Contains("$CurrentDir")) && realProfilesPath != "") ZomberryBase.Log( "ZomBerryConfig", "INFO: We highly recommend to put admins.cfg inside this folder: " + realProfilesPath + "\\ZomBerry (create it manually if it doesn't exist)");

		return temp_path;
	}

	private void CreateNew(string dPath) {
		if (!FileExist(dPath + "ZomBerryConfig.json")) {
			ref ZBerryJsonConfig newConfigData = new ZBerryJsonConfig();

			ZomberryBase.Log( "ZomBerryConfig", "INFO: Trying to create new config file in " + dPath + "ZomBerryConfig.json" );

			newConfigData.DebugLevel = zbryDebug;
			newConfigData.UseScriptLog = false;
			newConfigData.MenuKey = typename.EnumToString(KeyCode, menuKey);
			newConfigData.SpawnMenuSorting = {
												new ZBerryJsonSpawnMenuGroup("Items","Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base"),
												new ZBerryJsonSpawnMenuGroup("Objects","Transport,House"),
												new ZBerryJsonSpawnMenuGroup("AI","DZ_LightAI"),
											 };
			newConfigData.PermissionGroups = {};

			JsonFileLoader<ZBerryJsonConfig>.JsonSaveFile(cfgPath + "ZomBerryConfig.json", newConfigData);
		}

		if (!FileExist(dPath + "ZomBerryKeybinds.bin") && GetGame().IsClient()) {
			FileSerializer kBindsFile = new FileSerializer();
			autoptr TStringIntMap keyBindList = new TStringIntMap;

			ZomberryBase.Log( "ZomBerryConfig", "INFO: keybinds file not found, trying to create new one in " + dPath + "ZomBerryKeybinds.bin" );

			keyBindList.Insert("DeleteObj", KeyCode.KC_DELETE);

			if (kBindsFile.Open(dPath + "ZomBerryKeybinds.bin", FileMode.WRITE)) {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: keybinds file created successfully." );
				kBindsFile.Write(keyBindList);
				kBindsFile.Close();
			}
		}
	}

	string InstFindAdmins() {
		string tmpStr = "";

		if (GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") return FindAdmins();
		}
		return "";
	}

	bool InstInsertAdmin(string adminUID) {
		string tmpStr = "";

		if (GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") {
				FileHandle adminsFile;
				tmpStr = FindAdmins();

				adminsFile = OpenFile(tmpStr + "admins.cfg", FileMode.APPEND);
				if (adminsFile != 0) {
					FPrintln(adminsFile, adminUID);
					ZomberryBase.Log( "ZomBerryConfig", "INFO: added " + adminUID + " to admins list" );
					CloseFile(adminsFile);
					return true;
				}
			}
		}
		return false;
	}

	string InstCreateAdmins() {
		string tmpStr;

		if (GetCLIParam("zbryInstallMode", tmpStr)) {
			if (tmpStr == "true") {
				tmpStr = "$profile:\\ZomBerry\\";

				if (!FileExist(tmpStr)) MakeDirectory(tmpStr);
				if (!FileExist(tmpStr)) return "MakeDirectory failed, please create folder ZomBerry in profile dir manually";

				if (!FileExist(tmpStr + "admins.cfg")) {
					FileHandle adminsFile = OpenFile(tmpStr + "admins.cfg", FileMode.WRITE);

					if (adminsFile != 0) {
						FPrintln(adminsFile, "YWrRTYsUNXUHr2ALuJGiTQ7nvnae8XcTxe3XvJ3Ay54=");
						ZomberryBase.Log( "ZomBerryConfig", "INFO: admins file created successfully." );
						CloseFile(adminsFile);
						return "OK";
					} else return "Can't create file (not enough rights?)";
				} else return "AlreadyExists";
			}
		}
		return "Wrong mode";
	}
};
