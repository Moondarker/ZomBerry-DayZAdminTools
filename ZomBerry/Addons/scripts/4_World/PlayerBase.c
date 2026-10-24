modded class PlayerBase
{
	private bool ZBGodMode;

	//Invisibility (the "Toggle invisibility" function): synced to every client, which then shrinks this character to
	//nothing, stops simulating it, takes it out of collisions and plays none of its sounds. The server keeps it out of
	//the AI's noise and targets, and it can't be hurt. Independent of god mode. Voice chat isn't affected
	protected bool m_ZBInvisible;
	protected bool m_ZBShrunkHere; //What this client last applied
	protected int m_ZBSavedLayer; //Its collision layer before, given back when shown again
	protected bool m_ZBMadeNonSolid; //Others walk through it while invisible, everywhere (see ZBUpdateSolid)
	static const float ZB_HIDDEN_SCALE = 0.00001;
	protected bool m_ZBSelfHidden; //Its own client: body and worn gear hidden, as a reminder
	protected float m_ZBSelfTimer;
	static const float ZB_SELF_INTERVAL = 0.5;
	protected static int s_ZBOwnLow, s_ZBOwnHigh; //Network id of this client's own character, also once detached

	//Server: gear of a player in god mode or invisible can't be damaged either (worn, attached, in cargo, in hands,
	//weapons included). Only items this switched off are switched back on, when they leave the player or protection ends
	protected ref array<EntityAI> m_ZBProtectedGear = new array<EntityAI>; //Weak: NULL once an item is deleted
	protected float m_ZBGearTimer;
	static const float ZB_GEAR_INTERVAL = 2; //Seconds between checks for gear picked up or put away

	//Infinite ammo: a server flag on the character, plus the character it's on for its own client (told through a
	//ZomBerry client feature rather than a synced variable: PlayerBase already has about as many as the engine allows)
	protected bool m_ZBInfiniteAmmo;
	protected float m_ZBAmmoTimer;
	static const float ZB_AMMO_INTERVAL = 0.5;
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

	//Server, twice a second while infinite ammo is on: the magazine of the weapon in hands is topped up, so one that was
	//empty (which can't be fired, so never refilled after a shot) fills up too. An empty chamber needs a reload
	protected void ZBTopUpAmmo() {
		Weapon_Base weapon = Weapon_Base.Cast(GetHumanInventory().GetEntityInHands());
		if (!weapon) return;

		Magazine magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
		if (magazine && magazine.GetAmmoCount() < magazine.GetAmmoMax()) magazine.ServerSetAmmoMax();
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

	//Other players' clients hide the character; its own client keeps seeing itself. So does a client spectating it:
	//the camera follows its bones, which a shrunk or frozen character would take along
	protected bool ZBHidesHere() {
		if (!GetGame().IsClient() || GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT) return false;
		return !ZomBerryCamSpec.IsTarget(this);
	}

	//Sounds and noise are dropped everywhere but on the character's own client, and on a client that hides it
	protected bool ZBSilent() {
		return (m_ZBInvisible && GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT) || m_ZBShrunkHere;
	}

	//In FreeCam the admin's client controls only the camera, and their body is left behind: hidden and frozen on that
	//client too, so it isn't in the way and plays no sounds when the camera moves
	protected bool ZBInFreeCamHere() {
		if (!GetGame().IsClient() || GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT) return false;
		if (!ZomBerryCamFree.IsUsedHere()) return false;

		int low, high;
		GetNetworkID(low, high);
		return low == s_ZBOwnLow && high == s_ZBOwnHigh;
	}

	//Other players can't target an invisible character with actions (check pulse, give, restrain...): their clients
	//pick action targets with this. The server and its own client keep vanilla's answer, for actions on themselves
	override bool CanBeActionTarget() {
		if (m_ZBInvisible && GetGame().IsClient() && GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT) return false;
		return super.CanBeActionTarget();
	}

	override void OnVariablesSynchronized() {
		super.OnVariablesSynchronized();
		ZBApplyInvisibility();
		ZBApplySelfInvisibility();
	}

	//Shrunk rather than hidden piece by piece: clothing, bare body parts and the item in hands are all drawn relative
	//to the character and shrink with it, while the engine kept showing hidden ones again (the item in hands flickered,
	//a body part showed as a stray texture). Not 0: a degenerate transform could break the maths that inverts it.
	//Not simulated and not colliding on this client either. Only on other players' clients: on the server and its own
	//client the same physics body keeps the character on the ground
	protected void ZBApplyInvisibility() {
		if ((m_ZBInvisible && ZBHidesHere()) || ZBInFreeCamHere()) {
			if (!m_ZBShrunkHere) {
				m_ZBSavedLayer = -1;
				if (dBodyIsSet(this)) m_ZBSavedLayer = dBodyGetInteractionLayer(this);
				m_ZBShrunkHere = true;
			}
			if (GetScale() > ZB_HIDDEN_SCALE * 2) SetScale(ZB_HIDDEN_SCALE);
			if (!GetIsSimulationDisabled()) DisableSimulation(true);
			if (dBodyIsSet(this) && dBodyGetInteractionLayer(this) != PhxInteractionLayers.NOCOLLISION) dBodySetInteractionLayer(this, PhxInteractionLayers.NOCOLLISION);
		} else if (m_ZBShrunkHere) {
			m_ZBShrunkHere = false;
			SetScale(1);
			DisableSimulation(false);
			if (m_ZBSavedLayer >= 0 && dBodyIsSet(this)) dBodySetInteractionLayer(this, m_ZBSavedLayer);
		}
	}

	//The invisible player's own client hides their body, bare body parts and what they wear, as a reminder that others
	//don't see them. Not shrunk: the camera and aiming follow the body. The item in hands stays, for aiming. Re-applied
	//twice a second, as the engine shows hidden parts again now and then
	protected void ZBApplySelfInvisibility() {
		bool hide = m_ZBInvisible && GetGame().IsClient() && GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT;
		if (!hide && !m_ZBSelfHidden) return;
		m_ZBSelfHidden = hide;

		ZBSetTreeInvisible(this, hide);
		GameInventory inventory = GetInventory();
		for (int i = 0; i < inventory.GetAttachmentSlotsCount(); ++i) ZBSetBodyPartInvisible(inventory.GetAttachmentSlotId(i), hide);
	}

	//Bare body parts (head, hands, torso, legs, feet and any other) are placeholders of their slots, shown while the
	//slot is empty, not attachments
	protected void ZBSetBodyPartInvisible(int slotId, bool invisible) {
		EntityAI bodyPart = GetInventory().FindPlaceholderForSlot(slotId);
		if (!bodyPart) return;

		bodyPart.SetInvisible(invisible);
		bodyPart.OnInvisibleSet(invisible);
	}

	//Recursive: parameters are copied into locals first, a nested call overwrote the caller's (see AGENTS.md)
	protected static void ZBSetTreeInvisible(EntityAI entityArg, bool invisibleArg) {
		EntityAI entity = entityArg;
		bool invisible = invisibleArg;
		entity.SetInvisible(invisible);
		entity.OnInvisibleSet(invisible);

		GameInventory inventory = entity.GetInventory();
		if (!inventory) return;
		for (int i = 0; i < inventory.AttachmentCount(); ++i) {
			EntityAI attachment = inventory.GetAttachmentFromIndex(i);
			if (attachment) ZBSetTreeInvisible(attachment, invisible);
		}
	}

	//New clothing is hidden too; taking clothing off uncovers the body part under it (vanilla's OnWasDetached shows
	//the clothing again)
	override void EEItemAttached(EntityAI item, string slot_name) {
		super.EEItemAttached(item, slot_name);
		if (m_ZBSelfHidden) ZBSetTreeInvisible(item, true);
	}

	override void EEItemDetached(EntityAI item, string slot_name) {
		super.EEItemDetached(item, slot_name);
		if (m_ZBSelfHidden) ZBSetBodyPartInvisible(InventorySlots.GetSlotIdFromString(slot_name), true);
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
		if (m_ZBInfiniteAmmo && (!GetGame().IsMultiplayer() || GetGame().IsServer())) {
			m_ZBAmmoTimer -= timeSlice;
			if (m_ZBAmmoTimer <= 0) {
				m_ZBAmmoTimer = ZB_AMMO_INTERVAL;
				ZBTopUpAmmo();
			}
		}

		if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT) GetNetworkID(s_ZBOwnLow, s_ZBOwnHigh);
		if (m_ZBInvisible || m_ZBMadeNonSolid) ZBUpdateSolid();
		if (m_ZBInvisible || m_ZBShrunkHere || ZomBerryCamFree.IsUsedHere()) ZBApplyInvisibility();

		if (!m_ZBInvisible && !m_ZBSelfHidden) return;
		m_ZBSelfTimer -= timeSlice;
		if (m_ZBSelfTimer > 0) return;
		m_ZBSelfTimer = ZB_SELF_INTERVAL;
		ZBApplySelfInvisibility();
	}

	//Invisible characters aren't obstacles: other players walk through them. On every machine, as the server decides
	//where they may go and each client predicts it. The character still stands on the ground. Checked every frame, as
	//the engine switches it itself (vehicles: left to it there); given back once when invisibility ends
	protected void ZBUpdateSolid() {
		if (IsInVehicle()) return;

		if (m_ZBInvisible && IsAlive()) {
			if (PhysicsIsSolid()) PhysicsSetSolid(false);
			m_ZBMadeNonSolid = true;
		} else if (m_ZBMadeNonSolid) {
			m_ZBMadeNonSolid = false;
			PhysicsSetSolid(true);
		}
	}

	//Again right before drawing, in case the frame's updates gave the character its size, simulation or collisions back
	override void EOnPostFrame(IEntity other, int extra) {
		super.EOnPostFrame(other, extra);
		if (m_ZBInvisible || m_ZBShrunkHere || ZomBerryCamFree.IsUsedHere()) ZBApplyInvisibility();
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
