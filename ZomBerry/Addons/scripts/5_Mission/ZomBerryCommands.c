class ZomberryCmdAPI {
	protected autoptr ZBerryCategories m_oCategoryList = new ZBerryCategories;
	protected autoptr ZBerryFunctions m_oFunctionsList = new ZBerryFunctions;

	protected int lastId = 0; //last Category id

	void ZomberryCmdAPI() {

	}

	void AddCategory(string catName, int funcColor) {
		m_oCategoryList.Insert(catName, {lastId, funcColor});
		++lastId;

		Print("[ZomBerryCmdAPI] INFO: Added category " + catName);
	}

	void AddCommand(string dispName, string funcName, Class instance, string catName) {
		if (m_oCategoryList.Contains(catName)) {
			auto funcParam = new ref ZBerryFuncWrapper(dispName, funcName, catName, instance, m_oCategoryList.Get(catName).Get(1));

			m_oFunctionsList.Insert(funcParam);

			Print("[ZomBerryCmdAPI] INFO: Added function " + funcName + " to category " + catName);
		} else {
			Print("[ZomBerryCmdAPI] WARN: Unknown category " + catName + ", cannot add function " + funcName);
		}
	}

	ZBerryFuncWrapper GetFunc(string funcName) {
		int funcId = 3;

		for (int i = 0; i < m_oFunctionsList.Count(); ++i) {
			if (m_oFunctionsList.Get(funcId).GetName() == funcName) {
				funcId = i;
				break;
			}
		}

		return m_oFunctionsList.Get(funcId);
	}

	ZBerryFunctions GetFunctions() {
		return m_oFunctionsList;
	}

	ZBerryCategories GetCategories() {
		return m_oCategoryList;
	}
};
