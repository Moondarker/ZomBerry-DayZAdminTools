class ZomberryMenu extends UIScriptedMenu {
	
	override Widget Init() {
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "ZomBerry\\AT\\gui\\layouts\\AdminDialog.layout" );

		return layoutRoot;
	}

	void OnKeyPress(int key) {

	}

	bool GetCloseClearance() {
		return true;
	}
}
