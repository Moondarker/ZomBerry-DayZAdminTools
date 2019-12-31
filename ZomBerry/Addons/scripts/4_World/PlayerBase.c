modded class PlayerBase
{
	private bool ZBGodMode;

	bool ZBIsGod() {
		return ZBGodMode;
	};

	void ZBSetGod(bool isGod) {
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) return;

		ZBGodMode = isGod;
		SetAllowDamage(!ZBGodMode);
	};
};
