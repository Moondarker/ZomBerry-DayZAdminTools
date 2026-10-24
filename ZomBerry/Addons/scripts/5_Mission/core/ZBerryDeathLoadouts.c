//What players wore when they died: the last MAX_DEATHS per Steam64 id, in memory only (gone after a restart).
//"Restore to a previous loadout slot" steps through them: 1 death ago, 2 deaths ago... and after the oldest back to
//what the player wore before the first step. A new death starts over
class ZBerryDeathLoadouts {
	static const int MAX_DEATHS = 5;

	protected ZBerryAuth m_Auth;
	protected ref map<string, ref ZBerryLoadoutArray> m_Deaths = new map<string, ref ZBerryLoadoutArray>; //Newest first
	protected ref map<string, ref ZBerryLoadout> m_Before = new map<string, ref ZBerryLoadout>; //Gear before the first step
	protected ref map<string, int> m_Slots = new map<string, int>; //Deaths ago the player now wears, while stepping

	void ZBerryDeathLoadouts(ZBerryAuth auth) {
		m_Auth = auth;
		ZBerryPlayerFeed.OnPlayerDying.Insert(OnPlayerDying);
	}

	void ~ZBerryDeathLoadouts() {
		ZBerryPlayerFeed.OnPlayerDying.Remove(OnPlayerDying);
	}

	//"" without one (AI and characters without a player)
	string GetKey(PlayerBase player) {
		if (!GetGame().IsMultiplayer()) return "singleplayer";
		PlayerIdentity identity = ZBGetIdentity(player);
		if (!identity) return "";
		return identity.GetPlainId();
	}

	//Before vanilla's death handling: the hotbar is still set
	void OnPlayerDying(PlayerBase victim, Object killer) {
		if (!GetGame().IsServer() || !victim) return;

		string key = GetKey(victim);
		if (key != "") Remember(key, ZBerryLoadoutKit.Capture(victim, "death"));
	}

	void Remember(string key, ZBerryLoadout loadout) {
		ZBerryLoadoutArray deaths = m_Deaths.Get(key);
		if (!deaths) {
			deaths = new ZBerryLoadoutArray;
			m_Deaths.Set(key, deaths);
		}
		deaths.InsertAt(loadout, 0);
		while (deaths.Count() > MAX_DEATHS) deaths.Remove(deaths.Count() - 1);

		m_Before.Remove(key); //Stepping starts over from the gear of the new life
		m_Slots.Remove(key);
	}

	int CountDeaths(string key) {
		ZBerryLoadoutArray deaths = m_Deaths.Get(key);
		if (!deaths) return 0;
		return deaths.Count();
	}

	//Puts the next slot on the target: returns it (1: the last death, 0: the gear from before the first step), -1 when
	//there's nothing to step through. Death gear follows the admin's spawn rules, the gear from before doesn't: it was
	//the target's own a moment ago
	int Step(PlayerBase target, PlayerIdentity admin, out ZBerryLoadoutJob job) {
		string key = GetKey(target);
		int count = CountDeaths(key);
		if (key == "" || count == 0) return -1;

		int slot = 1;
		if (m_Slots.Contains(key)) {
			slot = (m_Slots.Get(key) + 1) % (count + 1);
		} else {
			m_Before.Set(key, ZBerryLoadoutKit.Capture(target, "before"));
		}

		if (slot == 0) {
			ZBerryLoadout before = m_Before.Get(key);
			m_Before.Remove(key); //The next step saves the gear worn then
			m_Slots.Remove(key);
			job = ZBerryLoadoutJob.Start(target, before, NULL, admin);
			return 0;
		}

		m_Slots.Set(key, slot);
		job = ZBerryLoadoutJob.Start(target, m_Deaths.Get(key)[slot - 1], m_Auth, admin);
		return slot;
	}
};

static ref ZBerryDeathLoadouts g_ZBerryDeathLoadouts; //Server and singleplayer
