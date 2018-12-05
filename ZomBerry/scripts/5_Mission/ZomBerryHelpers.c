typedef array<ref ZBerryPlayer> ZBerryPlayerArray;
typedef array<ref ZBerryFuncWrapper> ZBerryFunctions;
typedef map<string, ref TIntArray> ZBerryCategories;

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

class ZBerryFuncWrapper {
	protected string m_DisplayName;
	protected string m_ActualName;
	protected string m_CategoryName;
	protected Class m_Instance;
    protected int m_DisplayColor;

	void ZBerryFuncWrapper( string dName, string aName, string cName, Class instance, int dColor ) {
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
}
