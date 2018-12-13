class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList = new ZBerryCategoryArray;

	void ZomberryCmdAPI() {

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
			if (GetFunc(funcName) != NULL) {
				ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Cannot add function " + funcName + ", function with same name already exists.");
				return;
			}

			int funcColor = m_oCategoryList.GetByName(catName).GetColor();

			if (!funcParams) funcParams = new ZBerryFuncParamArray;

			ref ZBerryFunction funcData = new ZBerryFunction(dispName, funcName, catName, instance, funcColor, onTarget, funcParams	);

			m_oCategoryList.GetByName(catName).Insert(funcData);

			ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added function " + funcName + " to category " + catName);
		} else {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Unknown category " + catName + ", cannot add function " + funcName);
		}
	}

	ZBerryFunction GetFunc(string funcName) {
		for (int i = 0; i < m_oCategoryList.Count(); ++i) {
			if (m_oCategoryList.Get(i).IsAvailable(funcName)) {
				return m_oCategoryList.Get(i).Get(funcName);
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
