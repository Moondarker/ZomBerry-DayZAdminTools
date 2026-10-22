modded class Weapon_Base
{
	//Infinite ammo (PlayerBase.ZBRefillAfterShot)
	override void EEFired(int muzzleType, int mode, string ammoType) {
		super.EEFired(muzzleType, mode, ammoType);

		PlayerBase shooter = PlayerBase.Cast(GetHierarchyRootPlayer());
		if (shooter) shooter.ZBRefillAfterShot(this);
	}
};
