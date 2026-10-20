//How a command call ended, for the log: commands report anything but Done themselves (RefuseCall, AwaitConfirmation)
enum ZBerryCallOutcome {
	DONE,
	REFUSED, //Nothing happened: logged as refused, with the reason
	CONFIRMING //It only asked the admin to run it again: not logged, the confirmed run is
};

class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList = new ZBerryCategoryArray;
	protected int m_lastFuncId = 0;
	protected int m_CallOutcome = ZBerryCallOutcome.DONE;
	protected string m_CallReason = "";

	void AddCategory(string catName, int funcColor) {
		if (m_oCategoryList.Contains(catName)) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Cannot add category " + catName + ", category with same name already exists.");
			return;
		}

		ref ZBerryCategory catParam = new ZBerryCategory(catName, funcColor);

		m_oCategoryList.Insert(catParam);
		ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added category " + catName);
	}

	//harmful: the function hurts, moves or otherwise works against its target. Groups can protect their members
	//from harmful functions run by lower-ranked or same-level admins (zomberry.protect.lower.harm, zomberry.protect.same.harm)
	//serverWide: it acts on the world rather than on a player (time, weather, cleanup), so the log files it that way
	void AddCommand(string dispName, string funcName, Class instance, string catName, bool onTarget = true, ZBerryFuncParamArray funcParams = NULL, bool harmful = false, bool serverWide = false) {
		if (m_oCategoryList.Contains(catName)) {
			if (m_oCategoryList.GetByName(catName).GetByName(funcName) != NULL) {
				ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Adding " + funcName + ", but function with same name already exists.");
			}

			int funcColor = m_oCategoryList.GetByName(catName).GetColor();

			ZBerryFuncParamArray params = funcParams; //Local keeps the new empty array alive, arguments are weak references
			if (!params) params = new ZBerryFuncParamArray;

			ref ZBerryFunction funcData = new ZBerryFunction(m_lastFuncId, dispName, funcName, catName, instance, funcColor, onTarget, params, harmful, serverWide);
			++m_lastFuncId;

			m_oCategoryList.GetByName(catName).Insert(funcData);

			ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added function " + funcName + " to category " + catName);
		} else {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Unknown category " + catName + ", cannot add function " + funcName);
		}
	}

	/**
	\brief Server: switch a client-side feature on or off for one player's client. Typically called from a command after
		ZomBerry checked its permission (and rank): the client runs what RegisterClientFeature registered under this name
		\param identity \p PlayerIdentity whose client runs it (NULL in singleplayer)
		\param feature \p string name both sides agree on, like "mymod_overlay"
		\param args \p TIntArray passed to the client's handler (NULL: none)
		\param requiredNode \p string permission that keeps it on: it's switched off when the player loses it ("" for none)
	*/
	void SetClientFeature(PlayerIdentity identity, string feature, bool enabled, TIntArray args = NULL, string requiredNode = "") {
		GetZBerryClientFeatures().Set(identity, feature, enabled, args, requiredNode);
	}

	/**
	\brief Server, from inside a command: this call did nothing (no target, the wrong target, nothing to act on).
		The log files it as refused, with the reason, instead of as run. Tell the admin yourself (MessagePlayer)
		\param reason \p string why, as the admin was told ("Select another player to freeze")
	*/
	void RefuseCall(string reason) {
		m_CallOutcome = ZBerryCallOutcome.REFUSED;
		m_CallReason = reason;
	}

	/**
	\brief Server, from inside a command: this call only asked the admin to confirm by running it again.
		It isn't logged; the confirmed run is
	*/
	void AwaitConfirmation() {
		m_CallOutcome = ZBerryCallOutcome.CONFIRMING;
		m_CallReason = "";
	}

	//ZomBerry's dispatcher, around every command call (calls run synchronously, one at a time)
	void BeginCall() {
		m_CallOutcome = ZBerryCallOutcome.DONE;
		m_CallReason = "";
	}

	int GetCallOutcome() {
		return m_CallOutcome;
	}

	string GetCallReason() {
		return m_CallReason;
	}

	//Server: is the feature on for this player's client
	bool IsClientFeatureActive(PlayerIdentity identity, string feature) {
		return GetZBerryClientFeatures().IsActive(identity, feature);
	}

	/**
	\brief Client: what runs when the server switches this feature: void funcName(bool enabled, TIntArray args)
	*/
	void RegisterClientFeature(string feature, Class instance, string funcName) {
		GetZBerryClientFeatures().Register(feature, instance, funcName);
	}

	//Client: did the server switch this feature on for us
	bool IsClientFeatureEnabled(string feature) {
		return GetZBerryClientFeatures().IsEnabledLocally(feature);
	}

	ZBerryFunction GetFunc(int funcId) {
		for (int i = 0; i < m_oCategoryList.Count(); ++i) {
			if (m_oCategoryList.Get(i).IsAvailable(funcId)) {
				return m_oCategoryList.Get(i).Get(funcId);
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
