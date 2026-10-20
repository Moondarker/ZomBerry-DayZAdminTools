class ZomberryStockFunctions {
	ref ZomberryCmdAPI m_ZomberryCmdAPI;
	autoptr TIntArray m_spectatingList = new TIntArray;
	autoptr TIntStringMap m_deleteList = new TIntStringMap;
	protected ref map<int, int> m_TPAllPending = new map<int, int>; //Admin session id -> game time of the first press
	protected static ref map<string, ref TStringArray> m_PartsBySlot; //See GetPartsBySlot

	void ZomberryStockFunctions() {
		m_ZomberryCmdAPI = GetZomberryCmdAPI();
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
		m_ZomberryCmdAPI.AddCommand("Time - Day", "TimeDay", this, "OnServer", false);
		m_ZomberryCmdAPI.AddCommand("Time - Night", "TimeNight", this, "OnServer", false);
		m_ZomberryCmdAPI.AddCommand("Set time", "SetTime", this, "OnServer", false, {
			new ZBerryFuncParam("Hour", {0, 23, 12,}),
			new ZBerryFuncParam("Minute", {0, 59, 0,}),
		});
		m_ZomberryCmdAPI.AddCommand("Set date", "SetDate", this, "OnServer", false, {
			new ZBerryFuncParam("Day", {1, 30, 1,}),
			new ZBerryFuncParam("Month", {1, 12, 1,}, {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}),
			new ZBerryFuncParam("Year", {1970, 2119, 2019,}),
		});
		m_ZomberryCmdAPI.AddCommand("Set weather", "SetWeather", this, "OnServer", false, {
			new ZBerryFuncParam("Fog", {0, 100, 0,}),
			new ZBerryFuncParam("Overcast", {0, 100, 0,}),
			new ZBerryFuncParam("Rain", {0, 100, 0,}),
		});
		m_ZomberryCmdAPI.AddCommand("Delete object near cursor", "DeleteObj", this, "OnServer", false);
		m_ZomberryCmdAPI.AddCommand("Teleport all players to me", "TPAllToAdmin", this, "OnServer", false, NULL, true);
		m_ZomberryCmdAPI.AddCommand("ESP", "ESPToggle", this, "OnServer", false, {
			new ZBerryFuncParam("Players", {0, 1, 1,}, {"Off", "On"}),
			new ZBerryFuncParam("Vehicles", {0, 1, 0,}, {"Off", "On"}),
			new ZBerryFuncParam("Infected, animals", {0, 1, 0,}, {"Off", "On"}),
		});
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

	//Looks both players up; returns false when the admin has no character (dead, respawning),
	//tells the admin and returns false when the target is gone
	protected bool GetTargets(int adminId, int targetId, out PlayerBase admin, out PlayerBase target) {
		admin = ZBGetPlayerById(adminId);
		target = ZBGetPlayerById(targetId);
		if (!admin) {
			ZomberryBase.Log( "ZomBerryFnc", "WARN: Admin #" + adminId + " has no character, function skipped" );
			return false;
		}
		if (target) return true;

		MessagePlayer(admin, "Target not found (probably disconnected?)");
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
			MessagePlayer(admin, "Unable to teleport: admin in vehicle");
		}
	}

	void TPCur( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(target, cursor)) {
			MessagePlayer(admin, "Target teleported");
		} else {
			MessagePlayer(admin, "Unable to teleport: target in vehicle");
		}
	}

	void TPToTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(admin, GetPosSafe(target))) {
			MessagePlayer(admin, "Teleported to target");
		} else {
			MessagePlayer(admin, "Unable to teleport: get out of vehicle!");
		}
	}

	void TPToAdmin( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (SetPosSafe(target, GetPosSafe(admin))) {
			MessagePlayer(admin, "Target teleported");
		} else {
			MessagePlayer(admin, "Unable to teleport: target in vehicle");
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

		} else {
			HealPlayer(target, true, true, true);

			target.ZBSetGod(true);
			MessagePlayer(target, "God mode activated");
			if (adminId != targetId) MessagePlayer(admin, "God mode activated for " + GetPlayerName(target));
		}
	}

	void RepairTargetHands( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		InventoryItem target_item = target.GetItemInHands();

		if (!target_item) {
			MessagePlayer(admin, "No item in target's hands");
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
				MessagePlayer(admin, "Target can't vomit right now (already sick?)");
				return;
			}

			symptom.SetDuration(fValues[0]);
			MessagePlayer(admin, "Target is gonna be sick for " + fValues[0] + " seconds");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void PsycoTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_LAUGHTER);
			MessagePlayer(admin, "Target is laughing...");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void SneezeTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase admin, target;
		if (!GetTargets(adminId, targetId, admin, target)) return;

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_SNEEZE);
			MessagePlayer(admin, "Target sneezed");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
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

	void FreeCamAdm( string funcName, int adminId, int targetId, vector cursor ) {
		int listId = m_spectatingList.Find(adminId);
		PlayerBase adminPly = ZBGetPlayerById(adminId);
		if (!adminPly) { //No character (dead, respawning)
			if (listId != -1) m_spectatingList.Remove(listId);
			return;
		}

		PlayerIdentity adminIdent = adminPly.GetIdentity();
		HumanInputController adminInput = adminPly.GetInputController();

		if (listId != -1) {
			adminInput.OverrideMovementSpeed(false, 0);
			adminInput.OverrideRaise(false, false);
			adminInput.OverrideAimChangeX(false, 0);
			adminInput.OverrideAimChangeY(false, 0);

			m_spectatingList.Remove(listId);

			GetGame().SelectPlayer(adminIdent, adminPly);
			MessagePlayer(ZBGetPlayerById(adminId), "Returned to player body");
		} else {
			adminInput.OverrideMovementSpeed(true, 0);
			adminInput.OverrideRaise(true, false);
			adminInput.OverrideAimChangeX(true, 0);
			adminInput.OverrideAimChangeY(true, 0);

			m_spectatingList.Insert(adminId);

			GetGame().SelectSpectator(adminIdent, "ZomBerryCamFree", (adminPly.GetPosition() + Vector(0,1.75,0)));
			MessagePlayer(ZBGetPlayerById(adminId), "Entered FreeCam, use MWheel to change camera speed");
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
		Weather WMgr = GetGame().GetWeather();

		SetWeatherPhenomenon(WMgr.GetFog(), fValues[0] / 100.0);
		SetWeatherPhenomenon(WMgr.GetOvercast(), fValues[1] / 100.0);
		SetWeatherPhenomenon(WMgr.GetRain(), fValues[2] / 100.0);
		WMgr.SetStorm(GetStormDensity(fValues[1], fValues[2]), 0.7, GetStormTimeout(fValues[2]));

		MessagePlayer(ZBGetPlayerById(adminId), "Weather changed (due to sync problems you might need to relog)");
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

	//Sets a weather phenomenon right away, then restores the server's own limits so its weather settings keep working
	protected void SetWeatherPhenomenon(WeatherPhenomenon phenomenon, float value) {
		float fcMin, fcMax, fnMin, fnMax, ftMin, ftMax;
		phenomenon.GetForecastChangeLimits(fcMin, fcMax);
		phenomenon.GetLimits(fnMin, fnMax);
		phenomenon.GetForecastTimeLimits(ftMin, ftMax);

		phenomenon.SetForecastChangeLimits(0, 1);
		phenomenon.SetLimits(0, 1);
		phenomenon.SetForecastTimeLimits(0, 3600);

		phenomenon.Set(value, 1, 360);
		phenomenon.SetNextChange(0.01);

		phenomenon.SetForecastChangeLimits(fcMin, fcMax);
		phenomenon.SetLimits(fnMin, fnMax);
		phenomenon.SetForecastTimeLimits(ftMin, ftMax);
	}

	//Switches the ESP overlay on the admin's own client (ZBerryESPOverlay). Everything off switches it off.
	//With players on, the server also streams everyone's position and health to that admin (ZBerryStreamServer)
	void ESPToggle( string funcName, int adminId, int targetId, vector cursor, TIntArray fValues ) {
		PlayerBase admin = ZBGetPlayerById(adminId);
		if (!admin) return;

		bool on = fValues[0] != 0 || fValues[1] != 0 || fValues[2] != 0;
		TIntArray args = {fValues[0], fValues[1], fValues[2]};
		GetZomberryCmdAPI().SetClientFeature(admin.GetIdentity(), ZBerryESPOverlay.FEATURE, on, args, ZBerryPermissions.FunctionNode(funcName));
		if (ZBerryStreamServer.Get()) ZBerryStreamServer.Get().SetEspActive(admin.GetIdentity(), on && fValues[0] != 0);

		if (on) {
			MessagePlayer(admin, "ESP on");
		} else {
			MessagePlayer(admin, "ESP off");
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
			MessagePlayer(admin, "This teleports every player to you. Run it again within " + (TP_ALL_CONFIRM_MS / 1000) + " seconds to confirm.");
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

		string result = "Teleported " + targets.Count() + " players";
		if (inVehicles > 0) result += ", " + inVehicles + " skipped (in vehicles)";
		if (protectedCount > 0) result += ", " + protectedCount + " skipped (protected from your rank)";
		MessagePlayer(admin, result);
		ZomberryBase.Log( "ZomBerryAdmin", GetPlayerName(admin) + " teleported all players to " + center.ToString() + ": " + result );
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

	void DeleteObj( string funcName, int adminId, int targetId, vector cursor ) {
		ref array<Object> nearest_objects = new array<Object>;
		ref array<CargoBase> proxy_cargos = new array<CargoBase>;
		Object toBeDeleted;

		GetGame().GetObjectsAtPosition3D(cursor, 1.5, nearest_objects, proxy_cargos);
		if (nearest_objects.Count() < 1) return;
		toBeDeleted = NULL;

		for (int i = 0; i < nearest_objects.Count(); ++i) {
			string tempObjId = nearest_objects.Get(i).ToString(); tempObjId.ToLower();
			if (nearest_objects.Get(i).IsKindOf("SurvivorBase") || tempObjId.Contains("static")) continue;
			if (nearest_objects.Get(i).IsWell() || nearest_objects.Get(i).IsBush()) continue;
			if (nearest_objects.Get(i).IsRock() || nearest_objects.Get(i).IsTree()) continue;

			toBeDeleted = nearest_objects.Get(i);
			break;
		}
		if (!toBeDeleted) return;

		if (!m_deleteList.Contains(adminId)) {
			m_deleteList.Insert(adminId, toBeDeleted.ToString());
			MessagePlayer(ZBGetPlayerById(adminId), "You're going to annihilate " + toBeDeleted.ToString() + ", execute this function again to continue.");
		} else if (m_deleteList.Get(adminId) != toBeDeleted.ToString()) {
			m_deleteList.Set(adminId, toBeDeleted.ToString());
			MessagePlayer(ZBGetPlayerById(adminId), "You're going to annihilate " + toBeDeleted.ToString() + ", execute this function again to continue.");
		} else if (m_deleteList.Get(adminId) == toBeDeleted.ToString()) {
			MessagePlayer(ZBGetPlayerById(adminId), toBeDeleted.ToString() + " annihilated.");
			GetGame().ObjectDelete(toBeDeleted);
			m_deleteList.Remove(adminId);
		}
	}
};
