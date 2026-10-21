//Icon of ZomBerry's message notifications (ZBerryMessages). 3_Game, because NotificationUI lives there
const string ZBERRY_NOTIFICATION_ICON = "set:dayz_inventory image:walkietalkie";

//Vanilla notifications stretch their icon over a square slot. The walkie-talkie is drawn in the upper left of its
//imageset cell, so in one-line notifications it looked off-centre. ZomBerry's notifications get the icon as a child
//of the slot, moved to the slot's centre; the slot's own image is blanked. Other notifications are left alone
modded class NotificationUI {
	override void AddNotification(NotificationRuntimeData data) {
		super.AddNotification(data);
		if (!data || data.GetIcon() != ZBERRY_NOTIFICATION_ICON) return;

		Widget notification = m_Notifications.Get(data);
		if (!notification) return;
		ImageWidget slot = ImageWidget.Cast( notification.FindAnyWidget("Image") );
		if (!slot) return;

		Widget icon = GetGame().GetWorkspace().CreateWidgets("ZomBerry/AT/scripts/5_Mission/layouts/NotificationIcon.layout", slot);
		if (!icon) return;
		slot.LoadImageFile(0, "set:dayz_gui image:alpha_0"); //A transparent pixel: the slot keeps its size and fading
	}
};
