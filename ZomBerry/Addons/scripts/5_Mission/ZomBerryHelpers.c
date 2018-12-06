typedef array<ref ZBerryPlayer> ZBerryPlayerArray;

typedef array<ref ZBerryFunction> ZBerryFunctionArray;

//typedef array<ref ZBerryCategory> ZBerryCategoryArray;

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

class ZBerryPlayer {
	int m_PlayerID;
	string m_PlayerName;
	bool m_IsAdmin;

	void ZBerryPlayer( int uid, string plyName, bool isAdmin ) {
		m_PlayerID = uid;
		m_PlayerName = plyName;
		m_IsAdmin = isAdmin;
	}
}

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

	bool IsAvailable(string funcName) {
		for (int i = 0; i < m_FunctionArray.Count(); ++i) {
			if (m_FunctionArray.Get(i).GetName() == funcName) {
				return true;
			}
		}

		return false;
	}

	ref ZBerryFunction Get(string funcName) {
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
		Print( " " );
		for (int idx = 0; idx < m_FunctionArray.Count(); ++idx) {
			m_FunctionArray.Get(idx).Debug();
		}
	}
};

class ZBerryFunction {
	protected string m_DisplayName;
	protected string m_ActualName;
	protected string m_CategoryName;
	protected Class m_Instance;
    protected int m_DisplayColor;

	void ZBerryFunction( string dName, string aName, string cName, Class instance, int dColor ) {
		m_DisplayName = dName;
		m_ActualName = aName;
		m_CategoryName = cName;
        m_Instance = instance;
        m_DisplayColor = dColor;
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

	void SetColor(int funcColor) {
		m_DisplayColor = funcColor;
	}

	void Debug() {
		PrintString( "------m_FunctionArray internals------" );
		PrintString( "[m_DisplayName] => " + m_DisplayName );
		PrintString( "[m_ActualName] => " + m_ActualName );
		PrintString( "[m_CategoryName] => " + m_CategoryName );
		PrintString( "[m_Instance] => " + string.ToString(m_Instance) );
		PrintString( "[m_DisplayColor] => " + m_DisplayColor.ToString() );
	}
}
