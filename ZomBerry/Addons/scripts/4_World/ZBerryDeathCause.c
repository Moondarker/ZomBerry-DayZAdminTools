//Why a player died, in words, for the admin log's kill events (server side). Kills by another player are worded by the
//log itself; this covers everything else: infected, animals, vehicles without a driver, explosions, suicide, falls,
//drowning, fire, barbed wire, bleeding, poisoning, thirst and hunger. Game names are ZBerryLocText tags
class ZBerryDeathCause {
	static const int RECENT_MS = 10000; //A hit this recent explains a death without a killer (a fall, fire)
	static const int CONTEXT_MS = 300000; //One this recent is mentioned after slower causes ("bled out, last hurt by...")

	//Goes after "{target}": " was killed by an infected", " died from a fall", " bled out, last hurt by a Grey Wolf 40 s earlier"
	static string Describe(PlayerBase victim, Object killer) {
		if (!victim) return " died";
		if (victim.ZBGetDeathNote() != "") return victim.ZBGetDeathNote();

		EntityAI killerEntity = EntityAI.Cast(killer);
		if (killerEntity && killerEntity != victim) {
			string name = DescribeSource(killerEntity);
			if (killerEntity.IsInherited(ExplosivesBase)) return " was killed by an explosion (" + ZBerryLocText.Name(killerEntity) + ")";
			if (killerEntity.IsInherited(Transport)) return " was run over by " + name;
			if (killerEntity.GetHierarchyRootPlayer() == victim) return " killed themselves with " + ZBerryLocText.Name(killerEntity);
			return " was killed by " + name;
		}

		//No killer: whatever hurt them last, then their state
		int age = 0;
		int damageType = 0;
		string ammo = "";
		string hitBy = "";
		bool hit = victim.ZBGetLastHit(age, damageType, ammo, hitBy);
		bool recent = hit && age <= RECENT_MS;
		//Drowning before falls: it drains health directly, with no hit, so a fall just before (jumping in) was the last hit
		if (victim.m_IsDrowning || victim.GetDrowningWaterLevelCheck()) return " drowned";
		if (recent && IsFall(ammo)) return " died from a fall";
		if (recent && damageType == DamageType.EXPLOSION) return " died in an explosion (" + hitBy + ")";
		if (recent && ammo == "FireDamage") return " burned to death";
		if (recent && ammo == "BarbedWireHit") return " died on barbed wire";

		string context = "";
		if (hit && age <= CONTEXT_MS) context = ", last hurt by " + hitBy + " " + FormatAge(age) + " earlier";
		if (victim.GetBleedingManagerServer() && victim.GetBleedingManagerServer().GetBleedingSourcesCount() > 0) return " bled out" + context;
		if (victim.GetSingleAgentCount(eAgents.CHEMICAL_POISON) > 0) return " died of chemical poisoning" + context;
		if (victim.GetStatWater() && victim.GetStatWater().Get() <= 0) return " died of thirst" + context;
		if (victim.GetStatEnergy() && victim.GetStatEnergy().Get() <= 0) return " starved to death" + context;
		if (recent) return " died from a hit by " + hitBy;

		return " died (" + DescribeStats(victim) + ")" + context;
	}

	//"an infected", "a Brown Bear", "a fall", or an object's name
	static string DescribeSource(EntityAI source, string ammo = "") {
		if (IsFall(ammo)) return "a fall";
		if (ammo == "FireDamage") return "fire";
		if (ammo == "BarbedWireHit") return "barbed wire";
		if (!source) return "something";
		if (source.IsInherited(ZombieBase)) return "an infected";

		string name = ZBerryLocText.Name(source);
		if (source.IsInherited(AnimalBase) || source.IsInherited(Transport)) return "a " + name;
		return name;
	}

	static bool IsFall(string ammo) {
		return ammo == DayZPlayerImplementFallDamage.FALL_DAMAGE_AMMO_HEALTH || ammo == DayZPlayerImplementFallDamage.FALL_DAMAGE_AMMO_SHOCK || ammo == DayZPlayerImplementFallDamage.FALL_DAMAGE_AMMO_HEALTH_ATTACHMENT || ammo == DayZPlayerImplementFallDamage.FALL_DAMAGE_AMMO_HEALTH_OTHER_ATTACHMENTS;
	}

	//"blood 2400, water 1200, energy 300", what a death without a clear cause looked like
	static string DescribeStats(PlayerBase victim) {
		string stats = "blood " + Math.Round(victim.GetHealth("GlobalHealth", "Blood"));
		if (victim.GetStatWater()) stats += ", water " + Math.Round(victim.GetStatWater().Get());
		if (victim.GetStatEnergy()) stats += ", energy " + Math.Round(victim.GetStatEnergy().Get());
		return stats;
	}

	static string FormatAge(int ms) {
		int seconds = ms / 1000;
		if (seconds < 120) return seconds.ToString() + " s";
		int minutes = seconds / 60;
		return minutes.ToString() + " min";
	}
};

//The last hit each player took, and a note for deaths the engine causes on purpose
modded class PlayerBase {
	protected int m_ZBLastHitMs = -1; //GetGame().GetTime(), -1 for never
	protected int m_ZBLastHitType;
	protected string m_ZBLastHitAmmo;
	protected string m_ZBLastHitBy;
	protected string m_ZBDeathNote = "";

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef) {
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer()) return;

		m_ZBLastHitMs = GetGame().GetTime();
		m_ZBLastHitType = damageType;
		m_ZBLastHitAmmo = ammo;
		m_ZBLastHitBy = ZBerryDeathCause.DescribeSource(source, ammo);
	}

	//False when they were never hit. ageMs: how long ago
	bool ZBGetLastHit(out int ageMs, out int damageType, out string ammo, out string hitBy) {
		if (m_ZBLastHitMs < 0) return false;

		ageMs = GetGame().GetTime() - m_ZBLastHitMs;
		damageType = m_ZBLastHitType;
		ammo = m_ZBLastHitAmmo;
		hitBy = m_ZBLastHitBy;
		return true;
	}

	//Server, just before the engine kills them (respawning or logging out while unconscious or restrained): the cause
	//the log gives instead of guessing, after "{target}"
	void ZBSetDeathNote(string note) {
		m_ZBDeathNote = note;
	}

	string ZBGetDeathNote() {
		return m_ZBDeathNote;
	}
};
