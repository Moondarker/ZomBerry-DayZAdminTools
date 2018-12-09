class ZomberryStockFunctions {
	ref ZomberryCmdAPI m_ZomberryCmdAPI;

	void ZomberryStockFunctions() {
		m_ZomberryCmdAPI = GetZomberryCmdAPI();
	}

	void Init() {
		m_ZomberryCmdAPI.AddCategory("OnTarget", 0xFF42AAFF);
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Cursor", "TPCur", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Teleport - Me to Target", "TPToTarget", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Teleport - Target to Me", "TPToAdmin", this, "OnTarget");
		m_ZomberryCmdAPI.AddCommand("Heal", "HealTarget", this, "OnTarget", false);
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

	void TPCur( string funcName, int adminId, int targetId, vector cursor ) {
		if (SetPosSafe(ZBGetPlayerById(targetId), cursor)) {
			MessagePlayer(ZBGetPlayerById(adminId), "Target teleported");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Unable to teleport: target in vehicle");
		}
	}

	void TPToTarget( string funcName, int adminId, int targetId, vector cursor ) {
		if (SetPosSafe(ZBGetPlayerById(adminId), GetPosSafe(ZBGetPlayerById(targetId)))) {
			MessagePlayer(ZBGetPlayerById(adminId), "Teleported to target");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Unable to teleport: get out of vehicle!");
		}
	}

	void TPToAdmin( string funcName, int adminId, int targetId, vector cursor ) {
		if (SetPosSafe(ZBGetPlayerById(targetId), GetPosSafe(ZBGetPlayerById(adminId)))) {
			MessagePlayer(ZBGetPlayerById(adminId), "Target teleported");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Unable to teleport: target in vehicle");
		}
	}

	void HealTarget( string funcName, int adminId, int targetId, vector cursor ) {
		PlayerBase target = ZBGetPlayerById(targetId);
		BleedingSourcesManagerServer BSMgr = target.GetBleedingManagerServer();

		target.SetHealth(target.GetMaxHealth());
		target.SetHealth("", "Blood", target.GetMaxHealth("", "Blood"));
		target.GetStatStamina().Set(1000);
		target.GetStatEnergy().Set(1000);
		target.GetStatWater().Set(1000);
		BSMgr.RemoveAllSources();

		MessagePlayer(ZBGetPlayerById(adminId), "Healed target");
	}

	void RefuelAndRepair( string funcName, int adminId, int targetId, vector cursor ) {
		autoptr TStringArray attachments = {
			"SparkPlug", "CarRadiator", "CarBattery", "GlowPlug", "TruckExhaust", "TruckBattery", "EngineBelt",
			"HatchbackHood", "CivSedanHood", "CivSedanTrunk", "V3SHood", "BusHood", "HeadlightH7", "HatchbackTrunk",
			"HatchbackWheel", "CivSedanWheel", "V3SWheel", "V3SWheelDouble", "TransitBusWheel", "TransitBusWheelDouble",
			"HatchbackDoors_Driver", "HatchbackDoors_CoDriver", "CivVanDoors_Driver", "CivVanDoors_CoDriver", "V3SDoors_Driver",
			"BusDoors_Left", "BusDoors_Right", "CivSedanDoors_BackLeft", "CivSedanDoors_BackRight", "V3SDoors_CoDriver",
		};
		ref array<Object> nearest_objects = new array<Object>;
		ref array<CargoBase> proxy_cargos = new array<CargoBase>;
		Car toBeFixed;
		vector position = ZBGetPlayerById(targetId).GetPosition();
		GetGame().GetObjectsAtPosition ( position, 15, nearest_objects, proxy_cargos );

		for (int i = 0; i < nearest_objects.Count(); ++i) {
			if (nearest_objects[i].IsKindOf("CarScript")) {
				toBeFixed = Car.Cast(nearest_objects[i]);

				//Repair
				int slotsCount = toBeFixed.GetInventory().GetAttachmentSlotsCount();

				for (int m = 0; m < attachments.Count(); ++m) {
					EntityAI attachment = EntityAI.Cast( GetGame().CreateObject(attachments[m], vector.Zero, false, false, false) );

					ZomberryBase.Log( "ZomberryFncDbg", "RefuelAndRepair: trying " + attachments[m]);
					while (toBeFixed.GetInventory().CanAddAttachment(attachment)) { //TOBEFIXED: Known possible 'NULL pointer to instance'
						//Print("[ZomberryFnc] RefuelAndRepair: attached " + attachments[m]);
						toBeFixed.GetInventory().CreateAttachment(attachments[m]);
					}

					attachment.Delete();
				}

				toBeFixed.SetPosition(toBeFixed.GetPosition() + Vector(0, 0.3, 0));

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
		ZBGetPlayerById(targetId).SetHealth(0);
		MessagePlayer(ZBGetPlayerById(adminId), "Killed target");
	}

	void BiteTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		BleedingSourcesManagerServer BSMgr = ZBGetPlayerById(targetId).GetBleedingManagerServer();

		for (int i = 0; i < fValues[0]; ++i) {
			for (int j = 0; j < 15; ++j) {
				if (BSMgr.AttemptAddBleedingSource(Math.RandomInt(0, 100))) break;
			}
		}
		MessagePlayer(ZBGetPlayerById(adminId), "Target was bitten " + fValues[0] + " times and now losing blood");
	}

	void StripTarget( string funcName, int adminId, int targetId, vector cursor ) {
		ZBGetPlayerById(targetId).RemoveAllItems();
		MessagePlayer(ZBGetPlayerById(adminId), "Target was stripped");
	}

	void RejectBellyTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		if (!ZBGetPlayerById(targetId).GetCommand_Vehicle()) {
			SymptomBase symptom = ZBGetPlayerById(targetId).GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_VOMIT);
			symptom.SetDuration(fValues[0]);
			MessagePlayer(ZBGetPlayerById(adminId), "Target is gonna be sick for " + fValues[0] + " seconds");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Target in vehicle, this action might cause game crash");
		}
	}

	void PsycoTarget( string funcName, int adminId, int targetId, vector cursor ) {
		if (!ZBGetPlayerById(targetId).GetCommand_Vehicle()) {
			ZBGetPlayerById(targetId).GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_LAUGHTER);
			MessagePlayer(ZBGetPlayerById(adminId), "Target is laughing...");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Target in vehicle, this action might cause game crash");
		}
	}

	void SneezeTarget( string funcName, int adminId, int targetId, vector cursor ) {
		if (!ZBGetPlayerById(targetId).GetCommand_Vehicle()) {
			ZBGetPlayerById(targetId).GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_SNEEZE);
			MessagePlayer(ZBGetPlayerById(adminId), "Target sneezed");
		} else {
			MessagePlayer(ZBGetPlayerById(adminId), "Target in vehicle, this action might cause game crash");
		}
	}

	void SetHealthTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);

		target.SetHealth(fValues[0]);

		MessagePlayer(ZBGetPlayerById(adminId), "Target health level was set to " + fValues[0]);
	}

	void SetBloodTarget( string funcName, int adminId, int targetId, vector cursor, autoptr TIntArray fValues ) {
		PlayerBase target = ZBGetPlayerById(targetId);

		target.SetHealth("", "Blood", fValues[0]);

		MessagePlayer(ZBGetPlayerById(adminId), "Target blood level was set to " + fValues[0]);
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
};
