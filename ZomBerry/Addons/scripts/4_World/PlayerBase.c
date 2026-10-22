modded class PlayerBase
{
	private bool ZBGodMode;

	//Invisibility (the "Toggle invisibility" function): synced to every client, which then doesn't draw this character
	//or its gear and plays none of its sounds. The server keeps it out of the AI's noise and targets, and it can't be
	//hurt (invisible characters still take bullets). Independent of god mode. Voice chat isn't affected
	protected bool m_ZBInvisible;
	protected bool m_ZBHiddenHere; //What this client last applied

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

	//God mode and invisibility each keep damage off, neither switches the other. SetAllowDamage(false) also stops
	//health and blood lost from script (bleeding, hunger), which EEOnDamageCalculated below doesn't see
	protected void ZBUpdateAllowDamage() {
		SetAllowDamage(!ZBGodMode && !m_ZBInvisible);
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

	//The body, everything attached to it (clothing, their attachments), the bare body parts and the item in hands.
	//Re-applied on every sync while hidden, only once when shown again: showing the item in hands would undo an
	//action that hid it
	protected void ZBApplyInvisibility() {
		if (!ZBHidesHere()) return;
		if (!m_ZBInvisible && !m_ZBHiddenHere) return;
		m_ZBHiddenHere = m_ZBInvisible;

		ZBSetTreeInvisible(this, m_ZBInvisible);
		//Bare head, hands, torso, legs and feet are placeholders of these slots, not attachments: shown while the slot is empty
		array<string> bodyParts = {"Head", "Gloves", "Body", "Legs", "Feet"};
		foreach (string bodyPart : bodyParts) ZBSetBodyPartInvisible(InventorySlots.GetSlotIdFromString(bodyPart), m_ZBInvisible);

		EntityAI inHands = GetHumanInventory().GetEntityInHands();
		if (inHands) ZBSetTreeInvisible(inHands, m_ZBInvisible);
		if (m_ZBInvisible) {
			GetItemAccessor().HideItemInHands(true);
		} else if (!IsSwimming() && !IsClimbingLadder() && !IsInVehicle()) { //Where vanilla keeps it hidden
			GetItemAccessor().HideItemInHands(false);
		}
	}

	protected void ZBSetBodyPartInvisible(int slotId, bool invisible) {
		EntityAI bodyPart = GetInventory().FindPlaceholderForSlot(slotId);
		if (!bodyPart) return;

		bodyPart.SetInvisible(invisible);
		bodyPart.OnInvisibleSet(invisible);
	}

	protected static void ZBSetTreeInvisible(EntityAI entity, bool invisible) {
		entity.SetInvisible(invisible);
		entity.OnInvisibleSet(invisible);

		GameInventory inventory = entity.GetInventory();
		if (!inventory) return;
		for (int i = 0; i < inventory.AttachmentCount(); ++i) {
			EntityAI attachment = inventory.GetAttachmentFromIndex(i);
			if (attachment) ZBSetTreeInvisible(attachment, invisible);
		}
	}

	//New gear is hidden too; gear that leaves the character is shown again (vanilla's OnWasDetached does that for
	//attachments, the hands are done here)
	override void EEItemAttached(EntityAI item, string slot_name) {
		super.EEItemAttached(item, slot_name);
		if (m_ZBInvisible && ZBHidesHere()) ZBSetTreeInvisible(item, true);
	}

	//Taking clothing off uncovers the body part under it
	override void EEItemDetached(EntityAI item, string slot_name) {
		super.EEItemDetached(item, slot_name);
		if (m_ZBInvisible && ZBHidesHere()) ZBSetBodyPartInvisible(InventorySlots.GetSlotIdFromString(slot_name), true);
	}

	override void EEItemIntoHands(EntityAI item) {
		super.EEItemIntoHands(item);
		if (m_ZBInvisible && ZBHidesHere()) {
			ZBSetTreeInvisible(item, true);
			GetItemAccessor().HideItemInHands(true);
		}
	}

	//The item in hands is drawn by the engine's own switch, which actions turn back on when they finish
	override void TryHideItemInHands(bool hide, bool force = false) {
		super.TryHideItemInHands(hide, force);
		if (m_ZBInvisible && ZBHidesHere()) GetItemAccessor().HideItemInHands(true);
	}

	override void EEItemOutOfHands(EntityAI item) {
		super.EEItemOutOfHands(item);
		if (m_ZBInvisible && ZBHidesHere() && item) ZBSetTreeInvisible(item, false);
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
