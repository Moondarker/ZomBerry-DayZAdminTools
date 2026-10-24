modded class PlayerBase
{
	private bool ZBGodMode;

	//Invisibility (the "Toggle invisibility" function): synced to every client, which then shrinks this character to
	//nothing and plays none of its sounds. The server keeps it out of the AI's noise and targets, and it can't be hurt.
	//Independent of god mode. Voice chat isn't affected
	protected bool m_ZBInvisible;
	protected bool m_ZBShrunkHere; //What this client last applied
	static const float ZB_HIDDEN_SCALE = 0.00001;

	//Server: gear of a player in god mode or invisible can't be damaged either (worn, attached, in cargo, in hands,
	//weapons included). Only items this switched off are switched back on, when they leave the player or protection ends
	protected ref array<EntityAI> m_ZBProtectedGear = new array<EntityAI>; //Weak: NULL once an item is deleted
	protected float m_ZBGearTimer;
	static const float ZB_GEAR_INTERVAL = 2; //Seconds between checks for gear picked up or put away

	//Infinite ammo: a server flag on the character, plus the character it's on for its own client (told through a
	//ZomBerry client feature rather than a synced variable: PlayerBase already has about as many as the engine allows)
	protected bool m_ZBInfiniteAmmo;
	protected static bool s_ZBLocalInfiniteAmmo;
	protected static int s_ZBInfiniteAmmoLow, s_ZBInfiniteAmmoHigh;

	override void Init() {
		super.Init();
		RegisterNetSyncVariableBool("m_ZBInvisible");
	}

	bool ZBIsGod() {
		return ZBGodMode;
	};

	void ZBSetGod(bool isGod) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;

		ZBGodMode = isGod;
		ZBUpdateAllowDamage();
	};

	//Synced: true on clients too
	bool ZBIsInvisible() {
		return m_ZBInvisible;
	}

	//Server
	void ZBSetInvisible(bool invisible) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;
		if (invisible == m_ZBInvisible) return;

		m_ZBInvisible = invisible;
		ZBUpdateAllowDamage();
		SetSynchDirty();
	}

	//Server and the character's own client
	bool ZBHasInfiniteAmmo() {
		if (GetGame().IsServer()) return m_ZBInfiniteAmmo;
		if (!s_ZBLocalInfiniteAmmo) return false;

		int low, high;
		GetNetworkID(low, high);
		return low == s_ZBInfiniteAmmoLow && high == s_ZBInfiniteAmmoHigh;
	}

	//Server
	void ZBSetInfiniteAmmo(bool enabled) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;
		m_ZBInfiniteAmmo = enabled;
	}

	//Client: which of this client's characters has it
	static void ZBSetLocalInfiniteAmmo(bool enabled, int networkLow, int networkHigh) {
		s_ZBLocalInfiniteAmmo = enabled;
		s_ZBInfiniteAmmoLow = networkLow;
		s_ZBInfiniteAmmoHigh = networkHigh;
	}

	//After a shot: the magazine full again, on the server and on the shooter's client like vanilla's debug option
	void ZBRefillAfterShot(Weapon_Base weapon) {
		if (!weapon || !ZBHasInfiniteAmmo()) return;

		Magazine magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
		if (!magazine) return;
		if (GetGame().IsServer()) {
			magazine.ServerSetAmmoMax();
		} else {
			magazine.LocalSetAmmoMax();
		}
	}

	//God mode and invisibility each keep damage off, neither switches the other. SetAllowDamage(false) also stops
	//health and blood lost from script (bleeding, hunger), which EEOnDamageCalculated below doesn't see
	protected void ZBUpdateAllowDamage() {
		SetAllowDamage(!ZBProtected());
		ZBUpdateGearProtection();
	}

	//God mode or invisibility: the player, their gear and their temperature are looked after
	bool ZBProtected() {
		return ZBGodMode || m_ZBInvisible;
	}

	//Server: everything the player carries can't be damaged while they're protected; items that left, or all of them
	//once protection ends, get their damage back
	protected void ZBUpdateGearProtection() {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;

		array<EntityAI> carried = new array<EntityAI>;
		if (ZBProtected()) {
			GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, carried);
			EntityAI inHands = GetHumanInventory().GetEntityInHands();
			if (inHands && carried.Find(inHands) == -1) carried.Insert(inHands);
		}

		for (int i = m_ZBProtectedGear.Count() - 1; i >= 0; --i) {
			EntityAI protectedItem = m_ZBProtectedGear[i];
			if (protectedItem && carried.Find(protectedItem) != -1) continue;
			if (protectedItem) protectedItem.SetAllowDamage(true);
			m_ZBProtectedGear.Remove(i);
		}

		foreach (EntityAI item : carried) {
			if (!item || item == this || !item.GetAllowDamage() || m_ZBProtectedGear.Find(item) != -1) continue; //Already undamageable: not ours to switch back
			item.SetAllowDamage(false);
			m_ZBProtectedGear.Insert(item);
		}
	}

	//Other players' clients hide the character; its own client keeps seeing itself
	protected bool ZBHidesHere() {
		return GetGame().IsClient() && GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT;
	}

	//Sounds and noise are dropped everywhere but on the character's own client
	protected bool ZBSilent() {
		return m_ZBInvisible && GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT;
	}

	override void OnVariablesSynchronized() {
		super.OnVariablesSynchronized();
		ZBApplyInvisibility();
	}

	//Shrunk rather than hidden piece by piece: clothing, bare body parts and the item in hands are all drawn relative
	//to the character and shrink with it, while the engine kept showing hidden ones again (the item in hands flickered,
	//a body part showed as a stray texture). Not 0: a degenerate transform could break the maths that inverts it
	protected void ZBApplyInvisibility() {
		if (!ZBHidesHere()) return;

		if (m_ZBInvisible) {
			if (GetScale() > ZB_HIDDEN_SCALE * 2) SetScale(ZB_HIDDEN_SCALE);
			m_ZBShrunkHere = true;
		} else if (m_ZBShrunkHere) {
			SetScale(1);
			m_ZBShrunkHere = false;
		}
	}

	override void EOnFrame(IEntity other, float timeSlice) {
		super.EOnFrame(other, timeSlice);

		//Server: gear picked up or put away since the last check
		if ((!GetGame().IsMultiplayer() || GetGame().IsServer()) && (ZBProtected() || m_ZBProtectedGear.Count() > 0)) {
			m_ZBGearTimer -= timeSlice;
			if (m_ZBGearTimer <= 0) {
				m_ZBGearTimer = ZB_GEAR_INTERVAL;
				ZBUpdateGearProtection();
			}
		}

		if (m_ZBInvisible) ZBApplyInvisibility();
	}

	//Again right before drawing, in case the frame's movement updates set the character's transform back to full size
	override void EOnPostFrame(IEntity other, int extra) {
		super.EOnPostFrame(other, extra);
		if (m_ZBInvisible) ZBApplyInvisibility();
	}

	//Steps, movement, clothing, handling and voice sounds (not voice chat), and the noise infected hear
	override void OnStepEvent(string pEventType, string pUserString, int pUserInt) {
		if (ZBSilent()) return;
		super.OnStepEvent(pEventType, pUserString, pUserInt);
	}

	override void OnSoundEvent(string pEventType, string pUserString, int pUserInt) {
		if (ZBSilent()) return;
		super.OnSoundEvent(pEventType, pUserString, pUserInt);
	}

	override bool PlaySoundEventEx(EPlayerSoundEventID id, bool from_anim_system = false, bool is_from_server = false, int param = 0) {
		if (ZBSilent()) return false;
		return super.PlaySoundEventEx(id, from_anim_system, is_from_server, param);
	}

	override void AddNoise(NoiseParams noisePar, float noiseMultiplier = 1.0) {
		if (m_ZBInvisible) return;
		super.AddNoise(noisePar, noiseMultiplier);
	}

	//Hits never land on invisible players, even after another mod switched damage back on with SetAllowDamage(true)
	//without asking ZBIsGod() first
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		if (m_ZBInvisible) return false;
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override bool CanBeTargetedByAI(EntityAI ai) {
		if (m_ZBInvisible) return false;
		return super.CanBeTargetedByAI(ai);
	}
};
