class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList = new ZBerryCategoryArray;
	//protected ref ZBerryFunctionArray m_oFunctionsList = new ZBerryFunctionArray;

	protected int lastId = 0; //last Category id

	void ZomberryCmdAPI() {

	}

	void AddCategory(string catName, int funcColor) {
		ref ZBerryCategory catParam = new ZBerryCategory(catName, funcColor);

		GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "catParam: " + catParam);
		GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "catParam Name: " + catParam.GetName());
		GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "catParam Color: " + catParam.GetColor());

		m_oCategoryList.Insert(catParam);

		GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "catParam Contains: " + m_oCategoryList.Contains(catName));
		++lastId;

		GetZomberryLogger().Log( "ZomBerryCmdAPI", "INFO: Added category " + catName);
	}

	void AddCommand(string dispName, string funcName, Class instance, string catName) {
		if (m_oCategoryList.Contains(catName)) {
			ref ZBerryFunction funcParam = new ZBerryFunction(dispName, funcName, catName, instance, m_oCategoryList.GetByName(catName).GetColor());

			GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "funcParam: " + funcParam);

			m_oCategoryList.GetByName(catName).Insert(funcParam);

			GetZomberryLogger().Log( "ZomBerryCmdAPIDbg", "funcParamInCat: " + m_oCategoryList.GetByName(catName).Get(funcName));

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

	/*ref ZBerryCategory GetByName(ZBerryCategory string catName) { //Не дописано
		for (int i = 0; i < Count(); ++i) {
			if (Get(i).GetName() == catName) return Get(i);
		}
		return NULL;
	}*/
	/*
	ref ZBerryFunctionArray GetFunctions() {
		return m_oFunctionsList;
	}*/

	ref ZBerryCategoryArray GetList() {
		return m_oCategoryList;
	}
};
