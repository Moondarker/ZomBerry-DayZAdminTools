class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList;
	protected int m_lastFuncId;

	void ZomberryCmdAPI() {
		m_lastFuncId = 0;
		m_oCategoryList = new ZBerryCategoryArray();
	}

	void ~ZomberryCmdAPI() {
		delete m_oCategoryList;
	}

	void AddCategory(string catName, int funcColor) {
		if (m_oCategoryList.Contains(catName)) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Cannot add category " + catName + ", category with same name already exists.");
			return;
		}

		ref ZBerryCategory catParam = new ZBerryCategory(catName, funcColor);

		m_oCategoryList.Insert(catParam);
		ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added category " + catName);
	}

	void AddCommand(string dispName, string funcName, Class instance, string catName, bool onTarget = true, autoptr ZBerryFuncParamArray funcParams = NULL) {
		if (m_oCategoryList.Contains(catName)) {
			if (m_oCategoryList.GetByName(catName).GetByName(funcName) != NULL) {
				ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Adding " + funcName + ", but function with same name already exists.");
			}

			int funcColor = m_oCategoryList.GetByName(catName).GetColor();

			if (!funcParams) funcParams = new ZBerryFuncParamArray;

			ref ZBerryFunction funcData = new ZBerryFunction(m_lastFuncId, dispName, funcName, catName, instance, funcColor, onTarget, funcParams);
			++m_lastFuncId;

			m_oCategoryList.GetByName(catName).Insert(funcData);

			ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added function " + funcName + " to category " + catName);
		} else {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Unknown category " + catName + ", cannot add function " + funcName);
		}
	}

	ZBerryFunction GetFunc(int funcId) {
		for (int i = 0; i < m_oCategoryList.Count(); ++i) {
			if (m_oCategoryList.Get(i).IsAvailable(funcId)) {
				return m_oCategoryList.Get(i).Get(funcId);
			}
		}

		return NULL;
	}

	ref ZBerryCategoryArray GetList() {
		return m_oCategoryList;
	}

	void Debug() {
		ZomberryBase.Log( "ZomBerryCmdAPIDbg", "Logging entire commands list to script.log");
		for (int i = 0; i < m_oCategoryList.Count(); ++i) { //Full list breakdown
			ref ZBerryCategory catEntry = m_oCategoryList.Get(i);
			catEntry.Debug();
		}
	}
};
