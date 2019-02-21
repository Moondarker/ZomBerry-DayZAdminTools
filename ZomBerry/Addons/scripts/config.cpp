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
		picture = "";
		action = "";
		hideName = 0;
		hidePicture = 1;
		name = "ZomBerry Admin Tools";
		credits = "Vaker";
		author = "Vaker";
		authorID = "0";
		version = "0.5";
		extra = 0;
		type = "mod";

		dependencies[] = {"Mission"};

		class defs
		{
			class missionScriptModule
			{
				value = "";
				files[] = {"ZomBerry/AT/scripts/5_Mission"};
			};
		}
	};
};

class RscMapControl
{
	scaleMin=0.001;
	scaleMax=0.95;
	ptsPerSquareSea=6;
	ptsPerSquareTxt=8;
	ptsPerSquareCLn=8;
	ptsPerSquareExp=8;
	ptsPerSquareCost=8;
	ptsPerSquareFor=4;
	ptsPerSquareForEdge=10;
	ptsPerSquareRoad=2;
	ptsPerSquareObj=10;
	maxSatelliteAlpha=0;
	alphaFadeStartScale=1;
	alphaFadeEndScale=1;
	userMapPath="dz\gear\navigation\data\usermap";
	maxUserMapAlpha=1;
	alphaUserMapFadeStartScale=0.32;
	alphaUserMapFadeEndScale=0.32;
	showCountourInterval=1;
	colorLevels[]={0.65,0.6,0.45,0.95};
	colorSea[]={0.651,0.827,0.92,0.8};
	colorForest[]={0.36,0.78,0.08,0};
	colorRocks[]={0.5,0.5,0.5,0.5};
	colorCountlines[]={0.85,0.8,0.65,1};
	colorMainCountlines[]={0.45,0.4,0.25,1};
	colorCountlinesWater[]={0.25,0.4,0.5,0.3};
	colorMainCountlinesWater[]={0.25,0.4,0.5,0.9};
	colorPowerLines[]={0.1,0.1,0.1,1};
	colorRailWay[]={0.8,0.2,0,1};
	colorForestBorder[]={0.4,0.8,0,1};
	colorRocksBorder[]={0.5,0.5,0.5,1};
	colorOutside[]={1,1,1,1};
	colorTracks[]={0.78,0.66,0.34,1};
	colorRoads[]={0.69,0.43,0.23,1};
	colorMainRoads[]={0.53,0.35,0,1};
	colorTracksFill[]={0.96,0.91,0.6,1};
	colorRoadsFill[]={0.92,0.73,0.41,1};
	colorMainRoadsFill[]={0.84,0.61,0.2,1};
	colorGrid[]={0.15,0.15,0.05,0.9};
	colorGridMap[]={0.25,0.25,0.1,0.75};
	fontNames="gui/fonts/Metron-Italic48";
	sizeExNames=0.06;
	colorNames[]={255,255,255,1};
	fontGrid="gui/fonts/Metron-Italic28";
	sizeExGrid=0.022;
	fontLevel="gui/fonts/MetronLight-Italic28";
	sizeExLevel=0.01;
	colorMountPoint[]={0.45,0.4,0.25,1};
	mapPointDensity=1.5;
	text="";
	fontLabel="gui/fonts/MetronLight-Italic28";
	fontInfo="gui/fonts/MetronLight-Italic28";
};
