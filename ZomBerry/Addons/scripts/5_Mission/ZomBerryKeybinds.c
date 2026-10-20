//Function keybinds of this client. A key runs one function, a function has at most one key.
//Binds are saved by function name (ZomberryConfig), since function ids can change between server restarts
class ZomberryKeybinds {
	protected ref TIntIntMap m_KeyToFunc = new TIntIntMap; //Key -> function id
	protected ref TIntStringMap m_FuncNames = new TIntStringMap; //Function id -> name

	void OnKeyPress( int key ) {
		int funcId;
		if (!m_KeyToFunc.Find(key, funcId)) return;

		int adminId = ZBerryUIUtils.GetLocalPlayerId(); //Also without a character (dead, respawning)
		TIntArray values = {0, 0, 0}; //Local: a temporary inside the Param would be freed before it's sent
		GetZBerryRPC().SendRPC( ZBerryRPCOp.ExecuteCommand, new Param4< int, int, vector, autoptr TIntArray >( funcId, adminId, ZBerryUIUtils.GetCursorPos(), values ), true );
	}

	//The server sent a new function list
	void ClearFuncList() {
		m_KeyToFunc.Clear();
		m_FuncNames.Clear();
	}

	void AddFunc(int funcId, string funcName) {
		m_FuncNames.Set(funcId, funcName);

		int key = ZomberryBase.GetConfig().GetKeyByFunc(funcName);
		if (key != -1) m_KeyToFunc.Set(key, funcId);
	}

	//-1 when the function has no key
	int GetKey(int funcId) {
		foreach (int key, int boundFunc : m_KeyToFunc) {
			if (boundFunc == funcId) return key;
		}
		return -1;
	}

	//Replaces the function's old key, and whatever function had this key before
	void Bind(int funcId, int key) {
		string funcName = m_FuncNames.Get(funcId);
		if (funcName == "") return;

		RemoveKeyOf(funcId);
		m_KeyToFunc.Set(key, funcId);
		ZomberryBase.GetConfig().SaveKeyBind(funcName, key);
	}

	void Unbind(int funcId) {
		if (!RemoveKeyOf(funcId)) return;

		ZomberryBase.GetConfig().RemoveKeyBind(m_FuncNames.Get(funcId));
	}

	void ClearAll() {
		m_KeyToFunc.Clear();
		ZomberryBase.GetConfig().ClearKeyBinds();
	}

	protected bool RemoveKeyOf(int funcId) {
		int key = GetKey(funcId);
		if (key == -1) return false;

		m_KeyToFunc.Remove(key);
		return true;
	}
}
