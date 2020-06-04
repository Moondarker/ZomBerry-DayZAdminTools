class ZomberryConfig {
	private string cfgPath = "$saves:";
	const private string cfgPathServer = "$profile:ZomBerry\\";
	protected string realProfilesPath = "";

	protected int zbryDebug = 0;
	protected bool defaultIOFlag = false;
	protected autoptr TStringIntMap m_keyBindList = new TStringIntMap;
	protected autoptr ZBerryJsonSpawnMenuGroupArray m_spawnMenuGroups;

	void ZomberryConfig() {
		if (GetGame().IsMultiplayer() && GetGame().IsServer()) {
			cfgPath = cfgPathServer;

			if (!GetCLIParam("profiles", realProfilesPath))
				ZomberryBase.Log( "ZomBerryConfig", "WARN: \"-profiles=<FOLDERNAME>\" launch parameter is NOT set!!!" );
			else {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: Profile dir is: " + realProfilesPath);
				ZomberryBase.Log( "ZomBerryConfig", "INFO: ZomBerry will use \"ZomBerry\" subfolder of profile dir to keep config files");
			}

			/*logic*/
		}

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

		PrepareFiles();
		ConfigureClient();
		ConfigureMain();
	}

	private void ConfigureMain() {
		autoptr ZBerryJsonConfig newConfigData;

		JsonFileLoader<ZBerryJsonConfig>.JsonLoadFile(cfgPath + "ZomBerryConfig.json", newConfigData);
		if (!newConfigData) {
			ZomberryBase.Log( "ZomBerryConfig", "ERROR: Unable to find config file, will continue with default settings (MenuKey is M, debug disabled, separate log files)" );
			m_spawnMenuGroups = {
				new ZBerryJsonSpawnMenuGroup("Items","Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base"),
				new ZBerryJsonSpawnMenuGroup("Objects","Transport,House"),
				new ZBerryJsonSpawnMenuGroup("AI","DZ_LightAI"),
			};
			return;
		}

		zbryDebug = newConfigData.DebugLevel;
		defaultIOFlag = newConfigData.UseScriptLog;

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
			m_spawnMenuGroups = newConfigData.SpawnMenuSorting;

		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
			ZomberryBase.GetPermsMgr().SetConfigData(newConfigData.PermissionGroups, newConfigData.AdminList);

		if (zbryDebug != 0) ZomberryBase.Log( "ZomBerryConfig", "INFO: Debug level " + zbryDebug );
		if (defaultIOFlag) ZomberryBase.Log( "ZomBerryConfig", "INFO: Will use script log files..." );
	}

	private void ConfigureClient() {
		if (GetGame().IsServer() && GetGame().IsMultiplayer()) return;

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

	private void PrepareFiles() {
		if (!FileExist(cfgPath + "ZomBerryConfig.json")) {
			autoptr ZBerryJsonConfig newConfigData = new ZBerryJsonConfig();

			ZomberryBase.Log( "ZomBerryConfig", "INFO: Trying to create new config file in " + cfgPath + "ZomBerryConfig.json" );

			newConfigData.DebugLevel = zbryDebug;
			newConfigData.UseScriptLog = false;
			newConfigData.SpawnMenuSorting = 	{
													new ZBerryJsonSpawnMenuGroup("Items","Edible_Base,Weapon_Base,Magazine_Base,Clothing_Base"),
													new ZBerryJsonSpawnMenuGroup("Objects","Transport,House"),
													new ZBerryJsonSpawnMenuGroup("AI","DZ_LightAI"),
											 	};
			newConfigData.PermissionGroups =	{
													new ZBerryPermGroup(0, COLOR_RED, "Supreme Admins", {"*"}),
											 	};
			newConfigData.AdminList = 	{
											new ZBerryAdmin("Vaker", "76561198038543835", "Supreme Admins"),
										};

			JsonFileLoader<ZBerryJsonConfig>.JsonSaveFile(cfgPath + "ZomBerryConfig.json", newConfigData);
		}

		if (!FileExist(cfgPath + "ZomBerryKeybinds.bin") && GetGame().IsClient()) {
			FileSerializer kBindsFile = new FileSerializer();
			autoptr TStringIntMap keyBindList = new TStringIntMap;

			ZomberryBase.Log( "ZomBerryConfig", "INFO: keybinds file not found, trying to create new one in " + cfgPath + "ZomBerryKeybinds.bin" );

			keyBindList.Insert("DeleteObj", KeyCode.KC_DELETE);

			if (kBindsFile.Open(cfgPath + "ZomBerryKeybinds.bin", FileMode.WRITE)) {
				ZomberryBase.Log( "ZomBerryConfig", "INFO: keybinds file created successfully." );
				kBindsFile.Write(keyBindList);
				kBindsFile.Close();
			}
		}
	}

	void UpdatePermissions(ref ZBerryPermGroupArray pGroups, ref ZBerryAdminArray aList) {
		if (GetGame().IsClient() || !GetGame().IsMultiplayer()) return;

		autoptr ZBerryJsonConfig newConfigData = new ZBerryJsonConfig();

		ZomberryBase.Log( "ZomBerryConfig", "INFO: Updating config file: " + cfgPath + "ZomBerryConfig.json" );

		newConfigData.DebugLevel = zbryDebug;
		newConfigData.UseScriptLog = defaultIOFlag;
		newConfigData.SpawnMenuSorting = {};
		newConfigData.PermissionGroups = pGroups;
		newConfigData.AdminList = aList;

		JsonFileLoader<ZBerryJsonConfig>.JsonSaveFile(cfgPath + "ZomBerryConfig.json", newConfigData);
	}
};
