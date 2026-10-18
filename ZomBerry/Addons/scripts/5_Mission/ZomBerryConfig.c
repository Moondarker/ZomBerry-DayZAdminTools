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

	//Where this side keeps its files: $profile:ZomBerry on servers, $saves: on clients
	string GetConfigDir() {
		return cfgPath;
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

	private void CreateNew(string dPath) {
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
};
