class ZomberryKeybinds {
	protected autoptr array<ref ZBerryFuncParamArray> m_funcParams = new array<ref ZBerryFuncParamArray>;
	protected autoptr TIntIntMap m_dKeyBindList = new TIntIntMap;
	protected autoptr TIntStringMap m_funcIdNameMap = new TIntStringMap;

	void ZomBerryKeybinds() {
		//m_assocList =
	}

	vector GetCursorPos() { //<3 Arkensor and CO
		if ( !GetGame().GetPlayer() ) { return "0 0 0"; }

		vector rayStart = GetGame().GetCurrentCameraPosition();
		vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
		vector hitPos; vector hitNormal; int hitComponentIndex;
		DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, GetGame().GetPlayer());

		return hitPos;
	}

	void OnKeyPress( int key ) {
		int funcId, adminId;
		if (m_dKeyBindList.Find(key, funcId)) {
			if (GetGame().IsMultiplayer()) {
				adminId = GetGame().GetPlayer().GetIdentity().GetPlayerId();
			} else {
				adminId = 0;
			}
			GetRPCManager().SendRPC( "ZomBerryAT", "ExecuteCommand", new Param5< int, int, int, vector, autoptr TIntArray >( funcId, adminId, adminId, GetCursorPos(), {0, 0, 0} ), true );
		};
	}

	void ClearFuncList() {
		m_dKeyBindList.Clear();
		m_funcIdNameMap.Clear();
	}

	int GetFuncKey(int fId) {
		return m_dKeyBindList.GetKeyByValue(fId);
	}

	void SetFuncKey(int fId, int keyId) {
		if (fId != -2) {
			ZomberryBase.Log( "ZomBerryKeybinds", "SetFuncKey: Setting " + fId + " to " + keyId );
			int toBeDeleted = m_dKeyBindList.GetKeyByValue(fId);

			if (toBeDeleted) m_dKeyBindList.ReplaceKey(toBeDeleted, keyId);
			if (m_dKeyBindList.Contains(keyId)) {
				m_dKeyBindList.Set(keyId, fId);
			} else {
				m_dKeyBindList.Insert(keyId, fId);
			}
			if (fId == -1) {
				m_dKeyBindList.Remove(keyId);
				ZomberryBase.GetConfig().UpdateKeyBind("REMOVE", keyId);
				return;
			}

			ZomberryBase.GetConfig().UpdateKeyBind(m_funcIdNameMap.Get(fId), keyId);
		} else {
			m_dKeyBindList.Clear();
			ZomberryBase.Log( "ZomBerryKeybinds", "SetFuncKey: Removing ALL keybinds!" );
			ZomberryBase.GetConfig().UpdateKeyBind("ALL", -2);
		}
	}

	void AddFunc(int fId, string fName) {
		m_funcIdNameMap.Insert(fId, fName);

		int keyId = ZomberryBase.GetConfig().GetKeyByFunc(fName);
		if (keyId == -1) return;

		m_dKeyBindList.Insert(keyId, fId);
	}
}
