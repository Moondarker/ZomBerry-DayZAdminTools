typedef array<ref ZBerryPlayer> ZBerryPlayerArray;

typedef array<ref ZBerryFunction> ZBerryFunctionArray;

typedef array<ref ZBerryJsonSpawnMenuGroup> ZBerryJsonSpawnMenuGroupArray;


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
	void Clone(ZBerryFuncParamArray original) {
		for (int idx = 0; idx < original.Count(); ++idx) {
			Insert(new ZBerryFuncParam(original.Get(idx).name, original.Get(idx).values, original.Get(idx).valueNames));
		}
	}

	override void Debug() {
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

	void Insert(ZBerryFunction newFunc) {
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
	protected bool m_Harmful; //Hurts or moves the target: groups can protect their members from it (zomberry.protect.*.harm)
	protected bool m_ServerWide; //Acts on the world rather than on a player (time, weather, cleanup), for the log

	void ZBerryFunction( int fID, string dName, string aName, string cName, Class instance, int dColor, bool tNeeded, ZBerryFuncParamArray fParam, bool harmful = false, bool serverWide = false ) {
		m_Harmful = harmful;
		m_ServerWide = serverWide;
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

	bool IsHarmful() {
		return m_Harmful;
	}

	bool IsServerWide() {
		return m_ServerWide;
	}

	//"Set weather (Fog: 0, Overcast: 80, Rain: 100)", named values by name, for the log
	string DescribeCall(TIntArray values) {
		if (m_Parameters.Count() == 0 || !values) return m_DisplayName;

		string text = m_DisplayName + " (";
		for (int i = 0; i < m_Parameters.Count() && i < values.Count(); ++i) {
			if (i > 0) text += ", ";
			ZBerryFuncParam param = m_Parameters[i];
			string valueName = param.GetValueName(values[i]);
			if (valueName == "") valueName = values[i].ToString();
			text += param.name + ": " + valueName;
		}
		return text + ")";
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

//A function parameter: an integer in [min, max]. Optional value names are shown instead of the number,
//the range is split evenly between them (one name per value when there are as many names as values)
class ZBerryFuncParam {
	string name;
	ref TIntArray values; //min, max, default
	ref TStringArray valueNames; //Empty when the parameter has no named values

	void ZBerryFuncParam( string pName, TIntArray pValues, TStringArray pValueNames = NULL ) {
		name = pName;
		values = new TIntArray;
		values.InsertAll(pValues);
		valueNames = new TStringArray;
		if (pValueNames) valueNames.InsertAll(pValueNames);
	}

	bool HasValueNames() {
		return valueNames.Count() > 0;
	}

	//Name covering this value, "" without names. Typed locals and Floor/Ceil on purpose:
	//Enforce doesn't do plain integer math on mixed array/method operands
	string GetValueName(int value) {
		float minValue = values.Get(0);
		float maxValue = values.Get(1);
		float range = maxValue - minValue + 1;
		float nameCount = Math.Min(valueNames.Count(), range); //Names beyond one per value are never reached
		if (nameCount <= 0) return "";

		float offset = Math.Clamp(value, minValue, maxValue) - minValue;
		int index = Math.Floor(offset * nameCount / range);
		return valueNames.Get(Math.Min(index, nameCount - 1));
	}

	//First value covered by this name (case-insensitive), false when there's no such name
	bool FindNameValue(string valueName, out int value) {
		float minValue = values.Get(0);
		float maxValue = values.Get(1);
		float range = maxValue - minValue + 1;
		float nameCount = Math.Min(valueNames.Count(), range);

		valueName.ToLower();
		for (int index = 0; index < nameCount; ++index) {
			string candidate = valueNames.Get(index);
			candidate.ToLower();
			if (candidate != valueName) continue;

			float firstOffset = Math.Ceil(index * range / nameCount);
			value = minValue + firstOffset;
			return true;
		}

		return false;
	}

	string Debug() {
		string result = "\"" + name + "\" - min: " + values[0].ToString() + ", max: " + values[1].ToString() + ", default: " + values[2].ToString();
		if (HasValueNames()) result += ", names: " + valueNames.Count();

		return result;
	}
}

class ZBerryJsonConfig {
	int DebugLevel;
	bool UseScriptLog;
	ref ZBerryJsonSpawnMenuGroupArray SpawnMenuSorting;
};

class ZBerryJsonSpawnMenuGroup {
	string CategoryName;
	string BaseClassNames;

	void ZBerryJsonSpawnMenuGroup(string name, string classes) {
		CategoryName = name;
		BaseClassNames = classes;
	}
};
