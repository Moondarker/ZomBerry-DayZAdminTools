class ZomberryStockFunctions {
	ref ZomberryCmdAPI m_ZomberryCmdAPI;
	autoptr TIntArray m_spectatingList = new TIntArray;
	autoptr TIntStringMap m_deleteList = new TIntStringMap;

	void ZomberryStockFunctions() {
		m_ZomberryCmdAPI = GetZomberryCmdAPI();
	}

	void Init() {
		m_ZomberryCmdAPI.AddCategory("OnTarget", 0xFF42AAFF);
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Cursor", "TPCur", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Teleport - Me 10 meters Forward", "TPForward", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Teleport - Me to Target", "TPToTarget", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Me", "TPToAdmin", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Heal", "HealTarget", this, "OnTarget", false);
		m_ZomberryCmdAPI.AddCommand("Toggle god", "GodTarget", this, "OnTarget", false);
		//m_ZomberryCmdAPI.AddCommand("Repair item in hands", "RepairTargetHands", this, "OnTarget", false); //Not ready yet
		m_ZomberryCmdAPI.AddCommand("Refuel and repair", "RefuelAndRepair", this, "OnTarget", false);

		m_ZomberryCmdAPI.AddCategory("==", 0xFFFF7C75);
		m_ZomberryCmdAPI.AddCommand("Kill", "KillTarget", this, "==");
		m_ZomberryCmdAPI.AddCommand("Bite", "BiteTarget", this, "==", true, {
			new ZBerryFuncParam("Bites", {1, 25, 1,}),
		});
		m_ZomberryCmdAPI.AddCommand("Strip", "StripTarget", this, "==");
		m_ZomberryCmdAPI.AddCommand("Induce vomiting", "RejectBellyTarget", this, "==", true, {
			new ZBerryFuncParam("Seconds", {5, 30, 5,}),
		});
		m_ZomberryCmdAPI.AddCommand("Induce laughter", "PsycoTarget", this, "==");
		m_ZomberryCmdAPI.AddCommand("Induce sneeze", "SneezeTarget", this, "==");
		m_ZomberryCmdAPI.AddCommand("Set health", "SetHealthTarget", this, "==", true, {
			new ZBerryFuncParam("Health", {1, 100, 100,}),
		});
		m_ZomberryCmdAPI.AddCommand("Set blood", "SetBloodTarget", this, "==", true, {
			new ZBerryFuncParam("Blood", {1, 5000, 5000,}),
		});

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
			new ZBerryFuncParam("Month", {1, 12, 1,}),
			new ZBerryFuncParam("Year", {1970, 2119, 2019,}),
		});
		m_ZomberryCmdAPI.AddCommand("Set weather", "SetWeather", this, "OnServer", false, {
			new ZBerryFuncParam("Fog", {0, 100, 0,}),
			new ZBerryFuncParam("Overcast", {0, 100, 0,}),
			new ZBerryFuncParam("Rain", {0, 100, 0,}),
		});
		m_ZomberryCmdAPI.AddCommand("Delete object near cursor", "DeleteObj", this, "OnServer", false);
	}

	void MessagePlayer(PlayerBase player, string msg) {
		Param1<string> param = new Param1<string>( "[ZomBerry]: " + msg );
		if (GetGame().IsMultiplayer()) {
			GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, param, true, player.GetIdentity());
		} else {
			GetGame().GetMission().OnEvent(ChatMessageEventTypeID, new ChatMessageEventParams(CCAdmin, "[ZomBerry]", msg, ""));
		}
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

	void TPForward( string funcName, int adminId, int targetId, vector cursor ) {
		const int tpDist = 10;
		vector currentPos, curToTgtDir, targetPos;
		PlayerBase admin = ZBGetPlayerById(adminId);

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
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (SetPosSafe(target, cursor)) {
			MessagePlayer(admin, "Target teleported");
		} else {
			MessagePlayer(admin, "Unable to teleport: target in vehicle");
		}
	}

	void TPToTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (SetPosSafe(admin, GetPosSafe(target))) {
			MessagePlayer(admin, "Teleported to target");
		} else {
			MessagePlayer(admin, "Unable to teleport: get out of vehicle!");
		}
	}

	void TPToAdmin( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (SetPosSafe(target, GetPosSafe(admin))) {
			MessagePlayer(admin, "Target teleported");
		} else {
			MessagePlayer(admin, "Unable to teleport: target in vehicle");
		}
	}

	void HealTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		BleedingSourcesManagerServer BSMgr = target.GetBleedingManagerServer();

		target.SetHealth(target.GetMaxHealth());
		target.SetHealth("", "Blood", target.GetMaxHealth("", "Blood"));
		target.GetStatStamina().Set(1000);
		target.GetStatEnergy().Set(1000);
		target.GetStatWater().Set(1000);
		BSMgr.RemoveAllSources();

		if (admin) MessagePlayer(admin, "Healed target");
	}

	void GodTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (target.ZBIsGod()) {
			target.ZBSetGod(false);
			MessagePlayer(target, "God mode deactivated");
			if (adminId != targetId) MessagePlayer(admin, "God mode deactivated for " + target.GetIdentity().GetName());

		} else {
			HealTarget("HealTarget", -1, targetId, "0 0 0");

			target.ZBSetGod(true);
			MessagePlayer(target, "God mode activated");
			if (adminId != targetId) MessagePlayer(admin, "God mode activated for " + target.GetIdentity().GetName());
		}
	}

	void RepairTargetHands( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		InventoryItem targetItem = target.GetItemInHands();

		if (!targetItem) {
			MessagePlayer(admin, "No item in target's hands");
			return;
		}

		MessagePlayer(admin, "Item: " + targetItem.GetDisplayName() + ", start health: " + targetItem.GetHealth().ToString() + " (state " + targetItem.GetHealthLevel().ToString() + ")");
		targetItem.AddHealth("", "", (targetItem.GetMaxHealth() - targetItem.GetHealth())); //SetHealth doesn't update HealthLevel
		targetItem.ProcessDirectDamage(DT_FIRE_ARM, EntityAI.Cast(targetItem), targetItem.GetDamageZoneNameByComponentIndex(0), "Bullet_556x45", "0 0 0", 0);
		ItemBase.Cast(targetItem).SetQuantityMax();
		MessagePlayer(ZBGetPlayerById(adminId), "New health: " + targetItem.GetHealth().ToString() + " out of " + targetItem.GetMaxHealth("", "").ToString() + " (state " + targetItem.GetHealthLevel().ToString() + ")");

		for (int i = 0; i < targetItem.GetInventory().AttachmentCount(); ++i) {
			EntityAI attachment = targetItem.GetInventory().GetAttachmentFromIndex(i);
			attachment.AddHealth("", "", (attachment.GetMaxHealth() - attachment.GetHealth()));
			attachment.ProcessDirectDamage(DT_FIRE_ARM, EntityAI.Cast(targetItem), attachment.GetDamageZoneNameByComponentIndex(0), "Bullet_556x45", "0 0 0", 0);
			if (attachment.IsMagazine()) Magazine.Cast(attachment).ServerSetAmmoMax();
		}

		array<EntityAI> subItems = new array<EntityAI>;
		if (targetItem.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, subItems)) {
			for(int j = 0; j < subItems.Count(); ++j) {
				if (!subItems.Get(j)) continue;
				subItems.Get(j).AddHealth("", "", (subItems.Get(j).GetMaxHealth() - subItems.Get(j).GetHealth()));
				subItems.Get(j).ProcessDirectDamage(DT_FIRE_ARM, EntityAI.Cast(targetItem), subItems.Get(j).GetDamageZoneNameByComponentIndex(0), "Bullet_556x45", "0 0 0", 0);
				ItemBase.Cast(subItems.Get(j)).SetQuantityMax();
			}
		}

		targetItem.SetSynchDirty();

		MessagePlayer(admin, "Repaired item to the best condition it can achieve");
	}

	void RefuelAndRepair( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		TStringArray slotsCar = new TStringArray;
		TStringArray slotsTemp = new TStringArray;
		string strName, strNameTemp;

		Car toBeFixed;
		ItemBase attachmentTemp;
		ref array<Object> nearest_objects = new array<Object>;
		ref array<CargoBase> proxy_cargos = new array<CargoBase>;
		int i, j, m, sc;
		vector position = GetPosSafe(target);
		GetGame().GetObjectsAtPosition ( position, 15, nearest_objects, proxy_cargos );

		for (i = 0; i < nearest_objects.Count(); ++i) {
			if (nearest_objects[i].IsKindOf("CarScript")) {
				toBeFixed = Car.Cast(nearest_objects[i]);

				//Repair
				GetGame().ConfigGetTextArray("CfgVehicles " + toBeFixed.GetType() + " attachments", slotsCar);
				ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: Fixing " + toBeFixed.GetType());

				sc = GetGame().ConfigGetChildrenCount("CfgVehicles");
				for (j = 0; j < sc; j++) {
					GetGame().ConfigGetChildName("CfgVehicles", j, strName );
					strNameTemp = strName; strNameTemp.ToLower();

					if (!GetGame().IsKindOf(strNameTemp, "Inventory_Base") || strNameTemp.Contains("ruined")) continue;

					if (GetGame().ConfigGetType("CfgVehicles " + strName + " inventorySlot") == CT_ARRAY) {
						GetGame().ConfigGetTextArray("CfgVehicles " + strName + " inventorySlot", slotsTemp);
					} else {
						GetGame().ConfigGetText("CfgVehicles " + strName + " inventorySlot", strNameTemp);
						slotsTemp.Insert(strNameTemp);
					}

					for (m = 0; m < slotsTemp.Count(); m++) {
						if (slotsCar.Find(slotsTemp[m]) != -1) {
							Class.CastTo(attachmentTemp, toBeFixed.FindAttachmentBySlotName(slotsTemp[m]));
							if (attachmentTemp) {
								if (attachmentTemp.GetHealth01() > 0.75) continue;
								ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: " + attachmentTemp.GetType() + " (" + attachmentTemp.GetHealth().ToString() + " HP) - detached");
								toBeFixed.GetInventory().DropEntity(InventoryMode.PREDICTIVE, toBeFixed, attachmentTemp);
								attachmentTemp.SetPosition("0 0 0");
								GetGame().ObjectDelete(attachmentTemp);
							}

							ZomberryBase.DebugLog(2, "ZomBerryFncDbg", "RefuelAndRepair: Found and attached " + strName);
							toBeFixed.GetInventory().CreateInInventory(strName);
						}
					}

					slotsTemp.Clear();
				}

				toBeFixed.SetHealth("Engine", "", toBeFixed.GetMaxHealth("Engine", ""));

				//Refuel
				float fuelReq = toBeFixed.GetFluidCapacity( CarFluid.FUEL ) - (toBeFixed.GetFluidCapacity( CarFluid.FUEL ) * toBeFixed.GetFluidFraction( CarFluid.FUEL ));
				float oilReq = toBeFixed.GetFluidCapacity( CarFluid.OIL ) - (toBeFixed.GetFluidCapacity( CarFluid.OIL ) * toBeFixed.GetFluidFraction( CarFluid.OIL ));
				float coolantReq = toBeFixed.GetFluidCapacity( CarFluid.COOLANT ) - (toBeFixed.GetFluidCapacity( CarFluid.COOLANT ) * toBeFixed.GetFluidFraction( CarFluid.COOLANT ));
				float brakeReq = toBeFixed.GetFluidCapacity( CarFluid.BRAKE ) - (toBeFixed.GetFluidCapacity( CarFluid.BRAKE ) * toBeFixed.GetFluidFraction( CarFluid.BRAKE ));
				toBeFixed.Fill( CarFluid.FUEL, fuelReq );
				toBeFixed.Fill( CarFluid.OIL, oilReq );
				toBeFixed.Fill( CarFluid.COOLANT, coolantReq );
				toBeFixed.Fill( CarFluid.BRAKE, brakeReq );
			}
		}
		MessagePlayer(ZBGetPlayerById(adminId), "Refueled and repaired vehicles near to target");
	}

	void KillTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		target.SetHealth(0);
		MessagePlayer(admin, "Killed target");
	}

	void BiteTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		BleedingSourcesManagerServer BSMgr = target.GetBleedingManagerServer();

		for (int i = 0; i < fValues[0]; ++i) {
			for (int j = 0; j < 15; ++j) {
				if (BSMgr.AttemptAddBleedingSource(Math.RandomInt(0, 100))) break;
			}
		}
		MessagePlayer(admin, "Target was bitten " + fValues[0] + " times and now losing blood");
	}

	void StripTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		target.RemoveAllItems();
		MessagePlayer(admin, "Target was stripped");
	}

	void RejectBellyTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (!target.GetCommand_Vehicle()) {
			SymptomBase symptom = ZBGetPlayerById(targetId).GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_VOMIT);
			symptom.SetDuration(fValues[0]);
			MessagePlayer(admin, "Target is gonna be sick for " + fValues[0] + " seconds");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void PsycoTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_LAUGHTER);
			MessagePlayer(admin, "Target is laughing...");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void SneezeTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		if (!target.GetCommand_Vehicle()) {
			target.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_SNEEZE);
			MessagePlayer(admin, "Target sneezed");
		} else {
			MessagePlayer(admin, "Target in vehicle, this action might cause game crash");
		}
	}

	void SetHealthTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		target.SetHealth(fValues[0]);

		MessagePlayer(admin, "Target health level was set to " + fValues[0]);
	}

	void SetBloodTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		PlayerBase admin = ZBGetPlayerById(adminId);

		if (!target) {
			MessagePlayer(admin, "Target not found (probably disconnected?)");
			return;
		}

		target.SetHealth("", "Blood", fValues[0]);

		MessagePlayer(admin, "Target blood level was set to " + fValues[0]);
	}

	void FreeCamAdm( string funcName, int adminId, int targetId, vector cursor ) {
		int listId = m_spectatingList.Find(adminId);
		PlayerBase adminPly = ZBGetPlayerById(adminId);
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

	void SetTime( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);
		GetGame().GetWorld().SetDate(year, month, day, fValues[0], fValues[1]);
		MessagePlayer(ZBGetPlayerById(adminId), "Time set to: " + fValues[0].ToString() + ":" + fValues[1].ToString() + " (it may take some time to effect!)");
	}

	void SetDate( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
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

	void SetWeather( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		int year, month, day, hour, minute;
		float fcMin, fcMax, fnMin, fnMax, ftMin, ftMax; //Damn, a lot of variables...
		Weather WMgr = GetGame().GetWeather();

		WMgr.GetFog().GetForecastChangeLimits(fcMin, fcMax); //But why would I need to Ctrl+C - Ctrl+V so much code?
		WMgr.GetFog().GetLimits(fnMin, fnMax);
		WMgr.GetFog().GetForecastTimeLimits(ftMin, ftMax); //Because users are important, and they might have their own weather settings!

		WMgr.GetFog().SetForecastChangeLimits(0, 1);
		WMgr.GetFog().SetLimits(0, 1);
		WMgr.GetFog().SetForecastTimeLimits(0, 3600);

		WMgr.GetFog().Set((fValues[0]/100), 1, 360);
		WMgr.GetFog().SetNextChange(0.01);
		WMgr.GetFog().SetForecastChangeLimits(fcMin, fcMax);
		WMgr.GetFog().SetLimits(fnMin, fnMax);
		WMgr.GetFog().SetForecastTimeLimits(ftMin, ftMax);


		WMgr.GetOvercast().GetForecastChangeLimits(fcMin, fcMax);
		WMgr.GetOvercast().GetLimits(fnMin, fnMax);
		WMgr.GetOvercast().GetForecastTimeLimits(ftMin, ftMax);

		WMgr.GetOvercast().SetForecastChangeLimits(0, 1);
		WMgr.GetOvercast().SetLimits(0, 1);
		WMgr.GetOvercast().SetForecastTimeLimits(0, 3600);

		WMgr.GetOvercast().Set((fValues[1]/100), 1, 360);
		WMgr.GetOvercast().SetNextChange(0.01);
		WMgr.GetOvercast().SetForecastChangeLimits(fcMin, fcMax);
		WMgr.GetOvercast().SetLimits(fnMin, fnMax);
		WMgr.GetOvercast().SetForecastTimeLimits(ftMin, ftMax);


		WMgr.GetRain().GetForecastChangeLimits(fcMin, fcMax);
		WMgr.GetRain().GetLimits(fnMin, fnMax);
		WMgr.GetRain().GetForecastTimeLimits(ftMin, ftMax);

		WMgr.GetRain().SetForecastChangeLimits(0, 1);
		WMgr.GetRain().SetLimits(0, 1);
		WMgr.GetRain().SetForecastTimeLimits(0, 3600);

		WMgr.GetRain().Set((fValues[2]/100), 1, 360);
		WMgr.GetRain().SetNextChange(0.01);
		WMgr.SetStorm((fValues[1]*fValues[2])/10000, 0.7, 3600/((fValues[2]/2)+0.1));
		WMgr.GetRain().SetForecastChangeLimits(fcMin, fcMax);
		WMgr.GetRain().SetLimits(fnMin, fnMax);
		WMgr.GetRain().SetForecastTimeLimits(ftMin, ftMax);

		MessagePlayer(ZBGetPlayerById(adminId), "Weather changed (due to sync problems you might need to relog)");
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
