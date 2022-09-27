class CfgPatches
{
	class zomberry_scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"RPC_Scripts"
		};
	};
};

class CfgMods
{
	class ZomBerry_AT
	{
		dir = "ZomBerry";
		picture = "set:zomberry_images image:ZBMenuLogo";
		inputs = "ZomBerry/AT/scripts/data/inputs.xml";
		action = "";
		hideName = 0;
		hidePicture = 0;
		name = "ZomBerry Admin Tools";
		credits = "Vaker";
		author = "Vaker";
		authorID = "0";
		version = "0.5";
		extra = 0;
		type = "mod";

		dependencies[] = {"World", "Mission"};

		class defs
		{
			class imageSets
			{
				files[] = {"ZomBerry/AT/scripts/imagesets/zomberry_images.imageset"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {
					"ZomBerry/AT/scripts/Common",
					"ZomBerry/AT/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {
					"ZomBerry/AT/scripts/Common",
					"ZomBerry/AT/scripts/5_Mission"
				};
			};
		};
	};
};