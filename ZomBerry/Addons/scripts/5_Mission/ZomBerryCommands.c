//How a command call ended, for the log: commands report anything but Done themselves (RefuseCall, AwaitConfirmation)
enum ZBerryCallOutcome {
	DONE,
	REFUSED, //Nothing happened: logged as refused, with the reason
	CONFIRMING //It only asked the admin to run it again: not logged, the confirmed run is
};

//One run of a command, from the dispatcher's BeginCall to its EndCall: who ran what on whom, and how it ended.
//Commands reach it through GetZomberryCmdAPI().GetCurrentCall(), or the RefuseCall/AwaitConfirmation/DescribeCall shortcuts
class ZBerryCall {
	protected ZBerryFunction m_Function;
	protected PlayerIdentity m_Admin, m_Target; //Target: NULL when run on the admin themselves or on nobody
	protected int m_Outcome = ZBerryCallOutcome.DONE;
	protected string m_Reason = "";
	protected string m_Description = "";
	protected ref ZBerryTeleportRecordArray m_Moves = new ZBerryTeleportRecordArray; //RecordMove: who this call moved, from where
	protected int m_TargetIndex = 0; //This call's target among the selected players (RunOnTargets), 0 of 1 otherwise
	protected int m_TargetCount = 1;
	protected vector m_Position; //SetCallPosition: where the command acted, for "Teleport here" in the Logs tab
	protected bool m_HasPosition;

	void ZBerryCall(ZBerryFunction function, PlayerIdentity admin, PlayerIdentity target) {
		m_Function = function;
		m_Admin = admin;
		m_Target = target;
	}

	ZBerryFunction GetFunction() {
		return m_Function;
	}

	PlayerIdentity GetAdmin() {
		return m_Admin;
	}

	PlayerIdentity GetTarget() {
		return m_Target;
	}

	int GetOutcome() {
		return m_Outcome;
	}

	string GetReason() {
		return m_Reason;
	}

	//"" when the command didn't describe itself: the log then says "ran <function>"
	string GetDescription() {
		return m_Description;
	}

	void Refuse(string reason) {
		m_Outcome = ZBerryCallOutcome.REFUSED;
		m_Reason = reason;
	}

	void AwaitConfirmation() {
		m_Outcome = ZBerryCallOutcome.CONFIRMING;
		m_Reason = "";
	}

	void Describe(string text) {
		m_Description = text;
	}

	ZBerryTeleportRecordArray GetMoves() {
		return m_Moves;
	}

	void AddMove(ZBerryTeleportRecord record) {
		m_Moves.Insert(record);
	}

	int GetTargetIndex() {
		return m_TargetIndex;
	}

	int GetTargetCount() {
		return m_TargetCount;
	}

	void SetTargetPosition(int index, int count) {
		m_TargetIndex = index;
		m_TargetCount = count;
	}

	void SetPosition(vector position) {
		m_Position = position;
		m_HasPosition = true;
	}

	//False when the command set none
	bool GetPosition(out vector position) {
		position = m_Position;
		return m_HasPosition;
	}
};

class ZomberryCmdAPI {
	protected ref ZBerryCategoryArray m_oCategoryList = new ZBerryCategoryArray;
	protected int m_lastFuncId = 0;
	protected ref array<ref ZBerryCall> m_Calls = new array<ref ZBerryCall>; //Running calls, innermost last

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
	//keybindable: false keeps it off keys, for commands that shouldn't run from a stray key press
	//multiTarget: the function can run on several selected players at once. ZomBerry then calls it once per player (the
	//usual signature) and logs one event with all of them. Without it, it only gets the last selected player
	void AddCommand(string dispName, string funcName, Class instance, string catName, bool onTarget = true, ZBerryFuncParamArray funcParams = NULL, bool harmful = false, bool serverWide = false, bool keybindable = true, bool multiTarget = false) {
		//Commands from before 0.8.0 had no serverWide: OnServer meant it, so their log entries get no player's position
		if (catName == "OnServer" && !m_oCategoryList.Contains(catName)) serverWide = true;
		if (!m_oCategoryList.Contains(catName)) catName = MapLegacyCategory(catName);
		if (m_oCategoryList.Contains(catName)) {
			if (m_oCategoryList.GetByName(catName).GetByName(funcName) != NULL) {
				ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Adding " + funcName + ", but function with same name already exists.");
			}

			int funcColor = m_oCategoryList.GetByName(catName).GetColor();

			ZBerryFuncParamArray params = funcParams; //Local keeps the new empty array alive, arguments are weak references
			if (!params) params = new ZBerryFuncParamArray;

			ref ZBerryFunction funcData = new ZBerryFunction(m_lastFuncId, dispName, funcName, catName, instance, funcColor, onTarget, params, harmful, serverWide, keybindable, multiTarget);
			++m_lastFuncId;

			m_oCategoryList.GetByName(catName).Insert(funcData);

			ZomberryBase.Log( "ZomBerryCmdAPI", "INFO: Added function " + funcName + " to category " + catName);
		} else {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: Unknown category " + catName + ", cannot add function " + funcName);
		}
	}

	//The stock categories before 0.8.0, which other mods' commands may still name
	protected static string MapLegacyCategory(string catName) {
		if (catName == "OnTarget") return ZomberryStockFunctions.CATEGORY_PLAYER;
		if (catName == "==") return ZomberryStockFunctions.CATEGORY_PUNISH;
		if (catName == "OnServer") return ZomberryStockFunctions.CATEGORY_WORLD;
		return catName;
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
		ZBerryCall call = GetCurrentCall();
		if (!call) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: RefuseCall outside a command call, not logged as refused: " + reason );
			return;
		}
		call.Refuse(reason);
	}

	/**
	\brief Server, from inside a command: this call only asked the admin to confirm by running it again.
		It isn't logged; the confirmed run is
	*/
	void AwaitConfirmation() {
		ZBerryCall call = GetCurrentCall();
		if (!call) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: AwaitConfirmation outside a command call, ignored" );
			return;
		}
		call.AwaitConfirmation();
	}

	/**
	\brief Server, from inside a command: what this call did, logged instead of "{actor} ran <function>". Same event type,
		harmful flag and target as that line would have had
		\param text \p string with {actor} for the admin and {target} for the selected player, as in
			"{actor} flipped Ada 4x4 at X 9872 Y 2170" or "{actor} froze {target}"
	*/
	void DescribeCall(string text) {
		ZBerryCall call = GetCurrentCall();
		if (!call) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: DescribeCall outside a command call, ignored: " + text );
			return;
		}
		call.Describe(text);
	}

	/**
	\brief Server, from inside a command that moves a player: where they were before, so the teleport can be reverted from
		the Logs tab. Call it after the move succeeded, with the position read before it
		\param player \p PlayerBase the moved player (players without a Steam64 id, as in singleplayer, aren't recorded)
		\param from \p vector where they were
	*/
	void RecordMove(PlayerBase player, vector from) {
		ZBerryCall call = GetCurrentCall();
		if (!call) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: RecordMove outside a command call, ignored" );
			return;
		}
		ZBerryTeleportRecord record = ZBerryTeleportReturns.MakeRecord(player, from);
		if (record.SteamId != "") call.AddMove(record);
	}

	/**
	\brief Server, from inside a command that acts at a spot (around the cursor, on an object): where, so admins can
		teleport there from the Logs tab. Commands run on a player don't need it, their log entry gets where the
		player is. Teleports recorded with RecordMove get where the players went
		\param position \p vector
	*/
	void SetCallPosition(vector position) {
		ZBerryCall call = GetCurrentCall();
		if (!call) {
			ZomberryBase.Log( "ZomBerryCmdAPI", "WARN: SetCallPosition outside a command call, ignored" );
			return;
		}
		call.SetPosition(position);
	}

	//The command call running right now, NULL outside one (a CallLater callback runs after its call has ended)
	ZBerryCall GetCurrentCall() {
		if (m_Calls.Count() == 0) return NULL;
		return m_Calls[m_Calls.Count() - 1];
	}

	//ZomBerry's dispatcher, around every command call. Calls can nest: each has its own outcome
	ZBerryCall BeginCall(ZBerryFunction function, PlayerIdentity admin, PlayerIdentity target) {
		ZBerryCall call = new ZBerryCall(function, admin, target);
		m_Calls.Insert(call);
		return call;
	}

	void EndCall(ZBerryCall call) {
		int index = m_Calls.Find(call);
		if (index == -1) return;

		m_Calls.RemoveOrdered(index); //The caller still holds the call: it reads the outcome afterwards
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

	//By the name it was added with (the method name), NULL when there's none
	ZBerryFunction GetFuncByName(string funcName) {
		foreach (ZBerryCategory category : m_oCategoryList) {
			ZBerryFunction funcData = category.GetByName(funcName);
			if (funcData) return funcData;
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
