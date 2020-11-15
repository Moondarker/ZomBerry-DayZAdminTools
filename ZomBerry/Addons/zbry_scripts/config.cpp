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

		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class imageSets
			{
				files[] = {"ZomBerry/AT/gui/imagesets/ZB_menuLogo.imageset"};
			};
			class gameScriptModule
			{
				value = "";
				files[] = {
					"ZomBerry/AT/scripts/common",
					"ZomBerry/AT/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {
					"ZomBerry/AT/scripts/common",
					"ZomBerry/AT/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {
					"ZomBerry/AT/scripts/common",
					"ZomBerry/AT/scripts/5_Mission"
				};
			};
		};
	};
};

// VanillaPlusPlus Map by DaOne and GravityWolf <3
// T136977 was "Fixed", but instead of game crash we now get a COMPLETELY EMPTY map
// In order to make UX of VPP and ZBerry users as good as possible, I decided to
// Keep this config until we get a (completely) vanilla map back.

class RscMapControl
{
	scaleMin=0.05;
	scaleMax=0.85;
	scaleDefault=0.85;
	ptsPerSquareSea=8;
	ptsPerSquareTxt=10;
	ptsPerSquareCLn=10;
	ptsPerSquareExp=10;
	ptsPerSquareCost=10;
	ptsPerSquareFor=99;
	ptsPerSquareForEdge=99;
	ptsPerSquareRoad=4;
	ptsPerSquareObj=15;
	maxSatelliteAlpha=1;
	alphaFadeStartScale=1;
	alphaFadeEndScale=1;
	userMapPath="dz\gear\navigation\data\usermap";
	maxUserMapAlpha=0.2;
	alphaUserMapFadeStartScale=0.5;
	alphaUserMapFadeEndScale=0.8;
	showCountourInterval=1;
	colorLevels[]={0.64999998,0.60000002,0.44999999,0.3};
	colorSea[]={0.2,0.5,0.7,1};
	colorForest[]={0.36000001,0.77999997,0.079999998,0};
	colorRocks[]={0.5,0.5,0.5,0.2};
	colorCountlines[]={0.85000002,0.80000001,0.64999998,0.1};
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
	colorGrid[]={0.9,0.9,0.9,0.8};
	colorGridMap[]={0.9,0.9,0.9,0.2};
	fontNames="gui/fonts/sdf_MetronBook24";
	sizeExNames=0.03;
	colorNames[]={1,1,1,1};
	fontGrid="gui/fonts/sdf_MetronBook24";
	sizeExGrid=0.02;
	fontLevel="gui/fonts/sdf_MetronBook24";
	sizeExLevel=0.01;
	colorMountPoint[]={0.44999999,0.40000001,0.25,0};
	mapPointDensity=0.12;
	text="";
	fontLabel="gui/fonts/sdf_MetronBook24";
	fontInfo="gui/fonts/sdf_MetronBook24";
	class Legend
	{
		x=0.050000001;
		y=0.85000002;
		w=0.40000001;
		h=0.1;
		font="gui/fonts/sdf_MetronBook24";
		sizeEx=0.02;
		colorBackground[]={1,1,1,0};
		color[]={0,0,0,0};
	};
	class Bush
	{
		icon="\dz\gear\navigation\data\map_bush_ca.paa";
		color[]={0.4,0.8,0,0};
		size=14;
		importance="0.2 * 14 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class SmallTree
	{
		icon="\dz\gear\navigation\data\map_smalltree_ca.paa";
		color[]={0.4,0.8,0,0};
		size=12;
		importance="0.6 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Tree
	{
		icon="\dz\gear\navigation\data\map_tree_ca.paa";
		color[]={0.4,0.8,0,0};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rock
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={0.1,0.1,0.1,0.8};
		size=12;
		importance="0.5 * 12 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fuelstation
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\fuelstation.paa";
		size=16;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.75;
		coefMax=2;
	};
	class Lighthouse
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\lighthouse.paa";
		size=30;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=1;
		coefMax=4;
	};
	class Stack
	{
		icon="\dz\gear\navigation\data\map_stack_ca.paa";
		size=16;
		color[]={1,1,1,0};
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Transmitter
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\radiotower.paa";
		color[]={1,1,1,1};
		size=30;
		importance="0.9 * 16 * 0.05";
		coefMin=0.7;
		coefMax=4;
	};
	class Watertower
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\watertower.paa";
		color[]={1,1,1,1};
		size=30;
		importance="0.9 * 16 * 0.05";
		coefMin=0.7;
		coefMax=4;
	};
	class Shipwreck
	{
		icon="\dz\gear\navigation\data\map_shipwreck_ca.paa";
		color[]={1,1,1,1};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.89999998;
		coefMax=4;
	};
	class Monument
	{
		icon="\dz\gear\navigation\data\map_monument_ca.paa";
		color[]={1,1,1,0};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class BusStop
	{
		icon="\dz\gear\navigation\data\map_busstop_ca.paa";
		color[]={1,1,1,0};
		size=16;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fountain
	{
		icon="\dz\gear\navigation\data\map_fountain_ca.paa";
		color[]={1,1,1,0};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Waterpump
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\waterpump.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Hospital
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\hospital.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Store
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\shopping.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Police
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\police.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class FireDep
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\firestation.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class TouristSign
	{
		icon="\dz\gear\navigation\data\map_tsign_ca.paa";
		color[]={1,1,1,0};
		size=15;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Tourism
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\bar.paa";
		color[]={1,1,1,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class Cross
	{
		icon="\dz\gear\navigation\data\map_cross_ca.paa";
		size=20;
		color[]={1,1,0,0};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Chapel
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\church.paa";
		color[]={1,1,1,1};
		size=25;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=2;
	};
	class Church
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\church.paa";
		color[]={1,1,1,1};
		size=35;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=2;
	};
	class ViewTower
	{
		icon="\VanillaPPMap\GUI\Textures\CustomMapIcons\watchtower.paa";
		color[]={1,1,1,1};
		size=22;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=2;
	};
	class TouristShelter
	{
		icon="\dz\gear\navigation\data\map_tshelter_ca.paa";
		color[]={1,1,1,1};
		size=18;
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Quay
	{
		icon="\dz\gear\navigation\data\map_quay_ca.paa";
		size=16;
		color[]={1,1,1,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.5;
		coefMax=4;
	};
	class Bunker
	{
		icon="\dz\gear\navigation\data\map_bunker_ca.paa";
		color[]={0.6,0.6,0.6,1};
		size=20;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Fortress
	{
		icon="\dz\gear\navigation\data\map_fortress_ca.paa";
		size=20;
		color[]={0.6,0.6,0.6,1};
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Rocks
	{
		icon="\dz\gear\navigation\data\map_rock_ca.paa";
		color[]={0.6,0.6,0.6,1};
		size=12;
		importance="0.9 * 16 * 0.05";
		coefMin=0.25;
		coefMax=4;
	};
	class Ruin
	{
		icon="\dz\gear\navigation\data\map_ruin_ca.paa";
		size=16;
		color[]={0.6,0.6,0.6,1};
		importance="1.2 * 16 * 0.05";
		coefMin=1;
		coefMax=4;
	};
};
class CfgLocationTypes
{
	class Name
	{
		name="keypoint";
		drawStyle="name";
		texture="#(argb,1,1,1)color(1,1,1,1)";
		color[]={1,1,1,1};
		size=0;
		font="gui/fonts/sdf_MetronBook24";
		textSize=0.029999999;
		shadow=0.1;
		importance=1;
	};
	class NameIcon
	{
		name="keypoint";
		drawStyle="icon";
		texture="#(argb,1,1,1)color(1,1,1,1)";
		color[]={1,1,1,1};
		size=0;
		font="gui/fonts/sdf_MetronBook24";
		textSize=0.029999999;
		shadow=0;
		importance=1;
	};
	class Capital: Name
	{
		color[]={1,1,1,1};
		textSize=0.059999999;
		importance=7;
	};
	class BigMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.05;
		importance=6;
	};
	class City: Name
	{
		textSize=0.050000001;
		importance=6;
	};
	class MediumMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.04;
		importance=5;
	};
	class SmallMilitary: Name
	{
		color[]={1,0.1,0.1,1};
		textSize=0.035;
		importance=5;
	};
	class Village: Name
	{
		textSize=0.039999999;
		importance=3;
	};
	class Local: Name
	{
		color[]={0.44,0.38,0.30000001,1};
		textSize=0.029999999;
	};
	class Marine: Name
	{
		color[]={0.050000001,0.40000001,0.80000001,0.80000001};
		textSize=0.029999999;
	};
	class FeedShack: NameIcon
	{
		texture="\VanillaPPMap\GUI\Textures\CustomMapIcons\feedshack.paa";
		importance=1;
		size=10;
	};
	class Deerstand: NameIcon
	{
		texture="\VanillaPPMap\GUI\Textures\CustomMapIcons\deerstand.paa";
		importance=1;
		size=10;
	};
	class Ruin: NameIcon
	{
		texture="\DZ\gear\navigation\data\map_ruin_ca.paa";
		textSize=0.029999999;
		importance=2;
		size=7;
	};
	class Camp: NameIcon
	{
		color[]={0.44,0.38,0.30000001,1};
		texture="\DZ\gear\navigation\data\map_camp_ca.paa";
		textSize=0.02;
		size=5;
	};
	class Hill: NameIcon
	{
		color[]={0.7,0.7,0.7,1};
		texture="\DZ\gear\navigation\data\map_hill_ca.paa";
		textSize=0.02;
		size=3;
	};
	class ViewPoint: NameIcon
	{
		color[]={0.77999997,0,0.050000001,0};
		texture="\DZ\gear\navigation\data\map_viewpoint_ca.paa";
		size=9;
	};
	class RockArea: NameIcon
	{
		color[]={0,0,0,1};
		texture="\DZ\gear\navigation\data\map_rock_ca.paa";
		size=9;
	};
	class RailroadStation: NameIcon
	{
		color[]={0,0,0,0};
		texture="\DZ\gear\navigation\data\map_station_ca.paa";
		size=9;
	};
	class IndustrialSite: NameIcon
	{
		color[]={0,0,0,1};
		texture="\DZ\gear\navigation\data\map_factory_ca.paa";
		size=9;
	};
	class LocalOffice: NameIcon
	{
		color[]={0.22,0.20999999,0.50999999,0};
		texture="\DZ\gear\navigation\data\map_govoffice_ca.paa";
		size=10;
	};
	class BorderCrossing: NameIcon
	{
		color[]={0.77999997,0,0.050000001,1};
		texture="\DZ\gear\navigation\data\map_border_cross_ca.paa";
		size=9;
	};
	class VegetationBroadleaf: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_broadleaf_ca.paa";
		size=9;
	};
	class VegetationFir: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_fir_ca.paa";
		size=9;
	};
	class VegetationPalm: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_palm_ca.paa";
		size=9;
	};
	class VegetationVineyard: NameIcon
	{
		color[]={0.25,0.40000001,0.2,1};
		texture="\DZ\gear\navigation\data\map_vineyard_ca.paa";
		size=9;
	};
};
class VPPMapSettings: RscMapControl
{
	scaleMin=0.85000002;
	scaleMax=0.85000002;
	scaleDefault=0.85000002;
};
class CfgWorlds
{
	class CAWorld;
	class ChernarusPlus: CAWorld
	{
		class Names
		{
			class Settlement_Chernogorsk
			{
				name = "Chernogorsk";
				position[] = {
					6778.6191,
					2321.7739
				};
				type = "Capital";
			};
			class Settlement_Novodmitrovsk
			{
				name = "Novodmitrovsk";
				position[] = {
					11339.77,
					14391.05
				};
				type = "Capital";
			};
			class Settlement_Novoselki
			{
				name = "Novoselki";
				position[] = {
					6115.9102,
					3256.3601
				};
				type = "City";
			};
			class Settlement_Dubovo
			{
				name = "Dubovo";
				position[] = {
					6754.5098,
					3595.2571
				};
				type = "City";
			};
			class Settlement_Vysotovo
			{
				name = "Vysotovo";
				position[] = {
					5701.2402,
					2556.1101
				};
				type = "City";
			};
			class Settlement_Zelenogorsk
			{
				name = "Zelenogorsk";
				position[] = {
					2796,
					5166
				};
				type = "City";
			};
			class Settlement_Berezino
			{
				name = "Berezino";
				position[] = {
					12372.383,
					9760.9814
				};
				type = "City";
			};
			class Settlement_Elektrozavodsk
			{
				name = "Elektrozavodsk";
				position[] = {
					10286.68,
					1992.77
				};
				type = "City";
			};
			class Settlement_NovayaPetrovka
			{
				name = "Novaya Petrovka";
				position[] = {
					3477.1689,
					12931.243
				};
				type = "City";
			};
			class Settlement_Gorka
			{
				name = "Gorka";
				position[] = {
					9524,
					8945
				};
				type = "City";
			};
			class Settlement_Solnechny
			{
				name = "Solnechny";
				position[] = {
					13487.724,
					6225.8838
				};
				type = "City";
			};
			class Settlement_StarySobor
			{
				name = "Stary Sobor";
				position[] = {
					6114,
					7849
				};
				type = "City";
			};
			class Settlement_Vybor
			{
				name = "Vybor";
				position[] = {
					3909.8311,
					9019.6143
				};
				type = "City";
			};
			class Settlement_Severograd
			{
				name = "Severograd";
				position[] = {
					8168.4839,
					12573.602
				};
				type = "City";
			};
			class Settlement_Bor
			{
				name = "Bor";
				position[] = {
					3316.406,
					4070.9661
				};
				type = "Village";
			};
			class Settlement_Svetloyarsk
			{
				name = "Svetloyarsk";
				position[] = {
					13989.74,
					13263.78
				};
				type = "City";
			};
			class Settlement_Krasnostav
			{
				name = "Krasnostav";
				position[] = {
					11203.167,
					12340.808
				};
				type = "City";
			};
			class Settlement_ChernayaPolyana
			{
				name = "Chernaya Polyana";
				position[] = {
					12158.54,
					13770.22
				};
				type = "City";
			};
			class Settlement_Polyana
			{
				name = "Polyana";
				position[] = {
					10794,
					8160
				};
				type = "City";
			};
			class Settlement_Tulga
			{
				name = "Tulga";
				position[] = {
					12768,
					4384
				};
				type = "Village";
			};
			class Settlement_Msta
			{
				name = "Msta";
				position[] = {
					11333.79,
					5421.3999
				};
				type = "Village";
			};
			class Settlement_Staroye
			{
				name = "Staroye";
				position[] = {
					10212.52,
					5385.0498
				};
				type = "Village";
			};
			class Settlement_Shakhovka
			{
				name = "Shakhovka";
				position[] = {
					9728.6396,
					6567.2998
				};
				type = "Village";
			};
			class Settlement_Dolina
			{
				name = "Dolina";
				position[] = {
					11285.17,
					6633.5801
				};
				type = "Village";
			};
			class Settlement_Orlovets
			{
				name = "Orlovets";
				position[] = {
					12250,
					7253
				};
				type = "Village";
			};
			class Settlement_NovySobor
			{
				name = "Novy Sobor";
				position[] = {
					7123.6299,
					7774.6299
				};
				type = "Village";
			};
			class Settlement_Kabanino
			{
				name = "Kabanino";
				position[] = {
					5300.71,
					8645.9004
				};
				type = "Village";
			};
			class Settlement_Mogilevka
			{
				name = "Mogilevka";
				position[] = {
					7650.4102,
					5096.46
				};
				type = "Village";
			};
			class Settlement_Nadezhdino
			{
				name = "Nadezhdino";
				position[] = {
					5894,
					4780
				};
				type = "Village";
			};
			class Settlement_Guglovo
			{
				name = "Guglovo";
				position[] = {
					8445,
					6579
				};
				type = "Village";
			};
			class Settlement_Kamyshovo
			{
				name = "Kamyshovo";
				position[] = {
					12170,
					3458
				};
				type = "Village";
			};
			class Settlement_Pusta
			{
				name = "Pusta";
				position[] = {
					9197,
					3873
				};
				type = "Village";
			};
			class Settlement_Dubrovka
			{
				name = "Dubrovka";
				position[] = {
					10324.299,
					9888.2373
				};
				type = "Village";
			};
			class Settlement_VyshnayaDubrovka
			{
				name = "Vyshnaya Dubrovka";
				position[] = {
					9926.8232,
					10468.919
				};
				type = "Village";
			};
			class Settlement_Khelm
			{
				name = "Khelm";
				position[] = {
					12329.27,
					10771.01
				};
				type = "Village";
			};
			class Settlement_Olsha
			{
				name = "Olsha";
				position[] = {
					13402,
					12898
				};
				type = "Village";
			};
			class Settlement_Gvozdno
			{
				name = "Gvozdno";
				position[] = {
					8644.751,
					11934.115
				};
				type = "Village";
			};
			class Settlement_Grishino
			{
				name = "Grishino";
				position[] = {
					5980,
					10257
				};
				type = "Village";
			};
			class Settlement_Rogovo
			{
				name = "Rogovo";
				position[] = {
					4802,
					6748
				};
				type = "Village";
			};
			class Settlement_Pogorevka
			{
				name = "Pogorevka";
				position[] = {
					4445,
					6322
				};
				type = "Village";
			};
			class Settlement_Pustoshka
			{
				name = "Pustoshka";
				position[] = {
					3137.9441,
					7890.9248
				};
				type = "Village";
			};
			class Settlement_Kozlovka
			{
				name = "Kozlovka";
				position[] = {
					4402,
					4736
				};
				type = "Village";
			};
			class Settlement_Karmanovka
			{
				name = "Karmanovka";
				position[] = {
					12700.628,
					14691.312
				};
				type = "Village";
			};
			class Settlement_Balota
			{
				name = "Balota";
				position[] = {
					4527.5801,
					2465.3201
				};
				type = "Village";
			};
			class Settlement_Komarovo
			{
				name = "Komarovo";
				position[] = {
					3702,
					2440
				};
				type = "Village";
			};
			class Settlement_Kamenka
			{
				name = "Kamenka";
				position[] = {
					1981,
					2207
				};
				type = "Village";
			};
			class Settlement_Myshkino
			{
				name = "Myshkino";
				position[] = {
					2048,
					7258
				};
				type = "Village";
			};
			class Settlement_Pavlovo
			{
				name = "Pavlovo";
				position[] = {
					1693.98,
					3858.6499
				};
				type = "Village";
			};
			class Settlement_Lopatino
			{
				name = "Lopatino";
				position[] = {
					2801,
					10029
				};
				type = "Village";
			};
			class Settlement_Vyshnoye
			{
				name = "Vyshnoye";
				position[] = {
					6629,
					6051
				};
				type = "Village";
			};
			class Settlement_Prigorodki
			{
				name = "Prigorodki";
				position[] = {
					7760,
					3336
				};
				type = "Village";
			};
			class Settlement_Drozhino
			{
				name = "Drozhino";
				position[] = {
					3374,
					4861
				};
				type = "Village";
			};
			class Settlement_Sosnovka
			{
				name = "Sosnovka";
				position[] = {
					2556,
					6307
				};
				type = "Village";
			};
			class Settlement_Nizhneye
			{
				name = "Nizhneye";
				position[] = {
					12824.88,
					8097.7002
				};
				type = "Village";
			};
			class Settlement_Pulkovo
			{
				name = "Pulkovo";
				position[] = {
					5015,
					5607
				};
				type = "Village";
			};
			class Settlement_Berezhki
			{
				name = "Berezhki";
				position[] = {
					15020,
					13905
				};
				type = "Village";
			};
			class Settlement_Turovo
			{
				name = "Turovo";
				position[] = {
					13630,
					14146
				};
				type = "Village";
			};
			class Settlement_BelayaPolyana
			{
				name = "Belaya Polyana";
				position[] = {
					14166,
					14968
				};
				type = "Village";
			};
			class Settlement_Dobroye
			{
				name = "Dobroye";
				position[] = {
					12996,
					15066
				};
				type = "Village";
			};
			class Settlement_Nagornoye
			{
				name = "Nagornoye";
				position[] = {
					9286,
					14677
				};
				type = "Village";
			};
			class Settlement_Svergino
			{
				name = "Svergino";
				position[] = {
					9493.5195,
					13875.27
				};
				type = "Village";
			};
			class Settlement_Ratnoye
			{
				name = "Ratnoye";
				position[] = {
					6195.8779,
					12761.802
				};
				type = "Village";
			};
			class Settlement_Kamensk
			{
				name = "Kamensk";
				position[] = {
					6748.7412,
					14430.046
				};
				type = "Village";
			};
			class Settlement_Krasnoye
			{
				name = "Krasnoye";
				position[] = {
					6411.374,
					15029.961
				};
				type = "Village";
			};
			class Settlement_StaryYar
			{
				name = "StaryYar";
				position[] = {
					4987.8599,
					14992.976
				};
				type = "Village";
			};
			class Settlement_Polesovo
			{
				name = "Polesovo";
				position[] = {
					5955.7368,
					13558.046
				};
				type = "Village";
			};
			class Settlement_Tisy
			{
				name = "Tisy";
				position[] = {
					3441.6499,
					14799.55
				};
				type = "Village";
			};
			class Settlement_Topolniki
			{
				name = "Topolniki";
				position[] = {
					2913.7451,
					12366.357
				};
				type = "Village";
			};
			class Settlement_Zaprudnoye
			{
				name = "Zaprudnoye";
				position[] = {
					5214,
					12792
				};
				type = "Village";
			};
			class Settlement_Sinystok
			{
				name = "Sinystok";
				position[] = {
					1538.278,
					11904.578
				};
				type = "Village";
			};
			class Settlement_Vavilovo
			{
				name = "Vavilovo";
				position[] = {
					2263,
					11103
				};
				type = "Village";
			};
			class Settlement_Kumyrna
			{
				name = "Kumyrna";
				position[] = {
					8300,
					6054
				};
				type = "Village";
			};
			class Settlement_Kalinovka
			{
				name = "Kalinovka";
				position[] = {
					7448,
					13406
				};
				type = "Village";
			};
			class Settlement_Bogatyrka
			{
				name = "Bogatyrka";
				position[] = {
					1453.62,
					8870.9199
				};
				type = "Village";
			};
			class Settlement_SvyatoyRomanSkiResort
			{
				name = "Ski Resort";
				position[] = {
					638,
					11667
				};
				type = "Local";
			};
			class Settlement_SKVSChBiathlonArena
			{
				name = "Biathlon Arena";
				position[] = {
					357,
					11142
				};
				type = "Local";
			};
			class Settlement_Zvir
			{
				name = "Zvir";
				position[] = {
					577.33002,
					5317.4199
				};
				type = "Village";
			};
			class Settlement_Zabolotye
			{
				name = "Zabolotye";
				position[] = {
					1256.859,
					9993.2598
				};
				type = "Village";
			};
			class Settlement_Galkino
			{
				name = "Galkino";
				position[] = {
					1102.6801,
					8798.2695
				};
				type = "Local";
			};
			class Camp_Arsenovo
			{
				name = "Arsenovo";
				position[] = {
					8509.6611,
					13918.628
				};
				type = "Camp";
			};
			class Camp_Stroytel
			{
				name = "Stroytel";
				position[] = {
					7017.6299,
					4365.21
				};
				type = "Camp";
			};
			class Camp_Romashka
			{
				name = "Romashka";
				position[] = {
					8177.6021,
					10980.289
				};
				type = "Camp";
			};
			class Camp_Kometa
			{
				name = "Kometa";
				position[] = {
					10271.697,
					3558.228
				};
				type = "Camp";
			};
			class Camp_Druzhba
			{
				name = "Druzhba";
				position[] = {
					11437,
					10695
				};
				type = "Camp";
			};
			class Camp_Nadezhda
			{
				name = "Nadezhda";
				position[] = {
					7279.9678,
					7014.3501
				};
				type = "Camp";
			};
			class Camp_YouthPioneer
			{
				name = "Youth Pioneer";
				position[] = {
					11155.825,
					7074.1431
				};
				type = "Camp";
			};
			class Camp_ProudChernarus
			{
				name = "Proud Chernarus";
				position[] = {
					3199.6169,
					6174.9321
				};
				type = "Camp";
			};
			class Camp_Shkolnik
			{
				name = "Shkolnik";
				position[] = {
					14856.503,
					14571.028
				};
				type = "Camp";
			};
			class Camp_Pobeda
			{
				name = "Pobeda";
				position[] = {
					3703.3821,
					14876.384
				};
				type = "Camp";
			};
			class Camp_Metalurg
			{
				name = "Metalurg";
				position[] = {
					1035.406,
					6677.3828
				};
				type = "Camp";
			};
			class Hill_Zelenayagora
			{
				name = "Green Mountain";
				position[] = {
					3767.1699,
					6010.54
				};
				type = "Village";
			};
			class Local_Dichina
			{
				name = "Dichina";
				position[] = {
					4618,
					7821
				};
				type = "Local";
			};
			class Local_Novylug
			{
				name = "Novylug";
				position[] = {
					9251,
					11360
				};
				type = "Local";
			};
			class Local_Staryeluga
			{
				name = "Staryeluga";
				position[] = {
					6919,
					8995
				};
				type = "Local";
			};
			class Hill_Vysota
			{
				name = "Vysota";
				position[] = {
					6591.6299,
					3400
				};
				type = "Hill";
			};
			class Hill_Kopyto
			{
				name = "Kopyto";
				position[] = {
					7871.479,
					3869.1021
				};
				type = "Hill";
			};
			class Local_Grubyeskaly
			{
				name = "Grubyeskaly";
				position[] = {
					13115.06,
					11900
				};
				type = "Local";
			};
			class Local_Chernyles
			{
				name = "Chernyles";
				position[] = {
					9031,
					7803
				};
				type = "Local";
			};
			class Hill_Altar
			{
				name = "Altar";
				position[] = {
					8143.3999,
					9159.6797
				};
				type = "Hill";
			};
			class Local_RadioZenit
			{
				name = "Radio Zenit";
				position[] = {
					8080.0098,
					9341.6797
				};
				type = "Local";
			};
			class Hill_PikKozlova
			{
				name = "Pik Kozlova";
				position[] = {
					8850.1699,
					2880.53
				};
				type = "Hill";
			};
			class Local_Pustoykhrebet
			{
				name = "Pustoykhrebet";
				position[] = {
					10890,
					5665
				};
				type = "Local";
			};
			class Hill_Bashnya
			{
				name = "Bashnya";
				position[] = {
					4178.27,
					11771.22
				};
				type = "Hill";
			};
			class Hill_Veresnik
			{
				name = "Veresnik";
				position[] = {
					4440.1699,
					8070.54
				};
				type = "Hill";
			};
			class Hill_Kurgan
			{
				name = "Kurgan";
				position[] = {
					3368.52,
					5296.8701
				};
				type = "Hill";
			};
			class Hill_Kustryk
			{
				name = "Kustryk";
				position[] = {
					4912.8799,
					5063.4502
				};
				type = "Hill";
			};
			class Hill_Vetryanayagora
			{
				name = "Vetryanayagora";
				position[] = {
					3892.74,
					4200.5898
				};
				type = "Hill";
			};
			class Hill_Kalmyk
			{
				name = "Kalmyk";
				position[] = {
					6903.7798,
					4919.6602
				};
				type = "Hill";
			};
			class Hill_PopIvan
			{
				name = "PopIvan";
				position[] = {
					6420.2598,
					6570.6602
				};
				type = "Hill";
			};
			class Hill_Erbenka
			{
				name = "Erbenka";
				position[] = {
					2842.916,
					4014.811
				};
				type = "Hill";
			};
			class Local_Lesnoykhrebet
			{
				name = "Lesnoykhrebet";
				position[] = {
					8122.77,
					7815.54
				};
				type = "Local";
			};
			class Hill_Vysokiykamen
			{
				name = "Vysokiykamen";
				position[] = {
					8940.1904,
					4380.52
				};
				type = "Hill";
			};
			class Hill_Dobry
			{
				name = "Dobry";
				position[] = {
					10552.9,
					3061.03
				};
				type = "Hill";
			};
			class Hill_Baranchik
			{
				name = "Baranchik";
				position[] = {
					10153.147,
					6208.521
				};
				type = "Hill";
			};
			class Hill_Malinovka
			{
				name = "Malinovka";
				position[] = {
					10897.7,
					7575.5898
				};
				type = "Hill";
			};
			class Hill_Dubina
			{
				name = "Dubina";
				position[] = {
					11107.9,
					8474.8301
				};
				type = "Hill";
			};
			class Hill_Klen
			{
				name = "Klen";
				position[] = {
					11473.511,
					11315.393
				};
				type = "Hill";
			};
			class Hill_Chernayagora
			{
				name = "Black Mountain";
				position[] = {
					10280.816,
					12053.928
				};
				type = "Hill";
			};
			class Ruin_Zolotar
			{
				name = "Zolotar";
				position[] = {
					10179,
					11998
				};
				type = "Ruin";
			};
			class Hill_Ostry
			{
				name = "Ostry";
				position[] = {
					10792.747,
					12829.504
				};
				type = "Hill";
			};
			class Hill_Olsha
			{
				name = "Olsha";
				position[] = {
					12975.7,
					12775.2
				};
				type = "Hill";
			};
			class Marine_Tikhiyzaliv
			{
				name = "Tikhiyzaliv";
				position[] = {
					1221.49,
					2111.8899
				};
				type = "Marine";
			};
			class Marine_Mutnyizaliv
			{
				name = "Mutnyizaliv";
				position[] = {
					5735.479,
					1918.92
				};
				type = "Marine";
			};
			class Marine_Chernyzaliv
			{
				name = "Chernyzaliv";
				position[] = {
					7599.0688,
					2486.5859
				};
				type = "Marine";
			};
			class Marine_Zelenyzaliv
			{
				name = "Zelenyzaliv";
				position[] = {
					11227.019,
					2989.8379
				};
				type = "Marine";
			};
			class Marine_Skalistyproliv
			{
				name = "Skalistyproliv";
				position[] = {
					13385.92,
					3613.9399
				};
				type = "Marine";
			};
			class Marine_Nizhniyzaliv
			{
				name = "Nizhniyzaliv";
				position[] = {
					12989.3,
					8515.7598
				};
				type = "Marine";
			};
			class Marine_ZalivGuba
			{
				name = "Zaliv Guba";
				position[] = {
					14328.374,
					13136.732
				};
				type = "Marine";
			};
			class Marine_Rify
			{
				name = "Rify";
				position[] = {
					13931.73,
					11288.61
				};
				type = "Local";
			};
			class Marine_Ivovoyeozero
			{
				name = "Ivovoyeozero";
				position[] = {
					13248,
					11572
				};
				type = "Marine";
			};
			class Marine_Chernoyeozero
			{
				name = "Chernoyeozero";
				position[] = {
					13381,
					12002
				};
				type = "Marine";
			};
			class Marine_PlotinaTopolka
			{
				name = "Topolka Dam";
				position[] = {
					10231.33,
					3691.1499
				};
				type = "Marine";
			};
			class Marine_PlotinaPobeda
			{
				name = "Plotina Pobeda";
				position[] = {
					9928.2939,
					13874.643
				};
				type = "Marine";
			};
			class Marine_PlotinaTishina
			{
				name = "Plotina Tishina";
				position[] = {
					1150.8199,
					6432.4541
				};
				type = "Marine";
			};
			class Marine_Ozerko
			{
				name = "Ozerko";
				position[] = {
					6777.6299,
					4492.4199
				};
				type = "Marine";
			};
			class Marine_Prud
			{
				name = "Prud";
				position[] = {
					6610.98,
					9308.5703
				};
				type = "Marine";
			};
			class Ruin_Chortovzamok
			{
				name = "Devil's Castle";
				position[] = {
					6883.2388,
					11501.288
				};
				type = "Ruin";
			};
			class Ruin_Zub
			{
				name = "Zub";
				position[] = {
					6574.2798,
					5573.8501
				};
				type = "Ruin";
			};
			class Ruin_Rog
			{
				name = "Rog";
				position[] = {
					11267.206,
					4293.0361
				};
				type = "Ruin";
			};
			class Local_Grozovypereval
			{
				name = "Pass Grozovy";
				position[] = {
					3318.1201,
					15250.55
				};
				type = "Local";
			};
			class Local_Sosnovypereval
			{
				name = "Pass Sosnovy";
				position[] = {
					2687.3999,
					6590.2798
				};
				type = "Local";
			};
			class Local_PerevalOreshka
			{
				name = "Pass Oreshka";
				position[] = {
					9822.1914,
					6088.793
				};
				type = "Local";
			};
			class Local_Turovskiypereval
			{
				name = "Pass Turovskiy";
				position[] = {
					14674.241,
					14089.306
				};
				type = "Local";
			};
			class Local_Tridoliny
			{
				name = "Tridoliny";
				position[] = {
					12764.47,
					5412.21
				};
				type = "Local";
			};
			class Local_Grozovyedoly
			{
				name = "Grozovyedoly";
				position[] = {
					10403.982,
					14793.168
				};
				type = "Local";
			};
			class Ruin_Klyuch
			{
				name = "Klyuch";
				position[] = {
					9282.5703,
					13476.67
				};
				type = "Ruin";
			};
			class Hill_Lysayagora
			{
				name = "Lysayagora";
				position[] = {
					6722.459,
					14050.42
				};
				type = "Hill";
			};
			class Marine_Glubokoyeozero
			{
				name = "Glubokoyeozero";
				position[] = {
					1692.5551,
					14821.565
				};
				type = "Marine";
			};
			class Local_Skalka
			{
				name = "Skalka";
				position[] = {
					5758,
					14486
				};
				type = "Local";
			};
			class Local_Vidy
			{
				name = "Vidy";
				position[] = {
					785.05402,
					14398.815
				};
				type = "Local";
			};
			class Hill_Tumannyverkh
			{
				name = "Tumannyverkh";
				position[] = {
					331.224,
					12982.99
				};
				type = "Hill";
			};
			class Local_Adamovka
			{
				name = "Adamovka";
				position[] = {
					5340.6602,
					11380.01
				};
				type = "Local";
			};
			class Hill_Shishok
			{
				name = "Shishok";
				position[] = {
					3559.313,
					9422.2695
				};
				type = "Hill";
			};
			class Settlement_Skalisty
			{
				name = "Skalisty";
				position[] = {
					13715.454,
					3118.78
				};
				type = "Village";
			};
			class Ruin_Storozh
			{
				name = "Storozh";
				position[] = {
					2816.6011,
					1277.96
				};
				type = "Ruin";
			};
			class Local_MysGolova
			{
				name = "Mys Golova";
				position[] = {
					8286.0527,
					2405.5979
				};
				type = "Local";
			};
			class Local_Drakon
			{
				name = "Drakon";
				position[] = {
					11191,
					2443
				};
				type = "Local";
			};
			class Local_Otmel
			{
				name = "Otmel";
				position[] = {
					11581.25,
					3213.24
				};
				type = "Local";
			};
			class Local_MysKrutoy
			{
				name = "Mys Krutoy";
				position[] = {
					13578.19,
					3976.8201
				};
				type = "Local";
			};
			class Hill_Tokarnya
			{
				name = "Tokarnya";
				position[] = {
					8890.4775,
					5672.5532
				};
				type = "Hill";
			};
			class Hill_Ostrog
			{
				name = "Ostrog";
				position[] = {
					2655.7339,
					2264.6321
				};
				type = "Hill";
			};
			class Local_Maryanka
			{
				name = "Maryanka";
				position[] = {
					2789,
					3386
				};
				type = "Local";
			};
			class Local_Polonina
			{
				name = "Polonina";
				position[] = {
					1004.486,
					4242.8691
				};
				type = "Local";
			};
			class Local_Kalinka
			{
				name = "Kalinka";
				position[] = {
					3346.5,
					11292.097
				};
				type = "Local";
			};
			class Hill_Kikimora
			{
				name = "Kikimora";
				position[] = {
					1812,
					6200
				};
				type = "Hill";
			};
			class Hill_BolshoyKotel
			{
				name = "Bolshoy Kotel";
				position[] = {
					714.64899,
					6520.4312
				};
				type = "Hill";
			};
			class Hill_Simurg
			{
				name = "Simurg";
				position[] = {
					134,
					7569.9902
				};
				type = "Hill";
			};
			class Hill_Volchiypik
			{
				name = "Volchiypik";
				position[] = {
					320.254,
					2719.269
				};
				type = "Hill";
			};
			class Ruin_Krona
			{
				name = "Krona";
				position[] = {
					1493,
					9270.3398
				};
				type = "Ruin";
			};
			class Local_TriKresta
			{
				name = "Tri Kresta";
				position[] = {
					301,
					9478
				};
				type = "Local";
			};
			class Local_Rostoki
			{
				name = "Rostoki";
				position[] = {
					744.45001,
					8647.8604
				};
				type = "Local";
			};
			class Marine_OrlyeOzero
			{
				name = "Orlye Ozero";
				position[] = {
					597,
					5990
				};
				type = "Marine";
			};
			class Local_Makosh
			{
				name = "Makosh";
				position[] = {
					7849.7202,
					6480.1899
				};
				type = "Local";
			};
			class Local_Klenovyipereval
			{
				name = "Klenovyipereval";
				position[] = {
					10862.702,
					11585.512
				};
				type = "Local";
			};
			class Local_Zmeinykhrebet
			{
				name = "Zmeinykhrebet";
				position[] = {
					11446.17,
					13606.92
				};
				type = "Local";
			};
			class Hill_Sokol
			{
				name = "Sokol";
				position[] = {
					12001,
					14813
				};
				type = "Hill";
			};
			class Local_Krutyeskaly
			{
				name = "Krutyeskaly";
				position[] = {
					14880.63,
					13671.76
				};
				type = "Local";
			};
			class Local_Bogat
			{
				name = "Bogat";
				position[] = {
					7058.0112,
					12023.571
				};
				type = "Local";
			};
			class Local_Dubnik
			{
				name = "Dubnik";
				position[] = {
					3298.29,
					10330.89
				};
				type = "Local";
			};
			class Hill_Viselitsa
			{
				name = "Viselitsa";
				position[] = {
					12722.157,
					7504.0498
				};
				type = "Hill";
			};
			class Local_Dazhbog
			{
				name = "Dazhbog";
				position[] = {
					6581,
					8590
				};
				type = "Hill";
			};
			class Marine_Verbnik
			{
				name = "Verbnik";
				position[] = {
					4414.8789,
					9103.002
				};
				type = "Marine";
			};
			class Local_Medvezhilugi
			{
				name = "Medvezhilugi";
				position[] = {
					9641,
					13236
				};
				type = "Local";
			};
			class Ruin_Voron
			{
				name = "Voron";
				position[] = {
					13517.93,
					3311.2451
				};
				type = "Ruin";
			};
			class Ruin_Gnomovzamok
			{
				name = "Gnomovzamok";
				position[] = {
					7446.915,
					9097.1699
				};
				type = "Ruin";
			};
			class Marine_Glaza
			{
				name = "Glaza";
				position[] = {
					7383,
					9320
				};
				type = "Marine";
			};
			class Local_KarerKrasnayaZarya
			{
				name = "Karer Krasnaya Zarya";
				position[] = {
					8565.3486,
					13418.684
				};
				type = "Local";
			};
			class Local_Matveyevo
			{
				name = "Matveyevo";
				position[] = {
					4281,
					7380
				};
				type = "Local";
			};
			class Local_Kotka
			{
				name = "Kotka";
				position[] = {
					5873.7002,
					6883.2402
				};
				type = "Local";
			};
			class Local_Chernyeskaly
			{
				name = "Chernyeskaly";
				position[] = {
					3932.572,
					14599.496
				};
				type = "Local";
			};
			class Hill_SvyatoyRoman
			{
				name = "Svyatoy Roman";
				position[] = {
					84.832001,
					11901.246
				};
				type = "Hill";
			};
			class Hill_Koman
			{
				name = "Koman";
				position[] = {
					1660.652,
					7415.6611
				};
				type = "Hill";
			};
			class Hill_Mayak
			{
				name = "Mayak";
				position[] = {
					12323.813,
					4553.1069
				};
				type = "Hill";
			};
			class Local_MB_Berezino
			{
				name = "Berezino Military";
				position[] = {
					11966,
					9130
				};
				type = "SmallMilitary";
			};
			class Local_MB_NovyLug
			{
				name = "Novy Lug Military";
				position[] = {
					9579,
					11796
				};
				type = "SmallMilitary";
			};
			class Local_MB_Elektrozavodsk
			{
				name = "Elektrozavodsk Military";
				position[] = {
					10481,
					2356
				};
				type = "SmallMilitary";
			};
			class Local_MB_Turovo
			{
				name = "Turovo Military";
				position[] = {
					13626,
					13740
				};
				type = "SmallMilitary";
			};
			class Local_MB_StarySobor
			{
				name = "Stary Sobor Military";
				position[] = {
					6351,
					7790
				};
				type = "MediumMilitary";
			};
			class Local_MB_Sosnovy
			{
				name = "Sosnovy Military";
				position[] = {
					2720,
					6760
				};
				type = "SmallMilitary";
			};
			class Local_MB_Ostry
			{
				name = "Ostry Military";
				position[] = {
					10958,
					13341
				};
				type = "MediumMilitary";
			};
			class Local_MB_Severograd
			{
				name = "Severograd Military";
				position[] = {
					7074,
					12735
				};
				type = "SmallMilitary";
			};
			class Local_MB_Arsenovo
			{
				name = "Arsenovo Military";
				position[] = {
					8560,
					14000
				};
				type = "SmallMilitary";
			};
			class Local_MB_KamenskSmall
			{
				name = "Kamensk Military";
				position[] = {
					7075,
					14520
				};
				type = "SmallMilitary";
			};
			class Local_MB_Grozovy
			{
				name = "Grozovy Military";
				position[] = {
					3660,
					14920
				};
				type = "SmallMilitary";
			};
			class Local_MB_Zaprudnoye
			{
				name = "Zaprudnoye Military";
				position[] = {
					4964,
					12908
				};
				type = "SmallMilitary";
			};
			class Local_MB_Bashnya
			{
				name = "Bashnya Military";
				position[] = {
					4107,
					11660
				};
				type = "MediumMilitary";
			};
			class Local_MB_TriKresta
			{
				name = "Tri Kresta Military";
				position[] = {
					324,
					9324
				};
				type = "SmallMilitary";
			};
			class Local_MB_Tikhiyzaliv
			{
				name = "Tikhiyzaliv Military";
				position[] = {
					780,
					1870
				};
				type = "MediumMilitary";
			};
			class Local_MB_PrisonIsland
			{
				name = "Prison Island Military";
				position[] = {
					2706,
					1355
				};
				type = "MediumMilitary";
			};
			class Local_MB_Chernogorsk
			{
				name = "Chernogorsk Military";
				position[] = {
					7260,
					3024
				};
				type = "SmallMilitary";
			};
			class Local_MB_Novoselki
			{
				name = "Novoselki Military";
				position[] = {
					5820,
					3080
				};
				type = "SmallMilitary";
			};
			class Local_MB_Topolka
			{
				name = "Topolka Military";
				position[] = {
					10236,
					4029
				};
				type = "SmallMilitary";
			};
			class Local_MB_Staroye
			{
				name = "Staroye Military";
				position[] = {
					10441,
					5970
				};
				type = "MediumMilitary";
			};
			class Local_MB_Gorka
			{
				name = "Gorka Military";
				position[] = {
					10470,
					9170
				};
				type = "SmallMilitary";
			};
			class Local_MB_Myshkino
			{
				name = "Myshkino Military Base";
				position[] = {
					1190,
					7240
				};
				type = "MediumMilitary";
			};
			class Local_MB_VMC
			{
				name = "VMC Military Base";
				position[] = {
					4600,
					8291
				};
				type = "BigMilitary";
			};
			class Local_MB_Tisy
			{
				name = "Tisy Military Base";
				position[] = {
					1390,
					13840
				};
				type = "BigMilitary";
			};
			class Local_MB_Kamensk
			{
				name = "Troitskoe Military Base";
				position[] = {
					7920,
					14730
				};
				type = "BigMilitary";
			};
			class Local_MB_Zeleno
			{
				name = "Zeleno Military";
				position[] = {
					2415.375,
					5160.6812
				};
				type = "MediumMilitary";
			};
			class Local_MB_Pavlovo
			{
				name = "Pavlovo Military Base";
				position[] = {
					2136,
					3516
				};
				type = "BigMilitary";
			};
			class Local_AF_Balota
			{
				name = "Airstrip Balota";
				position[] = {
					5082.0308,
					2376.3279
				};
				type = "BigMilitary";
			};
			class Local_AF_Krasno
			{
				name = "Airstrip Krasnostav";
				position[] = {
					12100.681,
					12573.896
				};
				type = "MediumMilitary";
			};
			class Local_AF_Vybor
			{
				name = "North West Airfield";
				position[] = {
					4428,
					10179
				};
				type = "BigMilitary";
			};
			class RailroadStation_Elektro
			{
				name = "";
				position[] = {
					10309.912,
					2092.2029
				};
				type = "RailroadStation";
			};
			class RailroadStation_Prigorodki
			{
				name = "";
				position[] = {
					8056.0342,
					3264.908
				};
				type = "RailroadStation";
			};
			class RailroadStation_Cherno
			{
				name = "";
				position[] = {
					6540.626,
					2638.7749
				};
				type = "RailroadStation";
			};
			class RailroadStation_Balota
			{
				name = "";
				position[] = {
					4399.2588,
					2309.397
				};
				type = "RailroadStation";
			};
			class RailroadStation_Komarovo
			{
				name = "";
				position[] = {
					3670.667,
					2397.5249
				};
				type = "RailroadStation";
			};
			class RailroadStation_Kamenka
			{
				name = "";
				position[] = {
					1877.9351,
					2168.5591
				};
				type = "RailroadStation";
			};
			class RailroadStation_Zeleno
			{
				name = "";
				position[] = {
					2485.2661,
					5214.6641
				};
				type = "RailroadStation";
			};
			class RailroadStation_Vavilovo
			{
				name = "";
				position[] = {
					1991.5031,
					11289.488
				};
				type = "RailroadStation";
			};
			class RailroadStation_Novaya
			{
				name = "";
				position[] = {
					3534.425,
					12537.095
				};
				type = "RailroadStation";
			};
			class RailroadStation_Severograd
			{
				name = "";
				position[] = {
					7846.5811,
					12401.488
				};
				type = "RailroadStation";
			};
			class RailroadStation_Novo
			{
				name = "";
				position[] = {
					11610.908,
					14133.702
				};
				type = "RailroadStation";
			};
			class RailroadStation_Svetlo
			{
				name = "";
				position[] = {
					13970.473,
					13486.344
				};
				type = "RailroadStation";
			};
			class RailroadStation_Berezino
			{
				name = "";
				position[] = {
					12989.844,
					10200.673
				};
				type = "RailroadStation";
			};
			class RailroadStation_Solnich
			{
				name = "";
				position[] = {
					13146.109,
					7094.4722
				};
				type = "RailroadStation";
			};
			class RailroadStation_Solnich2
			{
				name = "";
				position[] = {
					13356.109,
					6154.4722
				};
				type = "RailroadStation";
			};
			class RailroadStation_Kamyshovo
			{
				name = "";
				position[] = {
					11970.969,
					3548.7229
				};
				type = "RailroadStation";
			};
			class RailroadStation_Dobroye
			{
				name = "";
				position[] = {
					12860.658,
					15050.221
				};
				type = "RailroadStation";
			};
			class LocalOffice_Novaya
			{
				name = "";
				position[] = {
					3469.1721,
					13135.165
				};
				type = "LocalOffice";
			};
			class LocalOffice_Vybor
			{
				name = "";
				position[] = {
					3857.656,
					8946.1396
				};
				type = "LocalOffice";
			};
			class LocalOffice_Sinys
			{
				name = "";
				position[] = {
					1439.08,
					12011.26
				};
				type = "LocalOffice";
			};
			class LocalOffice_Vavil
			{
				name = "";
				position[] = {
					2220.8401,
					11075.83
				};
				type = "LocalOffice";
			};
			class LocalOffice_Lopat
			{
				name = "";
				position[] = {
					2720.72,
					9978.1396
				};
				type = "LocalOffice";
			};
			class LocalOffice_Pustosh
			{
				name = "";
				position[] = {
					3051.4399,
					7804.6802
				};
				type = "LocalOffice";
			};
			class LocalOffice_Pogorev
			{
				name = "";
				position[] = {
					4480.71,
					6462
				};
				type = "LocalOffice";
			};
			class LocalOffice_Kabanin
			{
				name = "";
				position[] = {
					5382,
					8589
				};
				type = "LocalOffice";
			};
			class LocalOffice_Stary
			{
				name = "";
				position[] = {
					5971.0801,
					7743.3501
				};
				type = "LocalOffice";
			};
			class LocalOffice_Novy
			{
				name = "";
				position[] = {
					7166.96,
					7526.0098
				};
				type = "LocalOffice";
			};
			class LocalOffice_Grishino
			{
				name = "";
				position[] = {
					5983,
					10305
				};
				type = "LocalOffice";
			};
			class LocalOffice_Severo
			{
				name = "";
				position[] = {
					7986,
					12732
				};
				type = "LocalOffice";
			};
			class LocalOffice_Gorka
			{
				name = "";
				position[] = {
					9503,
					8805
				};
				type = "LocalOffice";
			};
			class LocalOffice_Mogi
			{
				name = "";
				position[] = {
					7562,
					5149
				};
				type = "LocalOffice";
			};
			class LocalOffice_Nadez
			{
				name = "";
				position[] = {
					5906,
					4844
				};
				type = "LocalOffice";
			};
			class LocalOffice_Cherno
			{
				name = "";
				position[] = {
					6607,
					2418
				};
				type = "LocalOffice";
			};
			class LocalOffice_Kozlov
			{
				name = "";
				position[] = {
					4394,
					4629
				};
				type = "LocalOffice";
			};
			class LocalOffice_Komar
			{
				name = "";
				position[] = {
					3676,
					2501
				};
				type = "LocalOffice";
			};
			class LocalOffice_Pavlovo
			{
				name = "";
				position[] = {
					1655,
					3851
				};
				type = "LocalOffice";
			};
			class LocalOffice_Zeleno
			{
				name = "";
				position[] = {
					2854,
					5282
				};
				type = "LocalOffice";
			};
			class LocalOffice_Elektro
			{
				name = "";
				position[] = {
					10212,
					2295
				};
				type = "LocalOffice";
			};
			class LocalOffice_Kamys
			{
				name = "";
				position[] = {
					12155,
					3514
				};
				type = "LocalOffice";
			};
			class LocalOffice_Staroye
			{
				name = "";
				position[] = {
					10106,
					5457
				};
				type = "LocalOffice";
			};
			class LocalOffice_Dolina
			{
				name = "";
				position[] = {
					11217,
					6564
				};
				type = "LocalOffice";
			};
			class LocalOffice_Solnich
			{
				name = "";
				position[] = {
					13383,
					6225
				};
				type = "LocalOffice";
			};
			class LocalOffice_Polana
			{
				name = "";
				position[] = {
					10676,
					7989
				};
				type = "LocalOffice";
			};
			class LocalOffice_Berezino
			{
				name = "";
				position[] = {
					12272,
					9474
				};
				type = "LocalOffice";
			};
			class LocalOffice_Krasno
			{
				name = "";
				position[] = {
					11257,
					12211
				};
				type = "LocalOffice";
			};
			class LocalOffice_Svetlo
			{
				name = "";
				position[] = {
					14013,
					13335
				};
				type = "LocalOffice";
			};
			class LocalOffice_Cpol
			{
				name = "";
				position[] = {
					12110,
					13779
				};
				type = "LocalOffice";
			};
			class LocalOffice_Novo
			{
				name = "";
				position[] = {
					11564,
					14742
				};
				type = "LocalOffice";
			};
			class ViewPoint_1
			{
				name = "";
				position[] = {
					567,
					6997
				};
				type = "ViewPoint";
			};
			class ViewPoint_2
			{
				name = "";
				position[] = {
					687,
					6874
				};
				type = "ViewPoint";
			};
			class ViewPoint_3
			{
				name = "";
				position[] = {
					724,
					6510
				};
				type = "ViewPoint";
			};
			class ViewPoint_4
			{
				name = "";
				position[] = {
					298,
					3460
				};
				type = "ViewPoint";
			};
			class ViewPoint_5
			{
				name = "";
				position[] = {
					172,
					3412
				};
				type = "ViewPoint";
			};
			class ViewPoint_6
			{
				name = "";
				position[] = {
					258,
					3283
				};
				type = "ViewPoint";
			};
			class ViewPoint_7
			{
				name = "";
				position[] = {
					322,
					2697
				};
				type = "ViewPoint";
			};
			class ViewPoint_8
			{
				name = "";
				position[] = {
					1671,
					2211
				};
				type = "ViewPoint";
			};
			class ViewPoint_9
			{
				name = "";
				position[] = {
					2915,
					3995
				};
				type = "ViewPoint";
			};
			class ViewPoint_10
			{
				name = "";
				position[] = {
					3371,
					4551
				};
				type = "ViewPoint";
			};
			class ViewPoint_11
			{
				name = "";
				position[] = {
					414,
					9574
				};
				type = "ViewPoint";
			};
			class ViewPoint_12
			{
				name = "";
				position[] = {
					241,
					11867
				};
				type = "ViewPoint";
			};
			class ViewPoint_13
			{
				name = "";
				position[] = {
					1153,
					12411
				};
				type = "ViewPoint";
			};
			class ViewPoint_14
			{
				name = "";
				position[] = {
					2320,
					13650
				};
				type = "ViewPoint";
			};
			class ViewPoint_15
			{
				name = "";
				position[] = {
					4084,
					14570
				};
				type = "ViewPoint";
			};
			class ViewPoint_16
			{
				name = "";
				position[] = {
					4445,
					15045
				};
				type = "ViewPoint";
			};
			class ViewPoint_17
			{
				name = "";
				position[] = {
					4926,
					14820
				};
				type = "ViewPoint";
			};
			class ViewPoint_18
			{
				name = "";
				position[] = {
					5172,
					14841
				};
				type = "ViewPoint";
			};
			class ViewPoint_19
			{
				name = "";
				position[] = {
					5345,
					14873
				};
				type = "ViewPoint";
			};
			class ViewPoint_20
			{
				name = "";
				position[] = {
					5480,
					14822
				};
				type = "ViewPoint";
			};
			class ViewPoint_21
			{
				name = "";
				position[] = {
					5692,
					14771
				};
				type = "ViewPoint";
			};
			class ViewPoint_22
			{
				name = "";
				position[] = {
					5896,
					14747
				};
				type = "ViewPoint";
			};
			class ViewPoint_23
			{
				name = "";
				position[] = {
					6246,
					14707
				};
				type = "ViewPoint";
			};
			class ViewPoint_24
			{
				name = "";
				position[] = {
					6434,
					14816
				};
				type = "ViewPoint";
			};
			class ViewPoint_25
			{
				name = "";
				position[] = {
					6707,
					13961
				};
				type = "ViewPoint";
			};
			class ViewPoint_26
			{
				name = "";
				position[] = {
					6861,
					14115
				};
				type = "ViewPoint";
			};
			class ViewPoint_27
			{
				name = "";
				position[] = {
					8710,
					14200
				};
				type = "ViewPoint";
			};
			class ViewPoint_28
			{
				name = "";
				position[] = {
					10822,
					14651
				};
				type = "ViewPoint";
			};
			class ViewPoint_29
			{
				name = "";
				position[] = {
					11369,
					15037
				};
				type = "ViewPoint";
			};
			class ViewPoint_30
			{
				name = "";
				position[] = {
					13604,
					14632
				};
				type = "ViewPoint";
			};
			class ViewPoint_31
			{
				name = "";
				position[] = {
					13877,
					14703
				};
				type = "ViewPoint";
			};
			class ViewPoint_32
			{
				name = "";
				position[] = {
					14140,
					15244
				};
				type = "ViewPoint";
			};
			class ViewPoint_33
			{
				name = "";
				position[] = {
					13840,
					13833
				};
				type = "ViewPoint";
			};
			class ViewPoint_34
			{
				name = "";
				position[] = {
					14075,
					13889
				};
				type = "ViewPoint";
			};
			class ViewPoint_35
			{
				name = "";
				position[] = {
					14895,
					13611
				};
				type = "ViewPoint";
			};
			class ViewPoint_36
			{
				name = "";
				position[] = {
					14828,
					13759
				};
				type = "ViewPoint";
			};
			class ViewPoint_37
			{
				name = "";
				position[] = {
					11982,
					13388
				};
				type = "ViewPoint";
			};
			class ViewPoint_38
			{
				name = "";
				position[] = {
					11690,
					13511
				};
				type = "ViewPoint";
			};
			class ViewPoint_39
			{
				name = "";
				position[] = {
					11473,
					13580
				};
				type = "ViewPoint";
			};
			class ViewPoint_40
			{
				name = "";
				position[] = {
					11376,
					13661
				};
				type = "ViewPoint";
			};
			class ViewPoint_41
			{
				name = "";
				position[] = {
					13937,
					11250
				};
				type = "ViewPoint";
			};
			class ViewPoint_42
			{
				name = "";
				position[] = {
					11481,
					8429
				};
				type = "ViewPoint";
			};
			class ViewPoint_43
			{
				name = "";
				position[] = {
					11574,
					7743
				};
				type = "ViewPoint";
			};
			class ViewPoint_44
			{
				name = "";
				position[] = {
					12123,
					6997
				};
				type = "ViewPoint";
			};
			class ViewPoint_45
			{
				name = "";
				position[] = {
					12980,
					5967
				};
				type = "ViewPoint";
			};
			class ViewPoint_46
			{
				name = "";
				position[] = {
					12352,
					4519
				};
				type = "ViewPoint";
			};
			class ViewPoint_47
			{
				name = "";
				position[] = {
					12304,
					3976
				};
				type = "ViewPoint";
			};
			class ViewPoint_48
			{
				name = "";
				position[] = {
					10584,
					2867
				};
				type = "ViewPoint";
			};
			class ViewPoint_49
			{
				name = "";
				position[] = {
					8767,
					2959
				};
				type = "ViewPoint";
			};
			class ViewPoint_50
			{
				name = "";
				position[] = {
					8811,
					2850
				};
				type = "ViewPoint";
			};
			class ViewPoint_51
			{
				name = "";
				position[] = {
					8547,
					3872
				};
				type = "ViewPoint";
			};
			class ViewPoint_52
			{
				name = "";
				position[] = {
					9029,
					4380
				};
				type = "ViewPoint";
			};
			class ViewPoint_53
			{
				name = "";
				position[] = {
					10741,
					4180
				};
				type = "ViewPoint";
			};
			class ViewPoint_54
			{
				name = "";
				position[] = {
					6897,
					11648
				};
				type = "ViewPoint";
			};
			class ViewPoint_55
			{
				name = "";
				position[] = {
					5633,
					6885
				};
				type = "ViewPoint";
			};
			class ViewPoint_56
			{
				name = "";
				position[] = {
					1456,
					9611
				};
				type = "ViewPoint";
			};
			class ViewPoint_57
			{
				name = "";
				position[] = {
					1941,
					6768
				};
				type = "ViewPoint";
			};
			class FeedShack_1
			{
				name = "";
				position[] = {
					80.255409,
					4422.158691
				};
				type = "FeedShack";
			};
			class FeedShack_2
			{
				name = "";
				position[] = {
					80.250587,
					10449.115234
				};
				type = "FeedShack";
			};
			class FeedShack_3
			{
				name = "";
				position[] = {
					151.035278,
					3655.145508
				};
				type = "FeedShack";
			};
			class FeedShack_4
			{
				name = "";
				position[] = {
					178.267670,
					7858.565918
				};
				type = "FeedShack";
			};
			class FeedShack_5
			{
				name = "";
				position[] = {
					139.805328,
					14357.005859
				};
				type = "FeedShack";
			};
			class FeedShack_6
			{
				name = "";
				position[] = {
					209.610764,
					11768.070313
				};
				type = "FeedShack";
			};
			class DeerStand_1
			{
				name = "";
				position[] = {
					196.368607,
					11938.011719
				};
				type = "DeerStand";
			};
			class DeerStand_2
			{
				name = "";
				position[] = {
					258.460449,
					4340.960938
				};
				type = "DeerStand";
			};
			class DeerStand_3
			{
				name = "";
				position[] = {
					275.758148,
					5026.475098
				};
				type = "DeerStand";
			};
			class FeedShack_7
			{
				name = "";
				position[] = {
					271.583893,
					5607.263672
				};
				type = "FeedShack";
			};
			class DeerStand_4
			{
				name = "";
				position[] = {
					284.283630,
					9767.231445
				};
				type = "DeerStand";
			};
			class FeedShack_8
			{
				name = "";
				position[] = {
					320.447876,
					12537.249023
				};
				type = "FeedShack";
			};
			class DeerStand_5
			{
				name = "";
				position[] = {
					348.418243,
					12972.145508
				};
				type = "DeerStand";
			};
			class DeerStand_6
			{
				name = "";
				position[] = {
					386.489899,
					3859.374023
				};
				type = "DeerStand";
			};
			class FeedShack_9
			{
				name = "";
				position[] = {
					398.866913,
					4056.456543
				};
				type = "FeedShack";
			};
			class DeerStand_7
			{
				name = "";
				position[] = {
					366.036987,
					6427.575684
				};
				type = "DeerStand";
			};
			class DeerStand_8
			{
				name = "";
				position[] = {
					469.456177,
					2043.090576
				};
				type = "DeerStand";
			};
			class FeedShack_10
			{
				name = "";
				position[] = {
					422.996643,
					9848.817383
				};
				type = "FeedShack";
			};
			class FeedShack_11
			{
				name = "";
				position[] = {
					456.326569,
					13249.150391
				};
				type = "FeedShack";
			};
			class DeerStand_9
			{
				name = "";
				position[] = {
					537.170349,
					9312.600586
				};
				type = "DeerStand";
			};
			class DeerStand_10
			{
				name = "";
				position[] = {
					532.378784,
					10757.907227
				};
				type = "DeerStand";
			};
			class FeedShack_12
			{
				name = "";
				position[] = {
					597.511475,
					6747.661621
				};
				type = "FeedShack";
			};
			class DeerStand_11
			{
				name = "";
				position[] = {
					555.714111,
					7361.366211
				};
				type = "DeerStand";
			};
			class FeedShack_13
			{
				name = "";
				position[] = {
					576.444336,
					8142.421387
				};
				type = "FeedShack";
			};
			class FeedShack_14
			{
				name = "";
				position[] = {
					647.718506,
					2313.115723
				};
				type = "FeedShack";
			};
			class DeerStand_12
			{
				name = "";
				position[] = {
					628.041870,
					7189.393555
				};
				type = "DeerStand";
			};
			class DeerStand_13
			{
				name = "";
				position[] = {
					677.399963,
					2779.036621
				};
				type = "DeerStand";
			};
			class DeerStand_14
			{
				name = "";
				position[] = {
					766.845154,
					6010.144531
				};
				type = "DeerStand";
			};
			class FeedShack_15
			{
				name = "";
				position[] = {
					820.721741,
					3153.156494
				};
				type = "FeedShack";
			};
			class DeerStand_15
			{
				name = "";
				position[] = {
					795.369812,
					4727.419922
				};
				type = "DeerStand";
			};
			class FeedShack_16
			{
				name = "";
				position[] = {
					798.055237,
					4781.344238
				};
				type = "FeedShack";
			};
			class DeerStand_16
			{
				name = "";
				position[] = {
					831.706970,
					6977.135254
				};
				type = "DeerStand";
			};
			class FeedShack_17
			{
				name = "";
				position[] = {
					807.184082,
					7155.583984
				};
				type = "FeedShack";
			};
			class DeerStand_17
			{
				name = "";
				position[] = {
					786.583740,
					8676.427734
				};
				type = "DeerStand";
			};
			class DeerStand_18
			{
				name = "";
				position[] = {
					804.834229,
					10798.018555
				};
				type = "DeerStand";
			};
			class DeerStand_19
			{
				name = "";
				position[] = {
					884.092896,
					4147.766113
				};
				type = "DeerStand";
			};
			class DeerStand_20
			{
				name = "";
				position[] = {
					873.904053,
					5302.406738
				};
				type = "DeerStand";
			};
			class FeedShack_18
			{
				name = "";
				position[] = {
					877.262695,
					6184.382324
				};
				type = "FeedShack";
			};
			class FeedShack_19
			{
				name = "";
				position[] = {
					962.603821,
					6855.634277
				};
				type = "FeedShack";
			};
			class DeerStand_21
			{
				name = "";
				position[] = {
					1056.565186,
					11242.220703
				};
				type = "DeerStand";
			};
			class DeerStand_22
			{
				name = "";
				position[] = {
					1032.998657,
					12424.235352
				};
				type = "DeerStand";
			};
			class DeerStand_23
			{
				name = "";
				position[] = {
					1100.015991,
					3585.554443
				};
				type = "DeerStand";
			};
			class DeerStand_24
			{
				name = "";
				position[] = {
					1276.824585,
					7723.996094
				};
				type = "DeerStand";
			};
			class FeedShack_20
			{
				name = "";
				position[] = {
					1292.369019,
					7988.301270
				};
				type = "FeedShack";
			};
			class DeerStand_25
			{
				name = "";
				position[] = {
					1270.747192,
					10175.732422
				};
				type = "DeerStand";
			};
			class FeedShack_21
			{
				name = "";
				position[] = {
					1354.832886,
					2457.884277
				};
				type = "FeedShack";
			};
			class FeedShack_22
			{
				name = "";
				position[] = {
					1371.501831,
					6786.635742
				};
				type = "FeedShack";
			};
			class DeerStand_26
			{
				name = "";
				position[] = {
					1350.683594,
					9858.787109
				};
				type = "DeerStand";
			};
			class FeedShack_23
			{
				name = "";
				position[] = {
					1386.137085,
					15012.594727
				};
				type = "FeedShack";
			};
			class DeerStand_27
			{
				name = "";
				position[] = {
					1491.477661,
					4759.689453
				};
				type = "DeerStand";
			};
			class DeerStand_28
			{
				name = "";
				position[] = {
					1443.860718,
					6559.951660
				};
				type = "DeerStand";
			};
			class DeerStand_29
			{
				name = "";
				position[] = {
					1487.256104,
					8567.621094
				};
				type = "DeerStand";
			};
			class DeerStand_30
			{
				name = "";
				position[] = {
					1460.334961,
					14897.563477
				};
				type = "DeerStand";
			};
			class DeerStand_31
			{
				name = "";
				position[] = {
					1581.829468,
					10646.833008
				};
				type = "DeerStand";
			};
			class DeerStand_32
			{
				name = "";
				position[] = {
					1676.656250,
					4699.439453
				};
				type = "DeerStand";
			};
			class FeedShack_24
			{
				name = "";
				position[] = {
					1667.447021,
					4932.444336
				};
				type = "FeedShack";
			};
			class DeerStand_33
			{
				name = "";
				position[] = {
					1740.953125,
					3235.361084
				};
				type = "DeerStand";
			};
			class FeedShack_25
			{
				name = "";
				position[] = {
					1797.985962,
					8055.370605
				};
				type = "FeedShack";
			};
			class FeedShack_26
			{
				name = "";
				position[] = {
					1770.884399,
					8434.260742
				};
				type = "FeedShack";
			};
			class DeerStand_34
			{
				name = "";
				position[] = {
					1805.223145,
					11432.714844
				};
				type = "DeerStand";
			};
			class DeerStand_35
			{
				name = "";
				position[] = {
					1918.333496,
					9409.086914
				};
				type = "DeerStand";
			};
			class DeerStand_36
			{
				name = "";
				position[] = {
					2029.426514,
					3048.326904
				};
				type = "DeerStand";
			};
			class DeerStand_37
			{
				name = "";
				position[] = {
					2022.291870,
					6598.076172
				};
				type = "DeerStand";
			};
			class FeedShack_27
			{
				name = "";
				position[] = {
					1987.264038,
					12965.860352
				};
				type = "FeedShack";
			};
			class FeedShack_28
			{
				name = "";
				position[] = {
					2057.730957,
					6277.836914
				};
				type = "FeedShack";
			};
			class DeerStand_38
			{
				name = "";
				position[] = {
					2092.778076,
					14762.103516
				};
				type = "DeerStand";
			};
			class DeerStand_39
			{
				name = "";
				position[] = {
					2112.390137,
					2630.348633
				};
				type = "DeerStand";
			};
			class DeerStand_40
			{
				name = "";
				position[] = {
					2192.453125,
					7547.806152
				};
				type = "DeerStand";
			};
			class DeerStand_41
			{
				name = "";
				position[] = {
					2166.572021,
					12707.178711
				};
				type = "DeerStand";
			};
			class FeedShack_29
			{
				name = "";
				position[] = {
					2171.252930,
					13767.171875
				};
				type = "FeedShack";
			};
			class DeerStand_42
			{
				name = "";
				position[] = {
					2271.819824,
					4283.489746
				};
				type = "DeerStand";
			};
			class DeerStand_43
			{
				name = "";
				position[] = {
					2276.884277,
					5970.969238
				};
				type = "DeerStand";
			};
			class DeerStand_44
			{
				name = "";
				position[] = {
					2298.498779,
					8493.967773
				};
				type = "DeerStand";
			};
			class FeedShack_30
			{
				name = "";
				position[] = {
					2394.461914,
					14753.761719
				};
				type = "FeedShack";
			};
			class DeerStand_45
			{
				name = "";
				position[] = {
					2491.123779,
					6882.457031
				};
				type = "DeerStand";
			};
			class DeerStand_46
			{
				name = "";
				position[] = {
					2489.629883,
					7680.270020
				};
				type = "DeerStand";
			};
			class FeedShack_31
			{
				name = "";
				position[] = {
					2501.253906,
					11130.858398
				};
				type = "FeedShack";
			};
			class FeedShack_32
			{
				name = "";
				position[] = {
					2543.643799,
					7593.268555
				};
				type = "FeedShack";
			};
			class FeedShack_33
			{
				name = "";
				position[] = {
					2563.898438,
					9221.776367
				};
				type = "FeedShack";
			};
			class DeerStand_47
			{
				name = "";
				position[] = {
					2538.370850,
					9381.547852
				};
				type = "DeerStand";
			};
			class DeerStand_48
			{
				name = "";
				position[] = {
					2577.137207,
					13904.271484
				};
				type = "DeerStand";
			};
			class DeerStand_49
			{
				name = "";
				position[] = {
					2626.663818,
					11336.510742
				};
				type = "DeerStand";
			};
			class DeerStand_50
			{
				name = "";
				position[] = {
					2737.955811,
					3598.014160
				};
				type = "DeerStand";
			};
			class FeedShack_34
			{
				name = "";
				position[] = {
					2747.542480,
					4226.286621
				};
				type = "FeedShack";
			};
			class FeedShack_35
			{
				name = "";
				position[] = {
					2813.126953,
					2482.899414
				};
				type = "FeedShack";
			};
			class FeedShack_36
			{
				name = "";
				position[] = {
					2974.195313,
					13591.541992
				};
				type = "FeedShack";
			};
			class DeerStand_51
			{
				name = "";
				position[] = {
					2943.977539,
					14412.017578
				};
				type = "DeerStand";
			};
			class DeerStand_52
			{
				name = "";
				position[] = {
					3118.239990,
					2397.147461
				};
				type = "DeerStand";
			};
			class FeedShack_37
			{
				name = "";
				position[] = {
					3063.564697,
					6581.652832
				};
				type = "FeedShack";
			};
			class DeerStand_53
			{
				name = "";
				position[] = {
					3228.164795,
					3600.866943
				};
				type = "DeerStand";
			};
			class DeerStand_54
			{
				name = "";
				position[] = {
					3214.305664,
					6287.803711
				};
				type = "DeerStand";
			};
			class DeerStand_55
			{
				name = "";
				position[] = {
					3199.400879,
					11591.539063
				};
				type = "DeerStand";
			};
			class DeerStand_56
			{
				name = "";
				position[] = {
					3282.512939,
					5767.630371
				};
				type = "DeerStand";
			};
			class FeedShack_38
			{
				name = "";
				position[] = {
					3273.123291,
					11624.182617
				};
				type = "FeedShack";
			};
			class DeerStand_57
			{
				name = "";
				position[] = {
					3360.879639,
					4284.316895
				};
				type = "DeerStand";
			};
			class DeerStand_58
			{
				name = "";
				position[] = {
					3418.327637,
					10188.814453
				};
				type = "DeerStand";
			};
			class DeerStand_59
			{
				name = "";
				position[] = {
					3374.056152,
					11019.997070
				};
				type = "DeerStand";
			};
			class DeerStand_60
			{
				name = "";
				position[] = {
					3423.884033,
					8144.595215
				};
				type = "DeerStand";
			};
			class DeerStand_61
			{
				name = "";
				position[] = {
					3443.354736,
					12136.321289
				};
				type = "DeerStand";
			};
			class DeerStand_62
			{
				name = "";
				position[] = {
					3654.335693,
					6260.979980
				};
				type = "DeerStand";
			};
			class DeerStand_63
			{
				name = "";
				position[] = {
					3603.958984,
					9367.570313
				};
				type = "DeerStand";
			};
			class FeedShack_39
			{
				name = "";
				position[] = {
					3614.812256,
					10572.548828
				};
				type = "FeedShack";
			};
			class DeerStand_64
			{
				name = "";
				position[] = {
					3746.576904,
					2979.056641
				};
				type = "DeerStand";
			};
			class DeerStand_65
			{
				name = "";
				position[] = {
					3818.073486,
					14120.452148
				};
				type = "DeerStand";
			};
			class DeerStand_66
			{
				name = "";
				position[] = {
					3871.458984,
					4434.831543
				};
				type = "DeerStand";
			};
			class DeerStand_67
			{
				name = "";
				position[] = {
					3965.750488,
					12130.792969
				};
				type = "DeerStand";
			};
			class DeerStand_68
			{
				name = "";
				position[] = {
					4079.576172,
					3865.762939
				};
				type = "DeerStand";
			};
			class DeerStand_69
			{
				name = "";
				position[] = {
					4081.335693,
					6016.446777
				};
				type = "DeerStand";
			};
			class DeerStand_70
			{
				name = "";
				position[] = {
					4111.559082,
					15170.727539
				};
				type = "DeerStand";
			};
			class FeedShack_40
			{
				name = "";
				position[] = {
					4145.294922,
					3819.479980
				};
				type = "FeedShack";
			};
			class DeerStand_71
			{
				name = "";
				position[] = {
					4221.718262,
					7087.982422
				};
				type = "DeerStand";
			};
			class DeerStand_72
			{
				name = "";
				position[] = {
					4315.516602,
					14287.283203
				};
				type = "DeerStand";
			};
			class DeerStand_73
			{
				name = "";
				position[] = {
					4351.984863,
					4973.549805
				};
				type = "DeerStand";
			};
			class FeedShack_41
			{
				name = "";
				position[] = {
					4364.224121,
					14671.948242
				};
				type = "FeedShack";
			};
			class DeerStand_74
			{
				name = "";
				position[] = {
					4393.419922,
					8058.583984
				};
				type = "DeerStand";
			};
			class FeedShack_42
			{
				name = "";
				position[] = {
					4389.111328,
					11896.748047
				};
				type = "FeedShack";
			};
			class DeerStand_75
			{
				name = "";
				position[] = {
					4414.087402,
					12237.998047
				};
				type = "DeerStand";
			};
			class FeedShack_43
			{
				name = "";
				position[] = {
					4444.734375,
					5074.045410
				};
				type = "FeedShack";
			};
			class FeedShack_44
			{
				name = "";
				position[] = {
					4544.919922,
					3130.607910
				};
				type = "FeedShack";
			};
			class DeerStand_76
			{
				name = "";
				position[] = {
					4719.491699,
					14976.774414
				};
				type = "DeerStand";
			};
			class DeerStand_77
			{
				name = "";
				position[] = {
					4700.047363,
					15234.972656
				};
				type = "DeerStand";
			};
			class DeerStand_78
			{
				name = "";
				position[] = {
					4826.613281,
					3318.208740
				};
				type = "DeerStand";
			};
			class FeedShack_45
			{
				name = "";
				position[] = {
					4839.227539,
					3845.394775
				};
				type = "FeedShack";
			};
			class DeerStand_79
			{
				name = "";
				position[] = {
					4838.276367,
					4611.082031
				};
				type = "DeerStand";
			};
			class DeerStand_80
			{
				name = "";
				position[] = {
					4852.212402,
					13584.428711
				};
				type = "DeerStand";
			};
			class DeerStand_81
			{
				name = "";
				position[] = {
					4882.743164,
					8150.267090
				};
				type = "DeerStand";
			};
			class DeerStand_82
			{
				name = "";
				position[] = {
					4880.121582,
					12475.937500
				};
				type = "DeerStand";
			};
			class DeerStand_83
			{
				name = "";
				position[] = {
					4951.245117,
					6380.975098
				};
				type = "DeerStand";
			};
			class DeerStand_84
			{
				name = "";
				position[] = {
					4998.564941,
					5068.760254
				};
				type = "DeerStand";
			};
			class FeedShack_46
			{
				name = "";
				position[] = {
					5026.826172,
					11275.820313
				};
				type = "FeedShack";
			};
			class DeerStand_85
			{
				name = "";
				position[] = {
					5071.281738,
					6279.514160
				};
				type = "DeerStand";
			};
			class FeedShack_47
			{
				name = "";
				position[] = {
					5097.835938,
					6386.399414
				};
				type = "FeedShack";
			};
			class DeerStand_86
			{
				name = "";
				position[] = {
					5209.119141,
					4085.263916
				};
				type = "DeerStand";
			};
			class FeedShack_48
			{
				name = "";
				position[] = {
					5263.221191,
					4857.933105
				};
				type = "FeedShack";
			};
			class DeerStand_87
			{
				name = "";
				position[] = {
					5276.738770,
					11899.777344
				};
				type = "DeerStand";
			};
			class DeerStand_88
			{
				name = "";
				position[] = {
					5329.666504,
					5795.321777
				};
				type = "DeerStand";
			};
			class DeerStand_89
			{
				name = "";
				position[] = {
					5338.433594,
					7360.754883
				};
				type = "DeerStand";
			};
			class FeedShack_49
			{
				name = "";
				position[] = {
					5298.087891,
					13590.939453
				};
				type = "FeedShack";
			};
			class FeedShack_50
			{
				name = "";
				position[] = {
					5346.172852,
					7893.019531
				};
				type = "FeedShack";
			};
			class DeerStand_90
			{
				name = "";
				position[] = {
					5466.619629,
					7888.142090
				};
				type = "DeerStand";
			};
			class DeerStand_91
			{
				name = "";
				position[] = {
					5543.330566,
					9563.018555
				};
				type = "DeerStand";
			};
			class DeerStand_92
			{
				name = "";
				position[] = {
					5535.759277,
					11251.676758
				};
				type = "DeerStand";
			};
			class DeerStand_93
			{
				name = "";
				position[] = {
					5520.566406,
					13834.319336
				};
				type = "DeerStand";
			};
			class DeerStand_94
			{
				name = "";
				position[] = {
					5564.586426,
					15084.688477
				};
				type = "DeerStand";
			};
			class DeerStand_95
			{
				name = "";
				position[] = {
					5669.751465,
					6843.336426
				};
				type = "DeerStand";
			};
			class FeedShack_51
			{
				name = "";
				position[] = {
					5682.586426,
					14211.780273
				};
				type = "FeedShack";
			};
			class DeerStand_96
			{
				name = "";
				position[] = {
					5646.145508,
					14309.628906
				};
				type = "DeerStand";
			};
			class DeerStand_97
			{
				name = "";
				position[] = {
					5869.347656,
					4303.317383
				};
				type = "DeerStand";
			};
			class DeerStand_98
			{
				name = "";
				position[] = {
					5921.166504,
					3466.402588
				};
				type = "DeerStand";
			};
			class DeerStand_99
			{
				name = "";
				position[] = {
					6085.942871,
					5033.987305
				};
				type = "DeerStand";
			};
			class DeerStand_100
			{
				name = "";
				position[] = {
					6115.563965,
					6129.095703
				};
				type = "DeerStand";
			};
			class FeedShack_52
			{
				name = "";
				position[] = {
					6155.048340,
					5688.335938
				};
				type = "FeedShack";
			};
			class DeerStand_101
			{
				name = "";
				position[] = {
					6144.721191,
					13133.608398
				};
				type = "DeerStand";
			};
			class FeedShack_53
			{
				name = "";
				position[] = {
					6187.657227,
					4090.872803
				};
				type = "FeedShack";
			};
			class DeerStand_102
			{
				name = "";
				position[] = {
					6227.561523,
					9750.394531
				};
				type = "DeerStand";
			};
			class FeedShack_54
			{
				name = "";
				position[] = {
					6204.862793,
					11372.897461
				};
				type = "FeedShack";
			};
			class DeerStand_103
			{
				name = "";
				position[] = {
					6213.135742,
					11360.179688
				};
				type = "DeerStand";
			};
			class FeedShack_55
			{
				name = "";
				position[] = {
					6220.269531,
					15337.155273
				};
				type = "FeedShack";
			};
			class DeerStand_104
			{
				name = "";
				position[] = {
					6296.671387,
					7822.172852
				};
				type = "DeerStand";
			};
			class DeerStand_105
			{
				name = "";
				position[] = {
					6250.131836,
					12220.539063
				};
				type = "DeerStand";
			};
			class DeerStand_106
			{
				name = "";
				position[] = {
					6290.256836,
					14224.719727
				};
				type = "DeerStand";
			};
			class DeerStand_107
			{
				name = "";
				position[] = {
					6332.484375,
					7949.256348
				};
				type = "DeerStand";
			};
			class DeerStand_108
			{
				name = "";
				position[] = {
					6376.416992,
					6573.804688
				};
				type = "DeerStand";
			};
			class DeerStand_109
			{
				name = "";
				position[] = {
					6375.043457,
					7788.135742
				};
				type = "DeerStand";
			};
			class DeerStand_110
			{
				name = "";
				position[] = {
					6434.425293,
					8473.619141
				};
				type = "DeerStand";
			};
			class DeerStand_111
			{
				name = "";
				position[] = {
					6579.566895,
					4391.391602
				};
				type = "DeerStand";
			};
			class DeerStand_112
			{
				name = "";
				position[] = {
					6601.274414,
					15192.606445
				};
				type = "DeerStand";
			};
			class FeedShack_56
			{
				name = "";
				position[] = {
					6682.371094,
					6772.425781
				};
				type = "FeedShack";
			};
			class DeerStand_113
			{
				name = "";
				position[] = {
					6696.359863,
					8429.767578
				};
				type = "DeerStand";
			};
			class FeedShack_57
			{
				name = "";
				position[] = {
					6703.775879,
					13709.308594
				};
				type = "FeedShack";
			};
			class DeerStand_114
			{
				name = "";
				position[] = {
					6736.159180,
					5409.023438
				};
				type = "DeerStand";
			};
			class DeerStand_115
			{
				name = "";
				position[] = {
					6784.255371,
					7184.154297
				};
				type = "DeerStand";
			};
			class DeerStand_116
			{
				name = "";
				position[] = {
					6798.009766,
					12984.000977
				};
				type = "DeerStand";
			};
			class DeerStand_117
			{
				name = "";
				position[] = {
					6815.090820,
					13603.063477
				};
				type = "DeerStand";
			};
			class DeerStand_118
			{
				name = "";
				position[] = {
					6856.548340,
					8004.646973
				};
				type = "DeerStand";
			};
			class FeedShack_58
			{
				name = "";
				position[] = {
					6889.029297,
					10862.518555
				};
				type = "FeedShack";
			};
			class FeedShack_59
			{
				name = "";
				position[] = {
					6860.779785,
					15051.707031
				};
				type = "FeedShack";
			};
			class FeedShack_60
			{
				name = "";
				position[] = {
					6997.190430,
					4549.720215
				};
				type = "FeedShack";
			};
			class DeerStand_119
			{
				name = "";
				position[] = {
					7071.360840,
					9943.950195
				};
				type = "DeerStand";
			};
			class FeedShack_61
			{
				name = "";
				position[] = {
					7024.971680,
					13569.238281
				};
				type = "FeedShack";
			};
			class FeedShack_62
			{
				name = "";
				position[] = {
					7088.251465,
					9160.886719
				};
				type = "FeedShack";
			};
			class DeerStand_120
			{
				name = "";
				position[] = {
					7139.668945,
					11959.412109
				};
				type = "DeerStand";
			};
			class FeedShack_63
			{
				name = "";
				position[] = {
					7101.616699,
					12359.934570
				};
				type = "FeedShack";
			};
			class DeerStand_121
			{
				name = "";
				position[] = {
					7180.401855,
					14000.493164
				};
				type = "DeerStand";
			};
			class DeerStand_122
			{
				name = "";
				position[] = {
					7235.306641,
					5952.166992
				};
				type = "DeerStand";
			};
			class DeerStand_123
			{
				name = "";
				position[] = {
					7236.671875,
					10704.550781
				};
				type = "DeerStand";
			};
			class FeedShack_64
			{
				name = "";
				position[] = {
					7428.102051,
					12732.724609
				};
				type = "FeedShack";
			};
			class DeerStand_124
			{
				name = "";
				position[] = {
					7463.298340,
					6770.593750
				};
				type = "DeerStand";
			};
			class FeedShack_65
			{
				name = "";
				position[] = {
					7499.933105,
					11747.946289
				};
				type = "FeedShack";
			};
			class DeerStand_125
			{
				name = "";
				position[] = {
					7619.321777,
					5407.146973
				};
				type = "DeerStand";
			};
			class FeedShack_66
			{
				name = "";
				position[] = {
					7635.484375,
					6637.422363
				};
				type = "FeedShack";
			};
			class FeedShack_67
			{
				name = "";
				position[] = {
					7652.499512,
					11186.456055
				};
				type = "FeedShack";
			};
			class DeerStand_126
			{
				name = "";
				position[] = {
					7626.868652,
					11781.449219
				};
				type = "DeerStand";
			};
			class DeerStand_127
			{
				name = "";
				position[] = {
					7696.034180,
					8689.221680
				};
				type = "DeerStand";
			};
			class DeerStand_128
			{
				name = "";
				position[] = {
					7926.215332,
					13360.151367
				};
				type = "DeerStand";
			};
			class DeerStand_129
			{
				name = "";
				position[] = {
					8035.270020,
					14131.938477
				};
				type = "DeerStand";
			};
			class DeerStand_130
			{
				name = "";
				position[] = {
					8079.486328,
					4485.928711
				};
				type = "DeerStand";
			};
			class DeerStand_131
			{
				name = "";
				position[] = {
					8078.722168,
					15145.269531
				};
				type = "DeerStand";
			};
			class DeerStand_132
			{
				name = "";
				position[] = {
					8143.494141,
					5039.329102
				};
				type = "DeerStand";
			};
			class DeerStand_133
			{
				name = "";
				position[] = {
					8115.218750,
					5753.802734
				};
				type = "DeerStand";
			};
			class DeerStand_134
			{
				name = "";
				position[] = {
					8135.917969,
					6808.039063
				};
				type = "DeerStand";
			};
			class FeedShack_68
			{
				name = "";
				position[] = {
					8118.465820,
					8237.879883
				};
				type = "FeedShack";
			};
			class FeedShack_69
			{
				name = "";
				position[] = {
					8171.707031,
					5446.848145
				};
				type = "FeedShack";
			};
			class DeerStand_135
			{
				name = "";
				position[] = {
					8194.226563,
					7708.655762
				};
				type = "DeerStand";
			};
			class FeedShack_70
			{
				name = "";
				position[] = {
					8209.609375,
					14276.988281
				};
				type = "FeedShack";
			};
			class DeerStand_136
			{
				name = "";
				position[] = {
					8291.768555,
					9722.355469
				};
				type = "DeerStand";
			};
			class DeerStand_137
			{
				name = "";
				position[] = {
					8337.324219,
					10673.748047
				};
				type = "DeerStand";
			};
			class DeerStand_138
			{
				name = "";
				position[] = {
					8368.446289,
					11439.750977
				};
				type = "DeerStand";
			};
			class DeerStand_139
			{
				name = "";
				position[] = {
					8406.135742,
					3301.346924
				};
				type = "DeerStand";
			};
			class DeerStand_140
			{
				name = "";
				position[] = {
					8404.897461,
					8159.793945
				};
				type = "DeerStand";
			};
			class FeedShack_71
			{
				name = "";
				position[] = {
					8414.364258,
					10533.499023
				};
				type = "FeedShack";
			};
			class DeerStand_141
			{
				name = "";
				position[] = {
					8480.406250,
					6131.102051
				};
				type = "DeerStand";
			};
			class FeedShack_72
			{
				name = "";
				position[] = {
					8546.187500,
					4394.577148
				};
				type = "FeedShack";
			};
			class FeedShack_73
			{
				name = "";
				position[] = {
					8633.569336,
					12096.212891
				};
				type = "FeedShack";
			};
			class DeerStand_142
			{
				name = "";
				position[] = {
					8676.989258,
					12304.579102
				};
				type = "DeerStand";
			};
			class DeerStand_143
			{
				name = "";
				position[] = {
					8659.514648,
					14297.824219
				};
				type = "DeerStand";
			};
			class FeedShack_74
			{
				name = "";
				position[] = {
					8701.784180,
					8780.086914
				};
				type = "FeedShack";
			};
			class DeerStand_144
			{
				name = "";
				position[] = {
					8805.170898,
					10479.630859
				};
				type = "DeerStand";
			};
			class DeerStand_145
			{
				name = "";
				position[] = {
					8771.007813,
					15021.982422
				};
				type = "DeerStand";
			};
			class DeerStand_146
			{
				name = "";
				position[] = {
					8823.190430,
					4197.714355
				};
				type = "DeerStand";
			};
			class DeerStand_147
			{
				name = "";
				position[] = {
					8844.287109,
					6457.552734
				};
				type = "DeerStand";
			};
			class FeedShack_75
			{
				name = "";
				position[] = {
					8855.451172,
					6983.576660
				};
				type = "FeedShack";
			};
			class DeerStand_148
			{
				name = "";
				position[] = {
					8824.827148,
					7566.711914
				};
				type = "DeerStand";
			};
			class DeerStand_149
			{
				name = "";
				position[] = {
					8826.719727,
					8649.653320
				};
				type = "DeerStand";
			};
			class FeedShack_76
			{
				name = "";
				position[] = {
					8841.281250,
					13877.817383
				};
				type = "FeedShack";
			};
			class DeerStand_150
			{
				name = "";
				position[] = {
					8895.172852,
					5322.286621
				};
				type = "DeerStand";
			};
			class FeedShack_77
			{
				name = "";
				position[] = {
					9127.639648,
					3101.748291
				};
				type = "FeedShack";
			};
			class FeedShack_78
			{
				name = "";
				position[] = {
					9177.203125,
					7769.825195
				};
				type = "FeedShack";
			};
			class DeerStand_151
			{
				name = "";
				position[] = {
					9163.377930,
					15016.912109
				};
				type = "DeerStand";
			};
			class DeerStand_152
			{
				name = "";
				position[] = {
					9186.093750,
					8289.341797
				};
				type = "DeerStand";
			};
			class DeerStand_153
			{
				name = "";
				position[] = {
					9332.737305,
					3373.754639
				};
				type = "DeerStand";
			};
			class DeerStand_154
			{
				name = "";
				position[] = {
					9325.957031,
					7220.442383
				};
				type = "DeerStand";
			};
			class FeedShack_79
			{
				name = "";
				position[] = {
					9303.292969,
					12043.563477
				};
				type = "FeedShack";
			};
			class FeedShack_80
			{
				name = "";
				position[] = {
					9311.432617,
					15032.320313
				};
				type = "FeedShack";
			};
			class FeedShack_81
			{
				name = "";
				position[] = {
					9429.698242,
					5824.308594
				};
				type = "FeedShack";
			};
			class DeerStand_155
			{
				name = "";
				position[] = {
					9453.714844,
					7718.005371
				};
				type = "DeerStand";
			};
			class DeerStand_156
			{
				name = "";
				position[] = {
					9535.524414,
					2705.531738
				};
				type = "DeerStand";
			};
			class DeerStand_157
			{
				name = "";
				position[] = {
					9502.707031,
					9051.148438
				};
				type = "DeerStand";
			};
			class DeerStand_158
			{
				name = "";
				position[] = {
					9548.324219,
					12634.149414
				};
				type = "DeerStand";
			};
			class DeerStand_159
			{
				name = "";
				position[] = {
					9626.601563,
					6258.805176
				};
				type = "DeerStand";
			};
			class FeedShack_82
			{
				name = "";
				position[] = {
					9656.128906,
					9187.786133
				};
				type = "FeedShack";
			};
			class DeerStand_160
			{
				name = "";
				position[] = {
					9665.511719,
					3669.480713
				};
				type = "DeerStand";
			};
			class DeerStand_161
			{
				name = "";
				position[] = {
					9665.866211,
					8331.416016
				};
				type = "DeerStand";
			};
			class DeerStand_162
			{
				name = "";
				position[] = {
					9689.205078,
					14663.023438
				};
				type = "DeerStand";
			};
			class DeerStand_163
			{
				name = "";
				position[] = {
					9769.472656,
					2516.266602
				};
				type = "DeerStand";
			};
			class DeerStand_164
			{
				name = "";
				position[] = {
					9728.222656,
					4496.684082
				};
				type = "DeerStand";
			};
			class DeerStand_165
			{
				name = "";
				position[] = {
					9763.421875,
					11714.376953
				};
				type = "DeerStand";
			};
			class DeerStand_166
			{
				name = "";
				position[] = {
					9832.164063,
					14217.416016
				};
				type = "DeerStand";
			};
			class DeerStand_167
			{
				name = "";
				position[] = {
					9811.542969,
					15234.364258
				};
				type = "DeerStand";
			};
			class FeedShack_83
			{
				name = "";
				position[] = {
					9884.574219,
					4360.161621
				};
				type = "FeedShack";
			};
			class DeerStand_168
			{
				name = "";
				position[] = {
					9855.334961,
					7239.017090
				};
				type = "DeerStand";
			};
			class DeerStand_169
			{
				name = "";
				position[] = {
					9864.711914,
					13456.027344
				};
				type = "DeerStand";
			};
			class DeerStand_170
			{
				name = "";
				position[] = {
					9945.505859,
					3192.294434
				};
				type = "DeerStand";
			};
			class DeerStand_171
			{
				name = "";
				position[] = {
					9919.722656,
					5257.385742
				};
				type = "DeerStand";
			};
			class DeerStand_172
			{
				name = "";
				position[] = {
					9914.376953,
					9632.333984
				};
				type = "DeerStand";
			};
			class FeedShack_84
			{
				name = "";
				position[] = {
					9940.592773,
					15282.304688
				};
				type = "FeedShack";
			};
			class DeerStand_173
			{
				name = "";
				position[] = {
					9979.723633,
					8973.899414
				};
				type = "DeerStand";
			};
			class DeerStand_174
			{
				name = "";
				position[] = {
					10085.732422,
					6696.156250
				};
				type = "DeerStand";
			};
			class FeedShack_85
			{
				name = "";
				position[] = {
					10143.283203,
					6752.474609
				};
				type = "FeedShack";
			};
			class DeerStand_175
			{
				name = "";
				position[] = {
					10227.559570,
					8431.295898
				};
				type = "DeerStand";
			};
			class FeedShack_86
			{
				name = "";
				position[] = {
					10297.117188,
					7458.819824
				};
				type = "FeedShack";
			};
			class DeerStand_176
			{
				name = "";
				position[] = {
					10333.323242,
					10130.712891
				};
				type = "DeerStand";
			};
			class DeerStand_177
			{
				name = "";
				position[] = {
					10322.300781,
					14695.112305
				};
				type = "DeerStand";
			};
			class DeerStand_178
			{
				name = "";
				position[] = {
					10383.368164,
					6011.972656
				};
				type = "DeerStand";
			};
			class FeedShack_87
			{
				name = "";
				position[] = {
					10482.507813,
					11750.060547
				};
				type = "FeedShack";
			};
			class DeerStand_179
			{
				name = "";
				position[] = {
					10443.572266,
					12495.661133
				};
				type = "DeerStand";
			};
			class FeedShack_88
			{
				name = "";
				position[] = {
					10561.745117,
					12798.185547
				};
				type = "FeedShack";
			};
			class DeerStand_180
			{
				name = "";
				position[] = {
					10663.932617,
					5224.531738
				};
				type = "DeerStand";
			};
			class FeedShack_89
			{
				name = "";
				position[] = {
					10643.495117,
					6059.497559
				};
				type = "FeedShack";
			};
			class DeerStand_181
			{
				name = "";
				position[] = {
					10635.291016,
					10921.458984
				};
				type = "DeerStand";
			};
			class DeerStand_182
			{
				name = "";
				position[] = {
					10704.666992,
					5836.669434
				};
				type = "DeerStand";
			};
			class FeedShack_90
			{
				name = "";
				position[] = {
					10783.258789,
					3645.513916
				};
				type = "FeedShack";
			};
			class DeerStand_183
			{
				name = "";
				position[] = {
					10797.969727,
					13557.957031
				};
				type = "DeerStand";
			};
			class DeerStand_184
			{
				name = "";
				position[] = {
					10785.374023,
					14904.295898
				};
				type = "DeerStand";
			};
			class DeerStand_185
			{
				name = "";
				position[] = {
					10855.990234,
					3497.844482
				};
				type = "DeerStand";
			};
			class DeerStand_186
			{
				name = "";
				position[] = {
					10858.775391,
					4244.750000
				};
				type = "DeerStand";
			};
			class FeedShack_91
			{
				name = "";
				position[] = {
					10877.514648,
					4134.331055
				};
				type = "FeedShack";
			};
			class DeerStand_187
			{
				name = "";
				position[] = {
					10898.418945,
					6443.583984
				};
				type = "DeerStand";
			};
			class FeedShack_92
			{
				name = "";
				position[] = {
					10873.352539,
					10268.761719
				};
				type = "FeedShack";
			};
			class FeedShack_93
			{
				name = "";
				position[] = {
					10886.481445,
					14709.836914
				};
				type = "FeedShack";
			};
			class FeedShack_94
			{
				name = "";
				position[] = {
					10967.703125,
					8591.667969
				};
				type = "FeedShack";
			};
			class DeerStand_188
			{
				name = "";
				position[] = {
					11201.738281,
					7260.879883
				};
				type = "DeerStand";
			};
			class DeerStand_189
			{
				name = "";
				position[] = {
					11160.273438,
					9590.683594
				};
				type = "DeerStand";
			};
			class FeedShack_95
			{
				name = "";
				position[] = {
					11167.938477,
					9848.037109
				};
				type = "FeedShack";
			};
			class DeerStand_190
			{
				name = "";
				position[] = {
					11237.152344,
					5624.030273
				};
				type = "DeerStand";
			};
			class FeedShack_96
			{
				name = "";
				position[] = {
					11234.224609,
					13647.419922
				};
				type = "FeedShack";
			};
			class DeerStand_191
			{
				name = "";
				position[] = {
					11326.162109,
					8057.800781
				};
				type = "DeerStand";
			};
			class DeerStand_192
			{
				name = "";
				position[] = {
					11339.577148,
					9193.647461
				};
				type = "DeerStand";
			};
			class DeerStand_193
			{
				name = "";
				position[] = {
					11358.840820,
					10188.303711
				};
				type = "DeerStand";
			};
			class FeedShack_97
			{
				name = "";
				position[] = {
					11364.681641,
					11773.545898
				};
				type = "FeedShack";
			};
			class DeerStand_194
			{
				name = "";
				position[] = {
					11448.268555,
					5127.837402
				};
				type = "DeerStand";
			};
			class FeedShack_98
			{
				name = "";
				position[] = {
					11575.989258,
					4676.304688
				};
				type = "FeedShack";
			};
			class DeerStand_195
			{
				name = "";
				position[] = {
					11535.919922,
					13324.779297
				};
				type = "DeerStand";
			};
			class DeerStand_196
			{
				name = "";
				position[] = {
					11609.091797,
					3733.312012
				};
				type = "DeerStand";
			};
			class DeerStand_197
			{
				name = "";
				position[] = {
					11687.974609,
					6963.618164
				};
				type = "DeerStand";
			};
			class DeerStand_198
			{
				name = "";
				position[] = {
					11707.245117,
					4435.037109
				};
				type = "DeerStand";
			};
			class FeedShack_99
			{
				name = "";
				position[] = {
					11756.562500,
					5805.470703
				};
				type = "FeedShack";
			};
			class DeerStand_199
			{
				name = "";
				position[] = {
					11750.498047,
					6317.567871
				};
				type = "DeerStand";
			};
			class FeedShack_100
			{
				name = "";
				position[] = {
					11733.570313,
					7100.343750
				};
				type = "FeedShack";
			};
			class FeedShack_101
			{
				name = "";
				position[] = {
					11816.871094,
					4013.440430
				};
				type = "FeedShack";
			};
			class DeerStand_200
			{
				name = "";
				position[] = {
					11819.041016,
					8417.521484
				};
				type = "DeerStand";
			};
			class DeerStand_201
			{
				name = "";
				position[] = {
					11868.572266,
					15252.761719
				};
				type = "DeerStand";
			};
			class DeerStand_202
			{
				name = "";
				position[] = {
					11887.834961,
					9545.048828
				};
				type = "DeerStand";
			};
			class DeerStand_203
			{
				name = "";
				position[] = {
					11942.659180,
					10835.887695
				};
				type = "DeerStand";
			};
			class FeedShack_102
			{
				name = "";
				position[] = {
					12073.034180,
					5111.582031
				};
				type = "FeedShack";
			};
			class FeedShack_103
			{
				name = "";
				position[] = {
					12130.356445,
					5451.783691
				};
				type = "FeedShack";
			};
			class FeedShack_104
			{
				name = "";
				position[] = {
					12128.536133,
					11189.931641
				};
				type = "FeedShack";
			};
			class FeedShack_105
			{
				name = "";
				position[] = {
					12178.150391,
					14802.255859
				};
				type = "FeedShack";
			};
			class DeerStand_204
			{
				name = "";
				position[] = {
					12251.652344,
					5433.676270
				};
				type = "DeerStand";
			};
			class DeerStand_205
			{
				name = "";
				position[] = {
					12263.446289,
					8735.812500
				};
				type = "DeerStand";
			};
			class DeerStand_206
			{
				name = "";
				position[] = {
					12289.343750,
					11441.018555
				};
				type = "DeerStand";
			};
			class DeerStand_207
			{
				name = "";
				position[] = {
					12345.380859,
					6924.681641
				};
				type = "DeerStand";
			};
			class FeedShack_106
			{
				name = "";
				position[] = {
					12378.531250,
					7740.572754
				};
				type = "FeedShack";
			};
			class DeerStand_208
			{
				name = "";
				position[] = {
					12372.088867,
					15202.404297
				};
				type = "DeerStand";
			};
			class DeerStand_209
			{
				name = "";
				position[] = {
					12463.246094,
					4638.870605
				};
				type = "DeerStand";
			};
			class DeerStand_210
			{
				name = "";
				position[] = {
					12582.752930,
					7388.138672
				};
				type = "DeerStand";
			};
			class FeedShack_107
			{
				name = "";
				position[] = {
					12619.966797,
					6673.754883
				};
				type = "FeedShack";
			};
			class DeerStand_211
			{
				name = "";
				position[] = {
					12635.895508,
					12804.073242
				};
				type = "DeerStand";
			};
			class FeedShack_108
			{
				name = "";
				position[] = {
					12624.722656,
					14528.345703
				};
				type = "FeedShack";
			};
			class DeerStand_212
			{
				name = "";
				position[] = {
					12719.810547,
					6323.478027
				};
				type = "DeerStand";
			};
			class DeerStand_213
			{
				name = "";
				position[] = {
					12728.170898,
					5133.476074
				};
				type = "DeerStand";
			};
			class DeerStand_214
			{
				name = "";
				position[] = {
					12761.924805,
					7721.466797
				};
				type = "DeerStand";
			};
			class FeedShack_109
			{
				name = "";
				position[] = {
					12818.688477,
					13117.197266
				};
				type = "FeedShack";
			};
			class DeerStand_215
			{
				name = "";
				position[] = {
					12867.194336,
					10430.960938
				};
				type = "DeerStand";
			};
			class FeedShack_110
			{
				name = "";
				position[] = {
					12890.481445,
					10460.095703
				};
				type = "FeedShack";
			};
			class DeerStand_216
			{
				name = "";
				position[] = {
					12935.910156,
					12241.822266
				};
				type = "DeerStand";
			};
			class DeerStand_217
			{
				name = "";
				position[] = {
					13254.438477,
					12791.208984
				};
				type = "DeerStand";
			};
			class DeerStand_218
			{
				name = "";
				position[] = {
					13200.933594,
					13232.412109
				};
				type = "DeerStand";
			};
			class FeedShack_111
			{
				name = "";
				position[] = {
					13331.621094,
					5062.051758
				};
				type = "FeedShack";
			};
			class DeerStand_219
			{
				name = "";
				position[] = {
					13353.467773,
					15078.259766
				};
				type = "DeerStand";
			};
			class FeedShack_112
			{
				name = "";
				position[] = {
					13381.250977,
					12246.166992
				};
				type = "FeedShack";
			};
			class DeerStand_220
			{
				name = "";
				position[] = {
					13452.052734,
					11908.126953
				};
				type = "DeerStand";
			};
			class FeedShack_113
			{
				name = "";
				position[] = {
					13462.884766,
					12455.708984
				};
				type = "FeedShack";
			};
			class DeerStand_221
			{
				name = "";
				position[] = {
					13616.199219,
					3089.462891
				};
				type = "DeerStand";
			};
			class DeerStand_222
			{
				name = "";
				position[] = {
					13646.508789,
					14477.600586
				};
				type = "DeerStand";
			};
			class FeedShack_114
			{
				name = "";
				position[] = {
					13746.810547,
					14415.497070
				};
				type = "FeedShack";
			};
			class DeerStand_223
			{
				name = "";
				position[] = {
					13953.609375,
					14845.188477
				};
				type = "DeerStand";
			};
			class DeerStand_224
			{
				name = "";
				position[] = {
					14386.710938,
					15063.619141
				};
				type = "DeerStand";
			};
			class FeedShack_115
			{
				name = "";
				position[] = {
					14422.506836,
					14289.826172
				};
				type = "FeedShack";
			};
			class FeedShack_116
			{
				name = "";
				position[] = {
					14645.491211,
					14643.556641
				};
				type = "FeedShack";
			};
			class DeerStand_225
			{
				name = "";
				position[] = {
					14844.644531,
					14073.151367
				};
				type = "DeerStand";
			};
			class FeedShack_117
			{
				name = "";
				position[] = {
					14917.345703,
					14257.615234
				};
				type = "FeedShack";
			};
			class DeerStand_226
			{
				name = "";
				position[] = {
					15134.531250,
					14788.875977
				};
				type = "DeerStand";
			};
		};
	};
};
