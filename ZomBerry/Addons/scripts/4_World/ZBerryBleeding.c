//God mode and invisibility keep new bleeding sources off: cuts from walking barefoot or in ruined shoes, emotes,
//crafting and other actions add them directly, past the damage switch that keeps hits off. Server
modded class BleedingSourcesManagerServer {
	override protected bool CanAddBleedingSource(int bit) {
		if (m_Player && m_Player.ZBProtected()) return false;
		return super.CanAddBleedingSource(bit);
	}
};
