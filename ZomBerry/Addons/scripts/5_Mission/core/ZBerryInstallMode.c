//-zbryInstallMode=true: guided admins.cfg setup through the function list, without real players or spawning
class ZBerryInstallMode {
	static const string CATEGORY = "ZomBerry Installation Mode";
	static const string ADDON_CONFIG_DIR = "$CurrentDir:\\ZomBerry\\Config\\";

	protected int m_Stage = 1;
	protected ZBerryServerRPC m_ServerRPC; //To push the grown function list to the admin

	void ZBerryInstallMode(ZBerryServerRPC serverRPC) {
		m_ServerRPC = serverRPC;
	}

	static bool IsActive() {
		string value;
		return GetCLIParam("zbryInstallMode", value) && value == "true";
	}

	void RegisterCommands() {
		GetZomberryCmdAPI().AddCategory(CATEGORY, COLOR_RED);
		AddStep("List of admins in file", 0);
		AddStep("Step 1: Check if admins.cfg was found", 1);
	}

	protected void AddStep(string displayName, int step) {
		TIntArray values = new TIntArray;
		values.Insert(step);
		values.Insert(step);
		values.Insert(step);

		ZBerryFuncParamArray funcParams = new ZBerryFuncParamArray;
		funcParams.Insert(new ZBerryFuncParam("Step", values));

		GetZomberryCmdAPI().AddCommand(displayName, "InstMode", this, CATEGORY, false, funcParams);
	}

	void InstMode( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		int step = fValues[0];
		string admPath = "";
		TStringArray outMessages = {};
		PlayerBase adminPly = ZBGetPlayerById(adminId);

		if (!IsActive() || !adminPly) return;

		switch (step) {
			case 0:
				outMessages.Insert("List of admins in admins.cfg (if exists):");
				outMessages.InsertAll(ZomberryBase.GetConfig().ConfigureAdmins());
				break;

			case 1:
				admPath = ZomberryBase.GetConfig().InstFindAdmins();
				if (admPath == "") {
					outMessages.Insert("admins.cfg was not found");
				} else {
					if (admPath == ADDON_CONFIG_DIR) outMessages.Insert("admins.cfg was found at default mod location, please try to create it in proper location!");
					outMessages.Insert("admins.cfg was found at: " + admPath);
				}

				if (m_Stage == 1) {
					AddStep("Step 2: Try to create admins.cfg in proper location", 2);
					if (admPath != "" && admPath != ADDON_CONFIG_DIR) {
						AddStep("Step 3: Try to add your BIGUID into file", 3);
						++m_Stage;
					}
					++m_Stage;
					m_ServerRPC.SendFunctionList(adminPly.GetIdentity());
				}
				break;

			case 2:
				string stageTwoResult = ZomberryBase.GetConfig().InstCreateAdmins();
				outMessages.Insert("admins.cfg create result: " + stageTwoResult);

				if (m_Stage == 2 && (stageTwoResult == "OK" || stageTwoResult == "AlreadyExists")) {
					AddStep("Step 3: Try to add your BIGUID into file", 3);
					++m_Stage;
					m_ServerRPC.SendFunctionList(adminPly.GetIdentity());
				}
				break;

			case 3:
				if (ZomberryBase.GetConfig().InstInsertAdmin(adminPly.GetIdentity().GetId())) {
					outMessages.Insert("Your BIGUID was successfully added into admins.cfg!");
				} else {
					outMessages.Insert("Can't add your BIGUID into admins.cfg");
				}
				break;
		}

		foreach (string message : outMessages) {
			ZomberryStockFunctions.MessagePlayer(adminPly, message);
		}
	}
};
