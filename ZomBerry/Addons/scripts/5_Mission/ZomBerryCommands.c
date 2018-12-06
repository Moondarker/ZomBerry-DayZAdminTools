class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList = new ZBerryCategoryArray;

	void ZomberryCmdAPI() {

	}

	void AddCategory(string catName, int funcColor) {
		ref ZBerryCategory catParam = new ZBerryCategory(catName, funcColor);

		m_oCategoryList.Insert(catParam);
		GetZomberryLogger().Log( "ZomBerryCmdAPI", "INFO: Added category " + catName);
	}

	void AddCommand(string dispName, string funcName, Class instance, string catName) {
		if (m_oCategoryList.Contains(catName)) {
			int funcColor = m_oCategoryList.GetByName(catName).GetColor();
			ref ZBerryFuncParamArray funcParamArray = new ZBerryFuncParamArray(true);
			funcParamArray.Insert(new ZBerryFuncParam("Not Really A Param", Math.RandomInt(0, 50), Math.RandomInt(51, 100), false));

			ref ZBerryFunction funcData = new ZBerryFunction(dispName, funcName, catName, instance, funcColor, funcParamArray);

			m_oCategoryList.GetByName(catName).Insert(funcData);

			GetZomberryLogger().Log( "ZomBerryCmdAPI", "INFO: Added function " + funcName + " to category " + catName);
		} else {
			GetZomberryLogger().Log( "ZomBerryCmdAPI", "WARN: Unknown category " + catName + ", cannot add function " + funcName);
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
		for (int i = 0; i < m_oCategoryList.Count(); ++i) { //Full list breakdown
			ref ZBerryCategory catEntry = m_oCategoryList.Get(i);
			catEntry.Debug();
		}
	}
};
