class ZomberryStockFunctions {
	ref ZomberryCmdAPI m_ZomberryCmdAPI;
	autoptr TIntArray m_spectatingList = new TIntArray;
	protected ref map<int, string> m_PendingKeys = new map<int, string>; //Admin session id -> what the last first run would do
	protected ref map<int, int> m_PendingTimes = new map<int, int>; //... and when
	protected ref map<int, int> m_TPAllPending = new map<int, int>; //Admin session id -> game time of the first press
	protected ref TStringArray m_Frozen = new TStringArray; //Steam64 ids: frozen until an admin unfreezes them, also across relogs
	protected static ref map<string, ref TStringArray> m_PartsBySlot; //See GetPartsBySlot

	static const string FEATURE_FREEZE = "zomberry_freeze";

	void ZomberryStockFunctions() {
		m_ZomberryCmdAPI = GetZomberryCmdAPI();
		m_ZomberryCmdAPI.RegisterClientFeature(FEATURE_FREEZE, this, "OnFreezeFeature"); //Client side of freezing
	}

	void Init() {
		m_ZomberryCmdAPI.AddCategory("OnTarget", 0xFF42AAFF);
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Cursor", "TPCur", this, "OnTarget", false, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Teleport - Me 10 meters Forward", "TPForward", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Teleport - Me to Target", "TPToTarget", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Me", "TPToAdmin", this, "OnTarget", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Heal", "HealTarget", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Partial heal", "HealTargetPart", this, "OnTarget", false, {
			new ZBerryFuncParam("Heal diseases", {0, 1, 1,}, {"No", "Yes"}),
			new ZBerryFuncParam("Heal wounds", {0, 1, 1,}, {"No", "Yes"}),
			new ZBerryFuncParam("Feed", {0, 1, 1,}, {"No", "Yes"}),
		});
		m_ZomberryCmdAPI.AddCommand("Toggle god", "GodTarget", this, "OnTarget", false, NULL, true); //Can switch an admin's god mode off
		m_ZomberryCmdAPI.AddCommand("Spectate target", "SpectateTarget", this, "OnTarget", true);
		m_ZomberryCmdAPI.AddCommand("Toggle freeze", "FreezeTarget", this, "OnTarget", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Repair item in hands", "RepairTargetHands", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Refuel and repair", "RefuelAndRepair", this, "OnTarget", false);

		m_ZomberryCmdAPI.AddCategory("==", 0xFFFF7C75);
		m_ZomberryCmdAPI.AddCommand("Kill", "KillTarget", this, "==", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Bite", "BiteTarget", this, "==", true, {
			new ZBerryFuncParam("Bites", {1, 25, 1,}),
		}, true);
		m_ZomberryCmdAPI.AddCommand("Strip", "StripTarget", this, "==", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Induce vomiting", "RejectBellyTarget", this, "==", true, {
			new ZBerryFuncParam("Seconds", {5, 30, 5,}),
		}, true);
		m_ZomberryCmdAPI.AddCommand("Induce laughter", "PsycoTarget", this, "==", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Induce sneeze", "SneezeTarget", this, "==", true, NULL, true);
		m_ZomberryCmdAPI.AddCommand("Set health", "SetHealthTarget", this, "==", true, {
			new ZBerryFuncParam("Health", {1, 100, 100,}),
		}, true);
		m_ZomberryCmdAPI.AddCommand("Set blood", "SetBloodTarget", this, "==", true, {
			new ZBerryFuncParam("Blood", {1, 5000, 5000,}),
		}, true);

		m_ZomberryCmdAPI.AddCategory("OnServer", 0xFF909090);
		m_ZomberryCmdAPI.AddCommand("FreeCam", "FreeCamAdm", this, "OnServer", false);
		m_ZomberryCmdAPI.AddCommand("Time - Day", "TimeDay", this, "OnServer", false, NULL, false, true);
		m_ZomberryCmdAPI.AddCommand("Time - Night", "TimeNight", this, "OnServer", false, NULL, false, true);
		m_ZomberryCmdAPI.AddCommand("Set time", "SetTime", this, "OnServer", false, {
			new ZBerryFuncParam("Hour", {0, 23, 12,}),
			new ZBerryFuncParam("Minute", {0, 59, 0,}),
		}, false, true);
		m_ZomberryCmdAPI.AddCommand("Set date", "SetDate", this, "OnServer", false, {
			new ZBerryFuncParam("Day", {1, 30, 1,}),
			new ZBerryFuncParam("Month", {1, 12, 1,}, {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}),
			new ZBerryFuncParam("Year", {1970, 2119, 2019,}),
		}, false, true);
		m_ZomberryCmdAPI.AddCommand("Set weather", "SetWeather", this, "OnServer", false, {
			new ZBerryFuncParam("Fog", {0, 100, 0,}),
			new ZBerryFuncParam("Overcast", {0, 100, 0,}),
			new ZBerryFuncParam("Rain", {0, 100, 0,}),
		}, false, true);
		m_ZomberryCmdAPI.AddCommand("Delete object near cursor", "DeleteObj", this, "OnServer", false, NULL, false, true);
		m_ZomberryCmdAPI.AddCommand("Clean up around cursor", "CleanupArea", this, "OnServer", false, {
			new ZBerryFuncParam("Radius (m)", {5, 200, 50,}),
			new ZBerryFuncParam("What", {0, 3, 0,}, ZBerryWorldTools.GetCleanNames()),
		}, false, true);
		m_ZomberryCmdAPI.AddCommand("Flip vehicle near cursor", "FlipVehicle", this, "OnServer", false, NULL, false, true);
		m_ZomberryCmdAPI.AddCommand("Teleport all players to me", "TPAllToAdmin", this, "OnServer", false, NULL, true, true);
		m_ZomberryCmdAPI.AddCommand("ESP", "ESPToggle", this, "OnServer", false); //No parameters, so it can be keybound
	}

	static void MessagePlayer(PlayerBase player, string msg) {
		Param1<string> param = new Param1<string>( "[ZomBerry]: " + msg );
		if (GetGame().IsMultiplayer()) {
			if (!player || !player.GetIdentity()) return; //Admin disconnected meanwhile

			GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, param, true, player.GetIdentity());
		} else {
			GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "[ZomBerry]", msg, ""));
		}
	}

	//Tells the admin why nothing happened, and has the call logged as refused instead of as run
	protected void Refuse(PlayerBase admin, string message) {
		MessagePlayer(admin, message);
		m_ZomberryCmdAPI.RefuseCall(message);
	}

	//The first of two runs: the call isn't logged, the confirmed one is
	protected void AskToConfirm(PlayerBase admin, string message) {
		MessagePlayer(admin, message);
		m_ZomberryCmdAPI.AwaitConfirmation();
	}

	//"{actor} <what> for {target}", or "for themselves" when the admin selected themselves
	protected void DescribeOnTarget(int adminId, int targetId, string what) {
		if (adminId == targetId) {
			m_ZomberryCmdAPI.DescribeCall("{actor} " + what + " for themselves");
		} else {
			m_ZomberryCmdAPI.DescribeCall("{actor} " + what + " for {target}");
		}
	}

	//Looks both players up; returns false when the admin has no character (dead, respawning),
	//tells the admin and returns false when the target is gone
	protected bool GetTargets(int adminId, int targetId, out PlayerBase admin, out PlayerBase target) {
		admin = ZBGetPlayerById(adminId);
		target = ZBGetPlayerById(targetId);
		if (!admin) {
			ZomberryBase.Log( "ZomBerryFnc", "WARN: " + ZBDescribePlayer(ZBGetIdentityById(adminId)) + " has no character, function skipped" );
			m_ZomberryCmdAPI.RefuseCall("No character (dead or respawning)");
			return false;
		}
		if (target) return true;

		Refuse(admin, "Target not found (probably disconnected?)");
		return false;
	}

	static vector GetPosSafe(PlayerBase target) {
		if (!target.GetCommand_Vehicle()) {
			return target.GetPosition();
		} else {
			return target.GetCommand_Vehicle().GetTransport().GetPosition();
		}
	}

	static bool SetPosSafe(PlayerBase target, vector position) {
		if (!target.GetCommand_Vehicle()) {
			target.SetPosition(position);
			return true;
		}
		return false;
	}

	//Players have no identity in singleplayer
	static string GetPlayerName(PlayerBase player) {
		if (!player || !player.GetIdentity()) return "Player";

		return player.GetIdentity().GetName();
	}

	void TPForward( string funcName, int adminId, int targetId, vector cursor ) {
		const int tpDist = 10;
		vector currentPos, curToTgtDir, targetPos;
		PlayerBase admin = ZBGetPlayerById(adminId);
		if (!admin) return; //No character (dead, respawning)

		currentPos = GetPosSafe(admin);
		currentPos[1] = 0;
		cursor[1] = 0;

		curToTgtDir = vector.Direction(currentPos, cursor);
		curToTgtDir.Normalize();

		targetPos[0] = curToTgtDir[0]*tpDist;
		targetPos[2] = curToTgtDir[2]*tpDist;
		targetPos = targetPos + currentPos;

		targetPos[1] = GetGame().SurfaceY(targetPos[0], targetPos[2]);

		if (!SetPosSafe(admin, targetPos)) {
			Refuse(admin, "Unable to teleport: admin in vehicle");
		}
	}

	void TPCur( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(target, cursor)) {
			MessagePlayer(admin, "Target teleported");
		} else {
			Refuse(admin, "Unable to teleport: target in vehicle");
		}
	}

	void TPToTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(admin, GetPosSafe(target))) {
			MessagePlayer(admin, "Teleported to target");
		} else {
			Refuse(admin, "Unable to teleport: get out of vehicle!");
		}
	}

	void TPToAdmin( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(target, GetPosSafe(admin))) {
			MessagePlayer(admin, "Target teleported");
		} else {
			Refuse(admin, "Unable to teleport: target in vehicle");
		}
	}

	void HealTarget( string funcName, int adminId, int targetId, vector cursor ) {
		HealTargetPart(funcName, adminId, targetId, cursor, {1, 1, 1});
	}

	void HealTargetPart( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		MessagePlayer(admin, GetPlayerName(target) + " has been " + HealPlayer(target, fValues[0] != 0, fValues[1] != 0, fValues[2] != 0));
	}

	//Returns what was done, for the admin message
	protected string HealPlayer(PlayerBase target, bool cure, bool heal, bool feed) {
		string result_message = "";

		if (cure) {
			bool is_area_exposure, is_mask;

			target.GetStomach().ClearContents();
			
			if (target.GetModifiersManager())
			{
				is_mask = target.GetModifiersManager().IsModifierActive(eModifiers.MDF_MASK);
				is_area_exposure = target.GetModifiersManager().IsModifierActive(eModifiers.MDF_AREAEXPOSURE);
				target.GetModifiersManager().DeactivateAllModifiers();
			}

			if (is_area_exposure)
				target.GetModifiersManager().ActivateModifier(eModifiers.MDF_AREAEXPOSURE);
			if (is_mask)
				target.GetModifiersManager().ActivateModifier(eModifiers.MDF_MASK);

			target.RemoveAllAgents();
			
			result_message += "cured; ";
		}

		if (heal) {
			if (target.GetBleedingManagerServer())
				target.GetBleedingManagerServer().RemoveAllSources();

			DamageZoneMap zones = new DamageZoneMap;
			DamageSystem.GetDamageZoneMap(target, zones);
			target.SetHealth("", "Health", target.GetMaxHealth("","Health"));
			target.SetHealth("", "Shock", target.GetMaxHealth("","Shock"));
			target.SetHealth("", "Blood", target.GetMaxHealth("","Blood"));
			
			for (int i = 0; i < zones.Count(); i++)
			{
				string zone = zones.GetKey(i);
				target.SetHealth(zone, "Health", target.GetMaxHealth(zone,"Health"));
				target.SetHealth(zone, "Shock", target.GetMaxHealth(zone,"Shock"));
				target.SetHealth(zone, "Blood", target.GetMaxHealth(zone,"Blood"));
			}

			if(target.IsUnconscious())
				DayZPlayerSyncJunctures.SendPlayerUnconsciousness(target, false);
			
			result_message += "healed; ";
		}

		if (feed) {
			target.GetStatStamina().Set(target.GetStatStamina().GetMax());
			target.GetStatWater().Set(target.GetStatWater().GetMax());
			target.GetStatEnergy().Set(target.GetStatEnergy().GetMax());
			
			result_message += "rehydrated and fed";
		}

		PluginLifespan module_lifespan = PluginLifespan.Cast(GetPlugin(PluginLifespan));
		module_lifespan.UpdateBloodyHandsVisibilityEx(target, eBloodyHandsTypes.CLEAN);

		return result_message;
	}

	void GodTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (target.ZBIsGod()) {
			target.ZBSetGod(false);
			MessagePlayer(target, "God mode deactivated");
			if (adminId != targetId) MessagePlayer(admin, "God mode deactivated for " + GetPlayerName(target));
			DescribeOnTarget(adminId, targetId, "switched god mode off");

		} else {
			HealPlayer(target, true, true, true);

			target.ZBSetGod(true);
			MessagePlayer(target, "God mode activated");
			if (adminId != targetId) MessagePlayer(admin, "God mode activated for " + GetPlayerName(target));
			DescribeOnTarget(adminId, targetId, "switched god mode on");
		}
	}

	void RepairTargetHands( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		InventoryItem target_item = target.GetItemInHands();

		if (!target_item) {
			Refuse(admin, "No item in target's hands");
			return;
		}

		GameInventory target_item_inv = target_item.GetInventory();
		ref array<EntityAI> sub_items = new array<EntityAI>;

		if (target_item_inv)
			target_item_inv.EnumerateInventory(InventoryTraversalType.INORDER, sub_items);

		target_item.SetHealthMax("","");
		for (int i = 0; i < sub_items.Count(); i++)
		{
			EntityAI item = sub_items.Get(i);
			item.SetHealthMax("","");
		}

		MessagePlayer(admin, "Repaired item to the best condition it can achieve");
	}

	void RefuelAndRepair( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		ref array<Object> nearestObjects = new array<Object>;
		ref array<CargoBase> proxyCargos = new array<CargoBase>;

		vector position = GetPosSafe(target);
		GetGame().GetObjectsAtPosition(position, 15, nearestObjects, proxyCargos);

		foreach(Object nearObject: nearestObjects) {
			if (nearObject.IsKindOf("CarScript")) RepairCar(Car.Cast(nearObject));
		}
		MessagePlayer(admin, "Refueled and repaired vehicles near to target");
	}

	protected void RepairCar(Car carObject) {
		TStringArray slotsCar = new TStringArray;
		map<string, ref TStringArray> partsBySlot = GetPartsBySlot();
		TStringArray slotParts;
		ItemBase currentAttachment;
		int m, p;

		ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: Fixing " + carObject.GetType());

		// Get list of available attachment slots from config
		GetGame().ConfigGetTextArray("CfgVehicles " + carObject.GetType() + " attachments", slotsCar);

		// For each slot of this car, in the same order as the parts appear in config
		for (m = 0; m < slotsCar.Count(); m++) {
			if (!partsBySlot.Find(slotsCar[m], slotParts)) continue;

			for (p = 0; p < slotParts.Count(); p++) {

				// Cast attachment that's currently in this slot to ItemBase
				Class.CastTo(currentAttachment, carObject.FindAttachmentBySlotName(slotsCar[m]));

				// If attachment is present
				if (currentAttachment) {

					// Skip if health is good
					if (currentAttachment.GetHealth01() > 0.75) continue;

					// Destroy if health is *not so good*
					ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: " + currentAttachment.GetType() + " (" + currentAttachment.GetHealth().ToString() + " HP) - detached");
					carObject.GetInventory().DropEntity(InventoryMode.PREDICTIVE, carObject, currentAttachment);
					currentAttachment.SetPosition("0 0 0");
					GetGame().ObjectDelete(currentAttachment);
				}

				// Add new attachment to vehicle
				carObject.GetInventory().CreateAttachmentEx(slotParts[p], InventorySlots.GetSlotIdFromString(slotsCar[m]));
				ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: Found and attached " + slotParts[p]);
			}
		}

		carObject.SetHealth("Engine", "", carObject.GetMaxHealth("Engine", ""));

		// Refuel
		float fuelReq = carObject.GetFluidCapacity( CarFluid.FUEL ) - (carObject.GetFluidCapacity( CarFluid.FUEL ) * carObject.GetFluidFraction( CarFluid.FUEL ));
		float oilReq = carObject.GetFluidCapacity( CarFluid.OIL ) - (carObject.GetFluidCapacity( CarFluid.OIL ) * carObject.GetFluidFraction( CarFluid.OIL ));
		float coolantReq = carObject.GetFluidCapacity( CarFluid.COOLANT ) - (carObject.GetFluidCapacity( CarFluid.COOLANT ) * carObject.GetFluidFraction( CarFluid.COOLANT ));
		float brakeReq = carObject.GetFluidCapacity( CarFluid.BRAKE ) - (carObject.GetFluidCapacity( CarFluid.BRAKE ) * carObject.GetFluidFraction( CarFluid.BRAKE ));
		carObject.Fill( CarFluid.FUEL, fuelReq );
		carObject.Fill( CarFluid.OIL, oilReq );
		carObject.Fill( CarFluid.COOLANT, coolantReq );
		carObject.Fill( CarFluid.BRAKE, brakeReq );
	}

	//Every non-ruined Inventory_Base item by the slot it fits, in config order. Built on first use:
	//walking the whole CfgVehicles for each repaired car took a noticeable while
	protected static map<string, ref TStringArray> GetPartsBySlot() {
		if (m_PartsBySlot) return m_PartsBySlot;

		m_PartsBySlot = new map<string, ref TStringArray>;
		TStringArray itemSlots = new TStringArray;
		string cfgVehItem, cfgVehItemLower;
		int cfgVehiclesCount = GetGame().ConfigGetChildrenCount("CfgVehicles");

		for (int cfgVehId = 0; cfgVehId < cfgVehiclesCount; cfgVehId++) {
			GetGame().ConfigGetChildName("CfgVehicles", cfgVehId, cfgVehItem);

			cfgVehItemLower = cfgVehItem;
			cfgVehItemLower.ToLower();

			// We don't need ruined, and unfortunately the most common root for all car parts is Inventory_Base
			if (!GetGame().IsKindOf(cfgVehItemLower, "Inventory_Base") || cfgVehItemLower.Contains("ruined")) continue;

			// Get list of slots this item can fit into
			itemSlots.Clear();
			if (GetGame().ConfigGetType("CfgVehicles " + cfgVehItem + " inventorySlot") == CT_ARRAY) {
				GetGame().ConfigGetTextArray("CfgVehicles " + cfgVehItem + " inventorySlot", itemSlots);
			} else {
				string singleSlot;
				GetGame().ConfigGetText("CfgVehicles " + cfgVehItem + " inventorySlot", singleSlot);
				itemSlots.Insert(singleSlot);
			}

			foreach (string slot : itemSlots) {
				if (!m_PartsBySlot.Contains(slot)) m_PartsBySlot.Insert(slot, new TStringArray);
				m_PartsBySlot.Get(slot).Insert(cfgVehItem);
			}
		}

		return m_PartsBySlot;
	}

	void KillTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		target.SetHealth(0);
		MessagePlayer(admin, "Killed target");
	}

	void BiteTarget( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		BleedingSourcesManagerServer BSMgr = target.GetBleedingManagerServer();

		for (int i = 0; i < fValues[0]; ++i) {
			for (int j = 0; j < 15; ++j) {
				if (BSMgr.AttemptAddBleedingSource(Math.RandomInt(0, 100))) break;
			}
		}
		MessagePlayer(admin, "Target was bitten " + fValues[0] + " times and now losing blood");
	}

	void StripTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		target.RemoveAllItems();
		MessagePlayer(admin, "Target was stripped");
	}

	void RejectBellyTarget( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (!target.GetCommand_Vehicle()) {
			SymptomBase symptom = target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_VOMIT);
			if (!symptom) {
				Refuse(admin, "Target can't vomit right now (already sick?)");
				return;
			}

			symptom.SetDuration(fValues[0]);
			MessagePlayer(admin, "Target is gonna be sick for " + fValues[0] + " seconds");
		} else {
			Refuse(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void PsycoTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_LAUGHTER);
			MessagePlayer(admin, "Target is laughing...");
		} else {
			Refuse(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void SneezeTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_SNEEZE);
			MessagePlayer(admin, "Target sneezed");
		} else {
			Refuse(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void SetHealthTarget( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		target.SetHealth(fValues[0]);

		MessagePlayer(admin, "Target health level was set to " + fValues[0]);
	}

	void SetBloodTarget( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		target.SetHealth("", "Blood", fValues[0]);

		MessagePlayer(admin, "Target blood level was set to " + fValues[0]);
	}

	//The admin's body stays where it is and doesn't react to input while they're in FreeCam or spectating
	static void FreezeBody(PlayerBase body, bool frozen) {
		HumanInputController input = body.GetInputController();
		if (!input) return;

		input.OverrideMovementSpeed(frozen, 0);
		input.OverrideRaise(frozen, false);
		input.OverrideAimChangeX(frozen, 0);
		input.OverrideAimChangeY(frozen, 0);
	}

	//The selected player can't move, turn or raise their hands until unfrozen: on the server (their body's input)
	//and on their client (FEATURE_FREEZE), which drives their movement. Survives relogs, see ReapplyFreeze
	void FreezeTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;
		if (target == admin) {
			Refuse(admin, "Select another player to freeze");
			return;
		}

		string steamId = GetSteamId(target);
		bool freeze = m_Frozen.Find(steamId) == -1;
		SetFrozen(target, freeze);

		if (freeze) {
			MessagePlayer(admin, GetPlayerName(target) + " is frozen, run Toggle freeze again to release them");
			m_ZomberryCmdAPI.DescribeCall("{actor} froze {target}");
		} else {
			MessagePlayer(admin, GetPlayerName(target) + " is released");
			m_ZomberryCmdAPI.DescribeCall("{actor} released {target}");
		}
	}

	protected void SetFrozen(PlayerBase target, bool freeze) {
		string steamId = GetSteamId(target);
		m_Frozen.RemoveItem(steamId);
		if (freeze) m_Frozen.Insert(steamId);

		FreezeBody(target, freeze);
		m_ZomberryCmdAPI.SetClientFeature(target.GetIdentity(), FEATURE_FREEZE, freeze);
		if (freeze) {
			MessagePlayer(target, "An admin froze you, please wait");
		} else {
			MessagePlayer(target, "An admin released you");
		}
	}

	protected static string GetSteamId(PlayerBase player) {
		if (!player.GetIdentity()) return "";
		return player.GetIdentity().GetPlainId();
	}

	//A frozen player who relogged: freeze them again (their client lost the feature with the old session)
	void ReapplyFreeze(PlayerBase player) {
		if (!player || m_Frozen.Find(GetSteamId(player)) == -1) return;

		SetFrozen(player, true);
	}

	//Client side of FreezeTarget
	void OnFreezeFeature(bool enabled, TIntArray args) {
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (player) FreezeBody(player, enabled);
	}

	//Through the selected player's eyes (or over their shoulder while they're in third person). Run it again to stop
	void SpectateTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		if (!admin) return;

		ZBerrySpectate spectate = GetZBerrySpectate();
		if (spectate.IsWatching(adminId)) {
			spectate.Stop(adminId, "Returned to player body");
			m_ZomberryCmdAPI.DescribeCall("{actor} stopped spectating");
			return;
		}
		if (m_spectatingList.Find(adminId) != -1) {
			Refuse(admin, "Leave FreeCam first");
			return;
		}

		PlayerBase target = ZBGetPlayerById(targetId);
		string problem;
		if (!spectate.Start(admin, target, ZBerryPermissions.FunctionNode(funcName), problem)) {
			Refuse(admin, "Can't spectate: " + problem);
			return;
		}
		MessagePlayer(admin, "Spectating " + GetPlayerName(target) + ", run Spectate target again to return");
		m_ZomberryCmdAPI.DescribeCall("{actor} started spectating {target}");
	}

	void FreeCamAdm( string funcName, int adminId, int targetId, vector cursor ) {
		int listId = m_spectatingList.Find(adminId);
		PlayerBase adminPly = ZBGetPlayerById(adminId);
		if (!adminPly) { //No character (dead, respawning)
			if (listId != -1) m_spectatingList.Remove(listId);
			return;
		}
		if (GetZBerrySpectate().IsWatching(adminId)) {
			Refuse(adminPly, "Stop spectating first");
			return;
		}

		PlayerIdentity adminIdent = adminPly.GetIdentity();

		if (listId != -1) {
			FreezeBody(adminPly, false);
			m_spectatingList.Remove(listId);

			GetGame().SelectPlayer(adminIdent, adminPly);
			MessagePlayer(ZBGetPlayerById(adminId), "Returned to player body");
			m_ZomberryCmdAPI.DescribeCall("{actor} left FreeCam");
		} else {
			FreezeBody(adminPly, true);
			m_spectatingList.Insert(adminId);

			vector facing = adminPly.GetDirection();
			facing[1] = 0;
			facing.Normalize();
			GetGame().SelectSpectator(adminIdent, "ZomBerryCamFree", adminPly.GetPosition() - facing * 1.2 + Vector(0, 2, 0)); //Behind the head, not inside it
			MessagePlayer(ZBGetPlayerById(adminId), "Entered FreeCam, use MWheel to change camera speed");
			m_ZomberryCmdAPI.DescribeCall("{actor} entered FreeCam");
		}
	}

	void SetTime( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		GetGame().GetWorld().SetDate(year, month, day, fValues[0], fValues[1]);
		MessagePlayer(ZBGetPlayerById(adminId), "Time set to: " + fValues[0].ToString() + ":" + fValues[1].ToString() + " (it may take some time to effect!)");
	}

	void SetDate( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		GetGame().GetWorld().SetDate(fValues[2], fValues[1], fValues[0], hour, minute);
		MessagePlayer(ZBGetPlayerById(adminId), "Date set to: " + fValues[0].ToString() + "." + fValues[1].ToString() + "." + fValues[2].ToString() + " (it may take some time to effect!)");
	}

	void TimeDay( string funcName, int adminId, int targetId, vector cursor ) {
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		GetGame().GetWorld().SetDate(year, month, day, 12, 0);
		MessagePlayer(ZBGetPlayerById(adminId), "Time set to: 12:00 (it may take some time to effect!)");
	}

	void TimeNight( string funcName, int adminId, int targetId, vector cursor ) {
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		GetGame().GetWorld().SetDate(year, month, day, 23, 0);
		MessagePlayer(ZBGetPlayerById(adminId), "Time set to: 23:00 (it may take some time to effect!)");
	}

	void SetWeather( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		ApplyWeather(fValues[0], fValues[1], fValues[2]);
		int holdMinutes = WEATHER_HOLD / 60;
		MessagePlayer(ZBGetPlayerById(adminId), "Weather changed, it holds for " + holdMinutes + " minutes");
	}

	static const float WEATHER_HOLD = 360; //Seconds the admin's weather lasts before the server's own weather takes over

	//Fog, overcast and rain in percent. Rain only starts above the world's overcast threshold (0.6 on Chernarus), and
	//below it the world stops the rain itself, over a minute. So overcast and rain change at once, in that order when
	//clouds come in and the other way round when they clear
	static void ApplyWeather(int fogPercent, int overcastPercent, int rainPercent) {
		Weather WMgr = GetGame().GetWeather();
		float overcast = overcastPercent / 100.0;
		bool cloudsComeIn = overcast >= WMgr.GetOvercast().GetActual();

		if (cloudsComeIn) SetWeatherPhenomenon(WMgr.GetOvercast(), overcast, 0);
		SetWeatherPhenomenon(WMgr.GetRain(), rainPercent / 100.0, 0);
		if (!cloudsComeIn) SetWeatherPhenomenon(WMgr.GetOvercast(), overcast, 0);
		SetWeatherPhenomenon(WMgr.GetFog(), fogPercent / 100.0, 1);
		WMgr.SetStorm(GetStormDensity(overcastPercent, rainPercent), 0.7, GetStormTimeout(rainPercent));
	}

	//0-1: overcast and rain (0-100 each) together make a storm. Typed float locals: int maths gave 0 below 100/100
	static float GetStormDensity(int overcastPercent, int rainPercent) {
		float overcast = overcastPercent;
		float rain = rainPercent;
		return Math.Clamp((overcast / 100.0) * (rain / 100.0), 0, 1);
	}

	//Seconds between lightning strikes: shorter the heavier the rain
	static float GetStormTimeout(int rainPercent) {
		float rain = rainPercent;
		return 3600 / (rain / 2.0 + 0.1);
	}

	//Sets a weather phenomenon (reached in changeTime seconds, kept for WEATHER_HOLD), then restores the server's own
	//limits so its weather settings keep working. No SetNextChange: an early next change let the world's weather logic
	//replace the value straight away (a storm's rain came out at its own forecast, clear weather kept raining for a minute)
	protected static void SetWeatherPhenomenon(WeatherPhenomenon phenomenon, float value, float changeTime) {
		float fcMin, fcMax, fnMin, fnMax, ftMin, ftMax;
		phenomenon.GetForecastChangeLimits(fcMin, fcMax);
		phenomenon.GetLimits(fnMin, fnMax);
		phenomenon.GetForecastTimeLimits(ftMin, ftMax);

		phenomenon.SetForecastChangeLimits(0, 1);
		phenomenon.SetLimits(0, 1);
		phenomenon.SetForecastTimeLimits(0, 3600);

		phenomenon.Set(value, changeTime, WEATHER_HOLD);

		phenomenon.SetForecastChangeLimits(fcMin, fcMax);
		phenomenon.SetLimits(fnMin, fnMax);
		phenomenon.SetForecastTimeLimits(ftMin, ftMax);
	}

	//Switches the ESP overlay on the admin's own client on or off (ZBerryESPOverlay). What it draws is set on that
	//client's ESP tab; while it's on, the server streams everyone's position and health to that admin if those
	//settings use them (ZBerryStreamServer.WantsEsp)
	void ESPToggle( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		if (!admin) return;

		PlayerIdentity identity = admin.GetIdentity();
		bool on = !GetZomberryCmdAPI().IsClientFeatureActive(identity, ZBerryESPOverlay.FEATURE);
		GetZomberryCmdAPI().SetClientFeature(identity, ZBerryESPOverlay.FEATURE, on, NULL, ZBerryPermissions.FunctionNode(funcName));
		ZBerryStreamServer streams = ZBerryStreamServer.Get();
		if (streams) streams.SetEspActive(identity, on);

		if (on) {
			MessagePlayer(admin, "ESP on");
			m_ZomberryCmdAPI.DescribeCall("{actor} switched ESP on");
		} else {
			MessagePlayer(admin, "ESP off");
			m_ZomberryCmdAPI.DescribeCall("{actor} switched ESP off");
		}
	}

	static const int TP_ALL_CONFIRM_MS = 10000;
	static const float TP_ALL_SPACING = 2; //Metres between players on the ring

	//Run twice within TP_ALL_CONFIRM_MS: it moves everyone at once. Players go on a ring around the admin, so they don't
	//end up inside each other. Skips players in vehicles and admins whose group is protected from this admin's rank
	void TPAllToAdmin( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		if (!admin) return;

		int now = GetGame().GetTime();
		int firstPress;
		if (!m_TPAllPending.Find(adminId, firstPress) || now - firstPress > TP_ALL_CONFIRM_MS) {
			m_TPAllPending.Set(adminId, now);
			AskToConfirm(admin, "This teleports every player to you. Run it again within " + (TP_ALL_CONFIRM_MS / 1000) + " seconds to confirm.");
			return;
		}
		m_TPAllPending.Remove(adminId);

		array<PlayerBase> targets = new array<PlayerBase>;
		int inVehicles = 0;
		int protectedCount = 0;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || player == admin || !player.IsAlive()) continue;
			if (player.GetCommand_Vehicle()) {
				++inVehicles;
				continue;
			}
			if (admin.GetIdentity() && player.GetIdentity() && !GetZomberryPermAPI().CanTarget(admin.GetIdentity(), player.GetIdentity(), true)) {
				++protectedCount;
				continue;
			}
			targets.Insert(player);
		}

		vector center = GetPosSafe(admin);
		for (int i = 0; i < targets.Count(); ++i) {
			targets[i].SetPosition(GetRingPosition(center, i, targets.Count()));
		}

		string skipped = "";
		if (inVehicles > 0) skipped += ", " + inVehicles + " skipped (in vehicles)";
		if (protectedCount > 0) skipped += ", " + protectedCount + " skipped (protected from your rank)";
		MessagePlayer(admin, "Teleported " + targets.Count() + " players" + skipped);
		m_ZomberryCmdAPI.DescribeCall("{actor} teleported all players to " + ZBDescribePosition(center) + ": " + targets.Count() + " moved" + skipped);
	}

	//Evenly spread on a circle around center, TP_ALL_SPACING apart, at least 3 m out, on the surface
	static vector GetRingPosition(vector center, int index, int count) {
		float circumference = Math.Max(count, 1) * TP_ALL_SPACING;
		float radius = Math.Max(3, circumference / Math.PI2);
		float angle = Math.PI2 * index / Math.Max(count, 1);

		float x = center[0] + Math.Cos(angle) * radius;
		float z = center[2] + Math.Sin(angle) * radius;
		return Vector(x, GetGame().SurfaceY(x, z), z);
	}

	static const int CONFIRM_MS = 10000;

	//Destructive functions run twice: the first run says what would happen, the second one (with the same key
	//within CONFIRM_MS) does it. Returns true on the confirming run
	protected bool IsConfirmed(int adminId, string key) {
		string pendingKey;
		int pendingTime;
		int now = GetGame().GetTime();
		bool confirmed = m_PendingKeys.Find(adminId, pendingKey) && pendingKey == key && m_PendingTimes.Find(adminId, pendingTime) && now - pendingTime <= CONFIRM_MS;

		if (confirmed) {
			m_PendingKeys.Remove(adminId);
			m_PendingTimes.Remove(adminId);
		} else {
			m_PendingKeys.Set(adminId, key);
			m_PendingTimes.Set(adminId, now);
		}
		return confirmed;
	}

	//The object the cursor is on (by bounding box, within 1 m): items, vehicles, bodies, base parts, infected, animals.
	//Never living players or map objects. Confirmed by running it again on the same object
	void DeleteObj( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		EntityAI entity = ZBerryWorldTools.PickAt(cursor, 1.0);
		if (!entity) {
			Refuse(admin, "Nothing to delete under the cursor (map objects and living players can't be deleted)");
			return;
		}

		int low, high;
		entity.GetNetworkID(low, high);
		string description = entity.GetDisplayName() + " (" + entity.GetType() + ")";
		if (!IsConfirmed(adminId, "delete:" + low + ":" + high)) {
			AskToConfirm(admin, "Delete " + description + "? Run it again within " + (CONFIRM_MS / 1000) + " seconds on the same object to confirm.");
			return;
		}

		m_ZomberryCmdAPI.DescribeCall("{actor} deleted " + ZBerryLocText.Name(entity) + " (" + entity.GetType() + ") at " + ZBDescribePosition(entity.GetPosition()));
		GetGame().ObjectDelete(entity);
		MessagePlayer(admin, "Deleted " + description);
	}

	//Removes one kind of thing within a radius of the cursor: infected, dead bodies, loose items or ruined items.
	//Built and placed things (bases, tents, storage, stashes, fireplaces) are never touched. Confirmed by running it again
	void CleanupArea( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		float radius = Math.Clamp(fValues[0], 5, 200);
		int kind = Math.Clamp(fValues[1], 0, ZBerryWorldTools.GetCleanNames().Count() - 1);
		string kindName = ZBerryWorldTools.GetCleanNames()[kind];
		kindName.ToLower(); //In place

		array<EntityAI> found = ZBerryWorldTools.FindForCleanup(cursor, radius, kind);
		if (found.Count() == 0) {
			Refuse(admin, "No " + kindName + " within " + radius + " m");
			return;
		}

		//Same kind, radius and spot (to 10 m) as the first run
		float x = cursor[0];
		float z = cursor[2];
		int cellX = Math.Floor(x / 10);
		int cellZ = Math.Floor(z / 10);
		if (!IsConfirmed(adminId, "clean:" + kind + ":" + radius + ":" + cellX + ":" + cellZ)) {
			AskToConfirm(admin, "Found " + found.Count() + " " + kindName + " within " + radius + " m. Run it again within " + (CONFIRM_MS / 1000) + " seconds to delete them.");
			return;
		}

		foreach (EntityAI entity : found) {
			if (entity) GetGame().ObjectDelete(entity);
		}
		m_ZomberryCmdAPI.DescribeCall("{actor} cleaned up " + found.Count() + " " + kindName + " within " + radius + " m of " + ZBDescribePosition(cursor));
		MessagePlayer(admin, "Deleted " + found.Count() + " " + kindName);
	}

	//The car or boat nearest to the cursor (within 5 m of its body), back on its wheels
	void FlipVehicle( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		Transport vehicle = Transport.Cast(ZBerryWorldTools.PickAt(cursor, 5, true));
		if (!vehicle) {
			Refuse(admin, "No vehicle near the cursor");
			return;
		}

		string problem = ZBerryWorldTools.FlipVehicle(vehicle);
		if (problem != "") {
			Refuse(admin, "Can't flip " + vehicle.GetDisplayName() + ": " + problem);
			return;
		}

		m_ZomberryCmdAPI.DescribeCall("{actor} flipped " + ZBerryLocText.Name(vehicle) + " at " + ZBDescribePosition(vehicle.GetPosition()));
		MessagePlayer(admin, vehicle.GetDisplayName() + " is back on its wheels");
	}
};
