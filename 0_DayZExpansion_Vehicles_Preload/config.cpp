#define _ARMA_

class CfgPatches
{
	class DayZExpansion_Vehicles_Preload
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {};
	};
};
class CfgMods
{
	class DZ_Expansion_Vehicles_Preload
	{
		type = "mod";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class engineScriptModule
			{
				files[] = {"0_DayZExpansion_Vehicles_Preload/Common","0_DayZExpansion_Vehicles_Preload/1_Core"};
			};
			class gameLibScriptModule
			{
				files[] = {"0_DayZExpansion_Vehicles_Preload/Common","0_DayZExpansion_Vehicles_Preload/2_GameLib"};
			};
			class gameScriptModule
			{
				files[] = {"0_DayZExpansion_Vehicles_Preload/Common","0_DayZExpansion_Vehicles_Preload/3_Game"};
			};
			class worldScriptModule
			{
				files[] = {"0_DayZExpansion_Vehicles_Preload/Common","0_DayZExpansion_Vehicles_Preload/4_World"};
			};
			class missionScriptModule
			{
				files[] = {"0_DayZExpansion_Vehicles_Preload/Common","0_DayZExpansion_Vehicles_Preload/5_Mission"};
			};
		};
	};
};
