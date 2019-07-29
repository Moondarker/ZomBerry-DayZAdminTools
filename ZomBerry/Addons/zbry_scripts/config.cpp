class CfgPatches
{
	class ZomBerry_Scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"JM_CF_Scripts",
			"ZomBerry_GUI"
		};
	};
};

class CfgMods
{
	class ZomBerry_AT
	{
		dir = "ZomBerry";
		picture = "set:zomberry_images image:ZBMenuLogo";
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

		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class imageSets
			{
				files[] = {"ZomBerry/AT/gui/imagesets/zomberry_images.imageset"};
			};
			class gameScriptModule
			{
				value = "";
				files[] = {"ZomBerry/AT/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"ZomBerry/AT/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"ZomBerry/AT/scripts/5_Mission"};
			};
		};
	};
};

// <3 VanillaPlusPlus
class RscMapControl
{
	scaleMin=0.001;
	scaleMax=0.95;
	scaleDefault=0.94;
	ptsPerSquareSea=8;
	ptsPerSquareTxt=10;
	ptsPerSquareCLn=10;
	ptsPerSquareExp=10;
	ptsPerSquareCost=10;
	ptsPerSquareFor=9;
	ptsPerSquareForEdge=15;
	ptsPerSquareRoad=4;
	ptsPerSquareObj=15;
	maxSatelliteAlpha=1;
	alphaFadeStartScale=1;
	alphaFadeEndScale=1;
	userMapPath="dz\gear\navigation\data\usermap";
	maxUserMapAlpha=0;
	alphaUserMapFadeStartScale=0.14;
	alphaUserMapFadeEndScale=0.14;
	showCountourInterval=0;
	colorLevels[]={0.64999998,0.60000002,0.44999999,0.94999999};
	colorSea[]={0.63999999,0.75999999,0.88999999,0.80000001};
	colorForest[]={0.36000001,0.77999997,0.079999998,0};
	colorRocks[]={0.5,0.5,0.5,0.5};
	colorCountlines[]={0.85000002,0.80000001,0.64999998,0};
	colorMainCountlines[]={0.44999999,0.40000001,0.25,0};
	colorCountlinesWater[]={0.25,0.40000001,0.5,0.30000001};
	colorMainCountlinesWater[]={0.25,0.40000001,0.5,0.89999998};
	colorPowerLines[]={0.1,0.1,0.1,1};
	colorRailWay[]={0.80000001,0.2,0,1};
	colorForestBorder[]={0.40000001,0.80000001,0,0};
	colorRocksBorder[]={0.5,0.5,0.5,0};
	colorOutside[]={1,1,1,1};
	colorTracks[]={0.77999997,0.66000003,0.34,1};
	colorRoads[]={0.69,0.43000001,0.23,1};
	colorMainRoads[]={0.52999997,0.34999999,0,1};
	colorTracksFill[]={0.95999998,0.91000003,0.60000002,1};
	colorRoadsFill[]={0.92000002,0.73000002,0.41,1};
	colorMainRoadsFill[]={0.83999997,0.61000001,0.20999999,1};
	colorGrid[]={0.80000001,0.69999999,0,1};
	colorGridMap[]={1,1,1,1};
	fontNames="gui/fonts/MetronBlack16";
	sizeExNames=0.050000001;
	colorNames[]={1,1,1,1};
	fontGrid="gui/fonts/MetronBlack48";
	sizeExGrid=0.02;
	fontLevel="gui/fonts/MetronLight-Italic28";
	sizeExLevel=0.0099999998;
	colorMountPoint[]={0.44999999,0.40000001,0.25,1};
	mapPointDensity=5.5;
	text="";
	fontLabel="gui/fonts/MetronLight-Italic28";
	fontInfo="gui/fonts/MetronLight-Italic28";
	class Legend
	{
		x=0.040000003;
		y=0.85000002;
		w=0.30000001;
		h=0.1;
		font="gui/fonts/MetronLight-Italic28";
		sizeEx=0.02;
		colorBackground[]={1,1,1,0.30000001};
		color[]={46,201,29,1};
	};
	class Bush
	{
		icon="\dz\gear\navigation\data\map_bush_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=14;
		importance="0.2 * 14 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class SmallTree
	{
		icon="\dz\gear\navigation\data\map_smalltree_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=12;
		importance="0.6 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Tree
	{
		icon="\dz\gear\navigation\data\map_tree_ca.paa";
		color[]={0.40000001,0.80000001,0,0};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rock
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={0.1,0.1,0.1,0.80000001};
		size=12;
		importance="0.5 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fuelstation
	{
		icon="\dz\gear\navigation\data\map_fuelstation_ca.paa";
		size=16;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.75;
		coefMax=4;
	};
	class Lighthouse
	{
		icon="\dz\gear\navigation\data\map_lighthouse_ca.paa";
		size=14;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Stack
	{
		icon="\dz\gear\navigation\data\map_stack_ca.paa";
		size=16;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Transmitter
	{
		icon="\dz\gear\navigation\data\map_transmitter_ca.paa";
		color[]={255,255,255,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Watertower
	{
		icon="\dz\gear\navigation\data\map_watertower_ca.paa";
		color[]={255,255,255,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Shipwreck
	{
		icon="\dz\gear\navigation\data\map_shipwreck_ca.paa";
		color[]={255,255,255,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Monument
	{
		icon="\dz\gear\navigation\data\map_monument_ca.paa";
		color[]={255,255,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class BusStop
	{
		icon="\dz\gear\navigation\data\map_busstop_ca.paa";
		color[]={255,255,255,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fountain
	{
		icon="\dz\gear\navigation\data\map_fountain_ca.paa";
		color[]={0,0,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Waterpump
	{
		icon="\dz\gear\navigation\data\map_waterpump_ca.paa";
		color[]={255,255,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Hospital
	{
		icon="\dz\gear\navigation\data\map_hospital_ca.paa";
		color[]={255,255,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Store
	{
		icon="\dz\gear\navigation\data\map_store_ca.paa";
		color[]={255,255,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Police
	{
		icon="\dz\gear\navigation\data\map_police_ca.paa";
		color[]={255,255,255,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class FireDep
	{
		icon="\dz\gear\navigation\data\map_firedep_ca.paa";
		color[]={255,255,255,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class TouristSign
	{
		icon="\dz\gear\navigation\data\map_tsign_ca.paa";
		color[]={255,255,255,1};
		size=15;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Tourism
	{
		icon="\dz\gear\navigation\data\map_tourism_ca.paa";
		color[]={255,255,255,1};
		size=15;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Cross
	{
		icon="\dz\gear\navigation\data\map_cross_ca.paa";
		size=20;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Chapel
	{
		icon="\dz\gear\navigation\data\map_chapel_ca.paa";
		color[]={0,0,255,1};
		size=18;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Church
	{
		icon="\dz\gear\navigation\data\map_church_ca.paa";
		color[]={255,0,0,1};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class ViewTower
	{
		icon="\dz\gear\navigation\data\map_viewtower_ca.paa";
		color[]={255,255,255,1};
		size=22;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class TouristShelter
	{
		icon="\dz\gear\navigation\data\map_tshelter_ca.paa";
		color[]={255,255,255,1};
		size=18;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Quay
	{
		icon="\dz\gear\navigation\data\map_quay_ca.paa";
		size=16;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Bunker
	{
		icon="\dz\gear\navigation\data\map_bunker_ca.paa";
		color[]={255,255,255,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fortress
	{
		icon="\dz\gear\navigation\data\map_fortress_ca.paa";
		size=20;
		color[]={255,255,255,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rocks
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={255,255,255,1};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Ruin
	{
		icon="\dz\gear\navigation\data\map_ruin_ca.paa";
		size=16;
		color[]={255,255,255,1};
		importance="1.2 * 16 * 0.05";
		coefMin=1;
		coefMax=4;
	};
	class CfgLocationTypes
	{
		class Name
		{
			name="keypoint";
			drawStyle="name";
			texture="#(argb,1,1,1)color(1,1,1,1)";
			color[]={255,255,255,1};
			size=1;
			font="gui/fonts/MetronLight-Italic28";
			textSize=0.029999999;
			shadow=0;
			importance=1;
		};
		class NameIcon
		{
			name="keypoint";
			drawStyle="icon";
			texture="#(argb,1,1,1)color(1,1,1,1)";
			color[]={255,255,255,1};
			size=1;
			font="gui/fonts/MetronLight-Italic28";
			textSize=0.029999999;
			shadow=0;
			importance=1;
		};
		class Capital: Name
		{
			textSize=0.25999999;
			importance=14;
		};
		class City: Name
		{
			textSize=0.25999999;
			importance=12;
		};
		class Village: Name
		{
			textSize=0.25999999;
			importance=10;
		};
		class Local: Name
		{
			color[]={255,255,255,1};
			textSize=0.25999999;
			importance=4;
		};
		class Marine: Name
		{
			color[]={255,255,255,1};
			textSize=0.029999999;
			importance=4;
		};
		class Ruin: NameIcon
		{
			texture="\DZ\gear\navigation\data\map_ruin_ca.paa";
			textSize=0.029999999;
			importance=4;
			size=7;
		};
		class Camp: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_camp_ca.paa";
			textSize=0.039999999;
			size=9;
		};
		class Hill: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_hill_ca.paa";
			size=9;
		};
		class ViewPoint: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_viewpoint_ca.paa";
			size=9;
		};
		class RockArea: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_rock_ca.paa";
			size=9;
		};
		class RailroadStation: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_station_ca.paa";
			size=9;
		};
		class IndustrialSite: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_factory_ca.paa";
			size=9;
		};
		class LocalOffice: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_govoffice_ca.paa";
			size=10;
		};
		class BorderCrossing: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_border_cross_ca.paa";
			size=9;
		};
		class VegetationBroadleaf: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_broadleaf_ca.paa";
			size=9;
		};
		class VegetationFir: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_fir_ca.paa";
			size=9;
		};
		class VegetationPalm: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_palm_ca.paa";
			size=9;
		};
		class VegetationVineyard: NameIcon
		{
			color[]={255,255,255,1};
			texture="\DZ\gear\navigation\data\map_vineyard_ca.paa";
			size=9;
		};
	};
};
