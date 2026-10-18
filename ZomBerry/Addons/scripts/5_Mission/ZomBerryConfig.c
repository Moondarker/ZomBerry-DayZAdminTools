class ZomberryConfig {
	private string cfgPath = "$saves:";
	const private string cfgPathServer = "$profile:";
	protected string realProfilesPath = "";

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
				if (FileExist(cfgPath + "ZomBerryConfig.json")) CopyFile(cfgPath + "ZomBerryConfig.json", cfgPath + "ZomBerry\\ZomBerryConfig.json");
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

		CreateNew(cfgPath);

		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) ConfigureKeybinds();

		ConfigureMain();
	}

	private void ConfigureMain() {
		ref ZBerryJsonConfig newConfigData;

		JsonFileLoader<ZBerryJsonConfig>.JsonLoadFile(cfgPath + "ZomBerryConfig.json", newConfigData);
		if (!newConfigData) {
			ZomberryBase.Log( "ZomBerryConfig", "ERROR: Unable to find config file, will continue with default settings (debug disabled, separate log files)" );
			m_spawnMenuGroups = {
				new ZBerryJsonSpawnMenuGroup("Items","Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base"),
				new ZBerryJsonSpawnMenuGroup("Objects","Transport,House"),
				new ZBerryJsonSpawnMenuGroup("AI","DZ_LightAI"),
			};
			return;
		}

		zbryDebug = newConfigData.DebugLevel;
		defaultIOFlag = newConfigData.UseScriptLog;

		if (zbryDebug != 0) ZomberryBase.Log( "ZomBerryConfig", "INFO: Debug level " + zbryDebug );
		if (defaultIOFlag) ZomberryBase.Log( "ZomBerryConfig", "INFO: Will use script log files..." );

		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) {
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

	private string FindAdmins(bool doLogs = true) {
		string temp_path;

		if (FileExist("$profile:ZomBerry\\admins.cfg")) {
			temp_path = "$profile:ZomBerry\\";
			if (doLogs) ZomberryBase.Log( "ZomBerryConfig", "INFO: Using admins.cfg from Profile directory");
		} else if (FileExist("$CurrentDir:admins.cfg")) {
			temp_path = "$CurrentDir:";
			if (doLogs) ZomberryBase.Log( "ZomBerryConfig", "WARN: Using admins.cfg from server root directory (Better use Profile dir!)" );
		} else if (FileExist("$CurrentDir:\\ZomBerry\\Config\\admins.cfg")) {
			temp_path = "$CurrentDir:ZomBerry\\Config\\";
			if (doLogs) ZomberryBase.Log( "ZomBerryConfig", "WARN: Using admins.cfg from ZomBerry Addon directory (Better use Profile dir!)" );
		} else {
			temp_path = "";
		}
		
		return temp_path;
	}

	private void CreateNew(string dPath) {
		if (FindAdmins(false) == "") {
			FileHandle adminsFile = OpenFile(dPath + "admins.cfg", FileMode.WRITE);

			ZomberryBase.Log( "ZomBerryConfig", "INFO: Trying to create new admins.cfg file in " + dPath + "admins.cfg" );

			if (adminsFile != 0) {
				FPrintln(adminsFile, "76561198038543835");
				ZomberryBase.Log( "ZomBerryConfig", "INFO: admins.cfg created successfully." );
				CloseFile(adminsFile);
			}
		}

		if (!FileExist(dPath + "ZomBerryConfig.json")) {
			ref ZBerryJsonConfig newConfigData = new ZBerryJsonConfig();

			ZomberryBase.Log( "ZomBerryConfig", "INFO: Trying to create new config file in " + dPath + "ZomBerryConfig.json" );

			newConfigData.DebugLevel = zbryDebug;
			newConfigData.UseScriptLog = false;
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
