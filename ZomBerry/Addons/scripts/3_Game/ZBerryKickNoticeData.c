//Client: why the server kicked or banned this player (ZBerryBans.SendKickNotice). It can arrive while the game is still
//loading into the server, and has to outlast leaving it, so it's received and kept here, by the game itself rather
//than the mission. The main menu shows it (5_Mission, ZBerryKickNotice)
class ZBerryKickNoticeData {
	protected static bool s_Pending;
	protected static bool s_Banned;
	protected static string s_Reason;
	protected static int s_Until; //Ban end (seconds since 1970, UTC), 0 permanent
	protected static int s_Remaining; //Seconds of the ban left when it was sent

	void OnKickNotice( ZBerryCallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		Param4<bool, string, int, int> notice; //Banned, reason, ban end, seconds left
		if ( !ctx.Read( notice ) ) return;
		if (type != ZBerryCallType.Client) return;

		s_Pending = true;
		s_Banned = notice.param1;
		s_Reason = notice.param2;
		s_Until = notice.param3;
		s_Remaining = notice.param4;
	}

	//Hands the notice over once: false when there's none
	static bool Take(out bool banned, out string reason, out int until, out int remaining) {
		if (!s_Pending) return false;

		s_Pending = false;
		banned = s_Banned;
		reason = s_Reason;
		until = s_Until;
		remaining = s_Remaining;
		return true;
	}

	static bool IsPending() {
		return s_Pending;
	}
};

modded class DayZGame {
	protected ref ZBerryKickNoticeData m_ZBerryKickNotice;

	void DayZGame() {
		m_ZBerryKickNotice = new ZBerryKickNoticeData();
		GetZBerryRPC().AddRPC( ZBerryRPCOp.KickNotice, "OnKickNotice", m_ZBerryKickNotice, ZBerrySPExec.Client, true );
	}
};
