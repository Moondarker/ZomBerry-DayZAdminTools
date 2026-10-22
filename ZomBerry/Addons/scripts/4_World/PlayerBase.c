modded class PlayerBase
{
	private bool ZBGodMode;

	//Invisibility (the "Toggle invisibility" function): synced to every client, which then doesn't draw this character
	//or its gear and plays none of its sounds. The server keeps it out of the AI's noise and targets, and god mode
	//stays on while it lasts. Voice chat isn't affected
	protected bool m_ZBInvisible;
	protected bool m_ZBGodBeforeInvisible; //Restored when invisibility ends

	override void Init() {
		super.Init();
		RegisterNetSyncVariableBool("m_ZBInvisible");
	}

	bool ZBIsGod() {
		return ZBGodMode;
	};

	//While invisible, god mode can't be switched off: invisible characters still take bullets
	void ZBSetGod(bool isGod) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;

		if (!isGod && m_ZBInvisible) {
			m_ZBGodBeforeInvisible = false; //Off once invisibility ends
			return;
		}

		ZBGodMode = isGod;
		SetAllowDamage(!ZBGodMode);
	};

	bool ZBIsInvisible() {
		return m_ZBInvisible;
	}

	//Server
	void ZBSetInvisible(bool invisible) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;
		if (invisible == m_ZBInvisible) return;

		if (invisible) {
			m_ZBGodBeforeInvisible = ZBGodMode;
			ZBSetGod(true);
			m_ZBInvisible = true;
		} else {
			m_ZBInvisible = false;
			ZBSetGod(m_ZBGodBeforeInvisible);
		}
		SetSynchDirty();
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

	//The body, everything attached to it (clothing, their attachments) and the item in hands
	protected void ZBApplyInvisibility() {
		if (!ZBHidesHere()) return;

		ZBSetTreeInvisible(this, m_ZBInvisible);
		EntityAI inHands = GetHumanInventory().GetEntityInHands();
		if (inHands) ZBSetTreeInvisible(inHands, m_ZBInvisible);
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

	override void EEItemIntoHands(EntityAI item) {
		super.EEItemIntoHands(item);
		if (m_ZBInvisible && ZBHidesHere()) ZBSetTreeInvisible(item, true);
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

	override bool CanBeTargetedByAI(EntityAI ai) {
		if (m_ZBInvisible) return false;
		return super.CanBeTargetedByAI(ai);
	}
};
