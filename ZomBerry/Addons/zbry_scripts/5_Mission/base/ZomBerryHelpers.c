typedef array<ref ZBerryPlayer> ZBerryPlayerArray;

typedef array<ref ZBerryFunction> ZBerryFunctionArray;

typedef array<ref ZBerryJsonSpawnMenuGroup> ZBerryJsonSpawnMenuGroupArray;

typedef array<ref ZBerryJsonPermissionsGroup> ZBerryJsonPermissionsGroupArray;

class ZBerryPlayer {
	int m_PlayerID;
	string m_PlayerName;
	bool m_IsAdmin;
	vector m_PlayerPos;
	int m_PlayerHealth;
	int m_PlayerBlood;

	void ZBerryPlayer( int uid, string plyName, bool isAdmin, vector plyPos, int plyHP, int plyBld ) {
		m_PlayerID = uid;
		m_PlayerName = plyName;
		m_IsAdmin = isAdmin;
		m_PlayerPos = plyPos;
		m_PlayerHealth = plyHP;
		m_PlayerBlood = plyBld;
	}
}

class ZBerryCategoryArray: array<ref ZBerryCategory> {
	bool Contains(string catName) {
		for (int i = 0; i < Count(); ++i) {
			if (Get(i).GetName() == catName) return true;
		}
		return false;
	}

	ref ZBerryCategory GetByName(string catName) {
		for (int i = 0; i < Count(); ++i) {
			if (Get(i).GetName() == catName) return Get(i);
		}
		return NULL;
	}
}

class ZBerryFuncParamArray: array<ref ZBerryFuncParam> {

	void Debug() {
		for (int idx = 0; idx < Count(); ++idx) {
			PrintString( "   [" + idx.ToString() + "]: " + Get(idx).Debug());
		}
	}
};

class ZBerryCategory {
	protected string m_CategoryName;
	protected int m_CategoryColor;
	protected ref ZBerryFunctionArray m_FunctionArray;

	void ZBerryCategory( string cName, int cColor ) {
		m_CategoryName = cName;
		m_CategoryColor = cColor;
		m_FunctionArray = new ZBerryFunctionArray();
	}

	string GetName() {
		return m_CategoryName;
	}

	int GetColor() {
		return m_CategoryColor;
	}

	void Insert(ref ZBerryFunction newFunc) {
		m_FunctionArray.Insert(newFunc);
	}

	bool IsAvailable(int funcId) {
		for (int i = 0; i < m_FunctionArray.Count(); ++i) {
			if (m_FunctionArray.Get(i).GetId() == funcId) {
				return true;
			}
		}

		return false;
	}

	ref ZBerryFunction Get(int funcId) {
		for (int i = 0; i < m_FunctionArray.Count(); ++i) {
			if (m_FunctionArray.Get(i).GetId() == funcId) {
				return m_FunctionArray.Get(i);
			}
		}

		return NULL;
	}

	ref ZBerryFunction GetByName(string funcName) {
		for (int i = 0; i < m_FunctionArray.Count(); ++i) {
			if (m_FunctionArray.Get(i).GetName() == funcName) {
				return m_FunctionArray.Get(i);
			}
		}

		return NULL;
	}

	ref ZBerryFunctionArray GetAll() {
		return m_FunctionArray;
	}

	void Debug() {
		Print( " " );Print( " " );
		PrintString( "------Category separator------" );
		PrintString( "[m_CategoryName] => " + m_CategoryName );
		PrintString( "[m_CategoryColor] => " + m_CategoryColor.ToString() );
		PrintString( "[m_FunctionArray] => array (" + m_FunctionArray.Count() + ")" );
		for (int idx = 0; idx < m_FunctionArray.Count(); ++idx) {
			m_FunctionArray.Get(idx).Debug();
		}
	}
};

class ZBerryFunction {
	protected int m_FunctionId;
	protected string m_DisplayName;
	protected string m_ActualName;
	protected string m_CategoryName;
	protected Class m_Instance;
    protected int m_DisplayColor;
	protected bool m_OnTarget;
	protected ref ZBerryFuncParamArray m_Parameters;

	void ZBerryFunction( int fID, string dName, string aName, string cName, Class instance, int dColor, bool tNeeded, ref ZBerryFuncParamArray fParam ) {
		m_FunctionId = fID;
		m_DisplayName = dName;
		m_ActualName = aName;
		m_CategoryName = cName;
        m_Instance = instance;
        m_DisplayColor = dColor;
		m_OnTarget = tNeeded;
		m_Parameters = fParam;
    }

	int GetId() {
        return m_FunctionId;
    }

	string GetDisplayName() {
        return m_DisplayName;
    }

	string GetName() {
		return m_ActualName;
	}

    string GetCategory() {
        return m_CategoryName;
    }

	Class GetInstance() {
        return m_Instance;
    }

    int GetColor() {
        return m_DisplayColor;
    }

	bool IsTargetRequired() {
		return m_OnTarget;
	}

	ref ZBerryFuncParamArray GetParams() {
		return m_Parameters;
	}

	void SetColor(int funcColor) {
		m_DisplayColor = funcColor;
	}

	void Debug() {
		Print( " " );
		PrintString( "------m_FunctionArray internals------" );
		PrintString( "[m_DisplayName] => " + m_DisplayName );
		PrintString( "[m_ActualName] => " + m_ActualName );
		PrintString( "[m_CategoryName] => " + m_CategoryName );
		PrintString( "[m_Instance] => " + string.ToString(m_Instance) );
		PrintString( "[m_DisplayColor] => " + m_DisplayColor.ToString() );
		PrintString( "[m_OnTarget] => " + m_OnTarget.ToString() );
		PrintString( "[m_Parameters] => array (" + m_Parameters.Count() + ")" );

		if (m_Parameters.Count() > 0) m_Parameters.Debug();
	}
}

class ZBerryFuncParam {
	string name;
	ref TIntArray values;

	void ZBerryFuncParam( string pName, ref TIntArray pValues ) {
		name = pName;
		values = new TIntArray;
		values.InsertAll(pValues);
	}

	string Debug() {
		return "\"" + name + "\" - min: " + values[0].ToString() + ", max: " + values[1].ToString() + ", default: " + values[2].ToString();
	}
}

class ZBerryJsonConfig {
	int DebugLevel;
	bool UseScriptLog;
	string MenuKey;
	ref ZBerryJsonSpawnMenuGroupArray SpawnMenuSorting;
	ref ZBerryJsonPermissionsGroupArray PermissionGroups;
};

class ZBerryJsonSpawnMenuGroup {
	string CategoryName;
	string BaseClassNames;

	void ZBerryJsonSpawnMenuGroup(string name, string classes) {
		CategoryName = name;
		BaseClassNames = classes;
	}
};

class ZBerryJsonPermissionsGroup {
	string GroupName;
	ref TStringArray AllowedFunctions;
};
