/**
 * ExpansionNamalskModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionNamalskQuestHolder
{
	int ID;
	string ClassName; 
	ref array<ref ExpansionNamalskQuestHolderPosition> Positions = new array<ref ExpansionNamalskQuestHolderPosition>;
	bool IsStatic;
	string Name;
	string DefaultText;
	
	void ExpansionNamalskQuestHolder(int id, string className, bool isStatic = true, string name = "Unknown", string text = "There is nothing to do here for you...")
	{
		ClassName = className;
		IsStatic = isStatic;
		Name = name;
		DefaultText = text;
	}
	
	void AddPosition(ExpansionNamalskQuestHolderPosition pos)
	{
		Positions.Insert(pos);
	}
};

class ExpansionNamalskQuestHolderPosition
{
	vector Position;
	vector Orientation;
	
	void ExpansionNamalskQuestHolderPosition(vector pos, vector ori)
	{
		Position = pos;
		Orientation = ori;
	}
};

[CF_RegisterModule(ExpansionNamalskModule)]
class ExpansionNamalskModule: CF_ModuleWorld
{
	protected static ExpansionNamalskModule s_ModuleInstance;
	static string s_dataFolderPath = "$mission:expansion\\namalsk_adventure\\";

	protected ref ExpansionNamalskAdventureData m_ServerData; //! Server
#ifdef EXPANSIONMODNAVIGATION
	protected ExpansionMarkerData m_MerchantServerMarker;
#endif

#ifdef EXPANSIONMODAI
	protected ref map<eAIBase, ref array<vector>> m_SpawnedAI;
	protected int m_AISpeed;
#ifdef EXPANSIONMODQUESTS
	protected ref map<eAIBase, ref array<vector>> m_QuestAI;
#endif
#endif

	protected typename m_LastNamalskEvent;

	protected bool m_EVRStormActive;
	protected bool m_EVRStormBlowout;

#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL
	protected SV_Abandoned_Sattelite_Antenna m_AbdonedSatellite;
	protected Expansion_Teleporter_Big m_SatelliteTeleporter;
	protected Expansion_Satellite_Control m_SatelliteController;
	
	protected const float SATELLITE_CRY_TIME = 300.0; //! 5 minutes tick
	protected float m_SatelliteCryTimer;
	protected bool m_SatelitteState;
	
	protected const float CLIENT_UPDATE_TIME = 5.0; //! 5 seconds tick
	protected float m_ClientUpdateTimer;
	
	protected const float BUNKER_GENERATORS_CHECK_TIME = 10.0; //! 10 seconds tick
	protected float m_BunkerGeneratorsCheckTimer;
	protected static ScriptCaller s_BunkerGeneratorCheckSC;
	static const int BUNKER_GENERATOR_DEACTIVATION_TIME = 300.0; //! 5 minute delay.
	
	protected int m_CurrentCheckTime;
	
	protected const vector m_A1_Bunker_EntrancePos = Vector(4229.486328, 81.322319, 8218.904297);

	protected const vector m_A1_Bunker_GeneratorPos = "2979.432373 17.282696 8674.547852";
	protected const vector m_A1_Bunker_GeneratorOri = "41.105145 0.000000 0.000000";
	
	protected const vector m_A1_Bunker_TeleporterPos = "3006.558594 5.375517 8639.664063";
	protected const vector m_A1_Bunker_TeleporterOri = "135.317398 0.000000 -0.000000";
	
	protected const vector m_A1_Bunker_EntrancePanelPos = "4226.144531 82.264626 8193.572266";
	protected const vector m_A1_Bunker_EntrancePanelOri = "-7.753430 -0.000000 -0.000000";
	
	protected const vector m_A1_Bunker_EntranceLeaverPos = "4231.801758 81.996811 8198.053711";
	protected const vector m_A1_Bunker_EntranceLeaverOri = "-97.819839 -0.000000 -0.000000";
	
	protected const vector m_A1_Bunker_FakeEntranceLeaverPos = "2985.023438 31.515968 8724.857422";
	protected const vector m_A1_Bunker_FakeEntranceLeaverOri = "133.101044 0.000000 -0.000000";
	
	protected Expansion_Teleporter_Big m_A1BunkerTeleporter;
	protected Expansion_Bunker_Generator m_A1BungerGenerator;
	
	protected Land_Underground_Panel m_A1BunkerEntrancePanel;
	protected Land_Underground_Panel_Lever m_A1BunkerEntranceLeaver;
	protected Land_Underground_Panel_Lever m_A1BunkerFakeEntranceLeaver;
	
	protected ref array<ref ExpansionNamalskQuestHolder> m_QuestHolders;
#endif
	
	protected const float SUPPLY_CRATES_CHECK_TIME = 60.0; //! 1 minute tick
	protected float m_SupplyCratesCheckTimer;
	protected static ScriptCaller s_SupplyCratesCheckSC;
	static const int LOOT_DELAY_TIME = 300; //! 5 minute delay.
	
	void ExpansionNamalskModule()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		s_ModuleInstance = this;

	#ifdef EXPANSIONMODAI
		m_SpawnedAI = new map<eAIBase, ref array<vector>>;
	#ifdef EXPANSIONMODQUESTS
		m_QuestAI = new map<eAIBase, ref array<vector>>;
	#endif
	#endif
	}

	override void OnInit()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		super.OnInit();

		EnableMissionStart();
		EnableMissionLoaded();
		EnableUpdate();
	}

	protected void CreateDirectoryStructure()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		if (!FileExist(s_dataFolderPath))
			ExpansionStatic.MakeDirectoryRecursive(s_dataFolderPath);
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		super.OnMissionLoaded(sender, args);

		//! Server only
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			CreateDirectoryStructure(); //! Create directoy structure if not existing.
			LoadNamalskAdventureServerData(); //! Load server data.
		}
	}

	protected void LoadNamalskAdventureServerData()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		array<string> files = ExpansionStatic.FindFilesInLocation(s_dataFolderPath, ".json");
		if (files && files.Count() > 0)
		{
			for (int i = 0; i < files.Count(); i++)
			{
				string fileName = files[i];
				GetServerData(fileName, s_dataFolderPath);
			}
		}
		else
		{
			m_ServerData = new ExpansionNamalskAdventureData();
			m_ServerData.Save();
		}
	}

	protected void GetServerData(string fileName, string path)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		m_ServerData = ExpansionNamalskAdventureData.Load(path + fileName);
		if (!m_ServerData)
			Error(ToString() + "::GetServerData - Could not get namalsk adventure server data!");
	}

	override void OnMissionLoaded(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		super.OnMissionLoaded(sender, args);

		//! Server only
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
		#ifdef EXPANSIONMODAI
			if (GetExpansionSettings().GetNamalskAdventure().EnableAISpawns)
				SpawnAI();
		#endif
		#ifdef EXPANSIONMODMARKET
			if (GetExpansionSettings().GetNamalskAdventure().EnableMerchant)
				CreateMerchant();
		#endif
			
		#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL
			SpawnSatelliteAntennaObjects();			
			SpawnA1Bunker();
		#endif
			
			if (GetExpansionSettings().GetNamalskAdventure().EnableSupplyCrates)
				SpawnSupplyCrates();
		}
	}

#ifdef EXPANSIONMODAI
	//! @note: Spawns all configured AI units from the NamalskAdventureSettings class.
	protected void SpawnAI()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		for (int i = 0; i < GetExpansionSettings().GetNamalskAdventure().AISpawnPositions.Count(); i++)
		{
			ExpansionAISpawnPosition aiSpawn = GetExpansionSettings().GetNamalskAdventure().AISpawnPositions[i];
			if (!aiSpawn)
				continue;

			SpawnSingleAI(aiSpawn);
		}
	}

	//! @note: Handles spawns of AI units.
	protected void SpawnSingleAI(ExpansionAISpawnPosition aiSpawn)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		vector pos = ExpansionAIPatrol.GetPlacementPosition(aiSpawn.Position);

		eAIBase ai;
		if (!Class.CastTo(ai, GetGame().CreateObject(GetRandomAI(), pos)))
			return;

		if (!ai.m_Expansion_NetsyncData)
			ai.m_Expansion_NetsyncData = new ExpansionNetsyncData(ai);

		if (ai.m_Expansion_NetsyncData)
		{
			ai.m_Expansion_NetsyncData.Set(0, aiSpawn.NPCName);
			ai.m_Expansion_NetsyncData.Set(1, "{3364F58EF7F7FBE3}DayZExpansion/Core/GUI/icons/misc/T_Soldier_256x256.edds");
		}

		ai.SetPosition(pos);
		ai.SetOrientation(aiSpawn.Orientation);
		ExpansionHumanLoadout.Apply(ai, aiSpawn.Loadout, false);
		ai.SetMovementSpeedLimits(aiSpawn.MinSpeed, aiSpawn.MaxSpeed);
		ai.Expansion_SetCanBeLooted(aiSpawn.CanBeLooted);
		ai.eAI_SetUnlimitedReload(aiSpawn.UnlimtedReload);
		ai.eAI_SetAccuracy(aiSpawn.MinAccuracy, aiSpawn.MaxAccuracy);
		ai.eAI_SetThreatDistanceLimit(aiSpawn.ThreatDistanceLimit);
		ai.eAI_SetDamageMultiplier(aiSpawn.DamageMultiplier);

		eAIGroup aiGroup;
		eAIFaction faction = eAIFaction.Create(aiSpawn.Faction);
		if (!Class.CastTo(aiGroup, ai.GetGroup()))
		{
			if (!Class.CastTo(aiGroup, eAIGroup.CreateGroup(faction)))
				return;

			ai.SetGroup(aiGroup);
		}

		aiGroup.SetFaction(faction);
		aiGroup.SetFormation(eAIFormation.Create("Column"));
		aiGroup.SetWaypointBehaviour(eAIWaypointBehavior.ALTERNATE);

		for (int idx = 0; idx < aiSpawn.Waypoints.Count(); idx++)
		{
			aiGroup.AddWaypoint(aiSpawn.Waypoints[idx]);
			if (aiSpawn.Waypoints[idx] == pos)
				aiGroup.m_CurrentWaypointIndex = idx;
		}

		if (aiSpawn.ShelterPositions)
		{
			m_SpawnedAI.Insert(ai, aiSpawn.ShelterPositions);
		}
	}
#endif

#ifdef EXPANSIONMODMARKET
	//! @note: Handles spawn of the dynamic merchant
	protected void CreateMerchant()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		ExpansionMarketSettings marketSettings = GetExpansionSettings().GetMarket();
		int findIndex = -1;

		array<int> usedMerchantPositions = m_ServerData.GetUsedMerchantPositions();
		array<int> usedMerchantSets = m_ServerData.GetUsedMerchantItemSets();

		ExpansionMerchantPosition positionToUse = GetExpansionSettings().GetNamalskAdventure().GetMerchantPosition(); //! Get a random position for the merchant to spawn.
		int posIDToUse = positionToUse.ID;

		//! Check if the random seleted position already got used in the previous sessions.
		findIndex = -1;
		findIndex = usedMerchantPositions.Find(posIDToUse);

		//! If the position got used we want to check for a valid position to use and if we already used all positions
		//! we reset the server data that holds that information and get a random position again.
		if (findIndex > -1)
		{
			ModuleDebugPrint("::CreateMerchant - Position with ID " + posIDToUse + " got already used in previous sessions. Check if we have still unused positions..");
			positionToUse = null;
			
			for (int i = 0; i < GetExpansionSettings().GetNamalskAdventure().MerchantPositions.Count(); i++)
			{
				ExpansionMerchantPosition pos = GetExpansionSettings().GetNamalskAdventure().MerchantPositions[i];
				if (!pos)
					continue;
				
				int posID = pos.ID;
				ModuleDebugPrint("::CreateMerchant - Check if position with ID " + posID + " got used yet..");
				findIndex = -1;
				findIndex = usedMerchantPositions.Find(posID);
				if (findIndex == -1)
				{
					ModuleDebugPrint("::CreateMerchant - Position with ID " + posID + " got not used yet! Using position..");
					positionToUse = pos;
					break;
				}
				else
				{
					ModuleDebugPrint("::CreateMerchant - Position with ID " + posID + " got already used! Check next..");
				}
			}

			//! If we found no position to use we need to reset the server data vector array of the last used merchant positions.
			if (!positionToUse)
			{
				ModuleDebugPrint("::CreateMerchant - No unused positions left! Reset used positons..");
				m_ServerData.ResetUsedPositions();
				positionToUse = GetExpansionSettings().GetNamalskAdventure().GetMerchantPosition(); //! Get a random position for the merchant to spawn.
			}
		}

		if (!positionToUse)
		{
			Error(ToString() + "::CreateMerchant - Could not get a valid position to spawn merchant!");
			return;
		}

		m_ServerData.GetUsedMerchantPositions().Insert(positionToUse.ID);

		ExpansionMerchantItemSet itemSetToUse = GetExpansionSettings().GetNamalskAdventure().GetMerchantItemSet(); //! Get a random set of items that the merchant will display.
		int itemSetIDToUse = itemSetToUse.ID;

		//! Check if the random seleted set already got used in the previous sessions.
		findIndex = -1;
		findIndex = usedMerchantSets.Find(itemSetIDToUse);

		//! If the set got used we want to check for a valid set to use and if we already used all sets
		//! we reset the server data that holds that information and get a random set again.
		if (findIndex > -1)
		{
			ModuleDebugPrint("::CreateMerchant - Item set with ID " + itemSetIDToUse + " got already used in previous sessions. Check if we have still unused item sets..");
			itemSetToUse = null;
			
			for (int j = 0; j < GetExpansionSettings().GetNamalskAdventure().MerchantItemSets.Count(); j++)
			{
				ExpansionMerchantItemSet itemSet = GetExpansionSettings().GetNamalskAdventure().MerchantItemSets[j];
				if (!itemSet)
					continue;
				
				int setID = itemSet.ID;
				ModuleDebugPrint("::CreateMerchant - Check if item set with ID " + setID + " got used yet..");
				findIndex = -1;
				findIndex = usedMerchantSets.Find(setID);
				if (findIndex == -1)
				{
					ModuleDebugPrint("::CreateMerchant - Item set with ID " + setID + " got not used yet! Using Item set..");
					itemSetToUse = itemSet;
					break;
				}
				else
				{
					Print(ToString() + "::CreateMerchant - Item set with ID " + setID + " got already used! Check next..");
				}
			}

			//! If we found no set to use we need to reset the server data int array of the last used merchant item sets.
			if (!itemSetToUse)
			{
				ModuleDebugPrint("::CreateMerchant - No unused item sets left! Reset used item sets..");
				m_ServerData.ResetUsedItemSets();
				itemSetToUse = GetExpansionSettings().GetNamalskAdventure().GetMerchantItemSet(); //! Get a random set of items that the merchant will display.
			}
		}

		if (!itemSetToUse)
		{
			Error(ToString() + "::CreateMerchant - Could not get a valid item set for merchant!");
			return;
		}

		m_ServerData.GetUsedMerchantItemSets().Insert(itemSetToUse.ID);
		m_ServerData.Save();

		ExpansionMarketTraderZone traderZone = new ExpansionMarketTraderZone();
		traderZone.m_DisplayName = "Merchant";
		traderZone.Position = positionToUse.Position;
		traderZone.Radius = 10.0;
		traderZone.BuyPricePercent = 100.0;
		traderZone.SellPricePercent = -1.0;

		ExpansionMarketTrader trader = new ExpansionMarketTrader();
		trader.DisplayName = "Merchant";
	#ifdef EXPANSIONMODHARDLINE
		trader.MinRequiredReputation = 0;
		trader.MaxRequiredReputation = 2147483647;
	#endif
		trader.TraderIcon = "Deliver";
		trader.Currencies.Insert("expansionbanknotehryvnia");

		string className;
		int amount;
		ExpansionMarketTraderBuySell buySell;

		array<ref ExpansionMerchantItem> items = itemSetToUse.Items;
		
		for (int k = 0; k < items.Count(); k++)
		{
			ExpansionMerchantItem item = items[k];
			if (!item)
				continue;

			className = item.ClassName;
			amount = item.Amount;
			buySell = item.BuySell;

			traderZone.AddStock(className, amount);
			trader.AddItem(className, buySell);
		}

		marketSettings.AddMarketZone(traderZone);
		marketSettings.AddMarketTrader(trader);

		Object obj = GetGame().CreateObject("ExpansionTraderAIMirek", positionToUse.Position);
		ExpansionTraderAIBase aiTrader = ExpansionTraderAIBase.Cast(obj);
		if (!aiTrader)
		{
			Error(ToString() + "::CreateMerchant - Could not spawn merchant object!");
			GetGame().ObjectDelete(obj);
			return;
		}

		ExpansionTraderObjectBase traderBase = new ExpansionTraderObjectBase(aiTrader);
		if (!traderBase)
		{
			Error(ToString() + "::CreateMerchant - Could not spawn merchant trader base object!");
			GetGame().ObjectDelete(obj);
			return;
		}

		traderBase.SetTraderZone(traderZone);
		traderBase.SetTraderMarket(trader);
		aiTrader.SetTraderObject(traderBase);

		aiTrader.m_Expansion_NetsyncData.Set(0, "Merchant");
		aiTrader.m_Expansion_NetsyncData.Set(1, "{5F2743E5F6F4DF0D}DayZExpansion/Core/GUI/icons/misc/coinstack2_64x64.edds");

		ExpansionHumanLoadout.Apply(aiTrader, "SurvivorLoadout", false);
		aiTrader.SetPosition(positionToUse.Position);
		aiTrader.SetOrientation(positionToUse.Orientation);

		traderBase.UpdateTraderZone();

	#ifdef EXPANSIONMODNAVIGATION
		m_MerchantServerMarker = ExpansionMarkerModule.GetModuleInstance().CreateServerMarker("Merchant", "Coins 2", positionToUse.Position, ARGB(255, 15, 185, 177), false);
	#endif
	}
#endif

	void OnNamalskEventStart(typename eventType)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		if (eventType == EVRStorm || eventType == EVRStormDeadly)
		{
			m_EVRStormActive = true;
		}
	}

	void OnNamalskEventCancel(typename eventType)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

	#ifdef DIAG
		ExpansionNotification(new StringLocaliser("NAMALSK EVENT CANCELED"), new StringLocaliser("%1 EVENT CANCELED", eventType.ToString()), ExpansionIcons.GetPath("Exclamationmark"), COLOR_EXPANSION_NOTIFICATION_INFO, 7, ExpansionNotificationType.TOAST).Create();
	#endif
		
		m_LastNamalskEvent = eventType;

		if (eventType == EVRStorm || eventType == EVRStormDeadly)
		{
			m_EVRStormActive = false;
			
			SetSatelitteActive(false);
		}
	}

	//! @note: Handles events that should start when ever a EVR storm event starts.
	void OnEVRStormInitPhaseServer()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
	}

	//! @note: Handles events that should start when ever a EVR storm mid phase starts.
	void OnEVRStormMidPhaseServer()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		SetSatelitteActive(true);
		
	#ifdef EXPANSIONMODAI
		SetAIBuildingPositions();
	#endif
	}
	
	//! @note: Handles events that should start when ever the EVR storm final blowout starts.
	//! Modified version to process and loop thrue spawned anomaly entities in chuncks.
	void OnEVRStormFinalBlowout()
	{
	    auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		m_EVRStormBlowout = true;
	}

	//! @note: Handles events that should start when ever a EVR storm event end phase starts.
	void OnEVRStormEndPhaseServer()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

	#ifdef EXPANSIONMODAI
		ResetAIPositions();
	#endif

		NamEventManager event_manager;
	    g_Script.CallFunction(GetGame().GetMission(), "GetNamEventManager", event_manager, null);
		
		m_LastNamalskEvent = event_manager.GetLastEventType();
		m_EVRStormActive = false;
		m_EVRStormBlowout = false;

		SetSatelitteActive(false);
	}

	//! @note: Handles events that should start when ever a EVR storm event ended.
	//! Gets called on every active player on the server.
	void OnEVRStormFinished(SurvivorBase player)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		//! PLACEHOLDER
	}

#ifdef EXPANSIONMODAI
	void SetAIBuildingPositions()
	{
	    auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

	    const int CHUNK_SIZE = 10; //! @note: Modifiy if needed
		int endIndex;

	    int numAI = m_SpawnedAI.Count();
	    for (int i = 0; i < numAI; i += CHUNK_SIZE)
	    {
	        endIndex = Math.Min(i + CHUNK_SIZE, numAI);
	        for (int j = i; j < endIndex; j++)
	        {
	            eAIBase ai = m_SpawnedAI.GetKey(j);
			 	GetAIBuildingWaypoint(ai);
	        }
	    }
	}

	//! @note: Handles reset of spawned AI units and active quest AI NPCs waypoints after a EVR strom event.
	protected void ResetAIPositions()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		for (int i = 0; i < m_SpawnedAI.Count(); i++)
		{
			eAIBase npcAI = m_SpawnedAI.GetKey(i);
			ModuleDebugPrint("::ResetAIPositions - AI: " + npcAI.GetType());
			ResetSpawnedAIWaypoints(npcAI);
		}
	}

	protected void GetAIBuildingWaypoint(eAIBase ai)
	{
	    auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
	
	    if (!ai.IsAlive())
	        return;
	
	    eAIGroup aiGroup = ai.GetGroup();
	    if (!aiGroup)
	        return;
		
		array<vector> shelterPositions = m_SpawnedAI.Get(ai);
		ModuleDebugPrint("::GetAIBuildingWaypoint - Shelter positions: " + shelterPositions.ToString() + " | Count: " + shelterPositions.Count());
		if (shelterPositions)
		{
			array<vector> waypoints = new array<vector>;
			
			waypoints.Copy(aiGroup.GetWaypoints());

			m_SpawnedAI.Set(ai, waypoints);
	   		
			aiGroup.ClearWaypoints();
	   		
			aiGroup.AddWaypoint(shelterPositions.GetRandomElement());
			
			aiGroup.m_CurrentWaypointIndex = 0;
			ai.SetMovementSpeedLimit(eAIMovementSpeed.JOG);
			m_AISpeed = ai.GetMovementSpeedLimit();
		}
	}

	protected void ResetSpawnedAIWaypoints(eAIBase ai)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		ModuleDebugPrint("::ResetSpawnedAIWaypoints - AI: " + ai.ToString());

		eAIGroup aiGroup = ai.GetGroup();
		if (!aiGroup)
		{
			Error("::ResetSpawnedAIWaypoints - AI has not Group!");
			return;
		}

		ModuleDebugPrint("::ResetSpawnedAIWaypoints - AI group: " + aiGroup.ToString());
		aiGroup.ClearWaypoints();

		array<vector> waypoints = m_SpawnedAI.Get(ai);
		ModuleDebugPrint("::ResetSpawnedAIWaypoints - Waypoints: " + waypoints.ToString());
		
		for (int i = 0; i < waypoints.Count(); i++)
		{
			vector pos = waypoints[i];
			
			ModuleDebugPrint("::ResetSpawnedAIWaypoints - Adding back old waypoint: " + waypoints.Count());
			aiGroup.AddWaypoint(pos);
		}
		
		aiGroup.m_CurrentWaypointIndex = 0;
		
		ai.SetMovementSpeedLimit(m_AISpeed);
	}

#ifdef EXPANSIONMODQUESTS
	void AfterQuestModuleClientInit(ExpansionQuestPersistentData playerQuestData, PlayerIdentity identity)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

		/*
		//! Server only
		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::AfterQuestModuleClientInit - Tryed to call AfterQuestModuleClientInit on Client!");
			return;
		}

		if (!playerQuestData)
		{
			Error(ToString() + "::AfterQuestModuleClientInit - Could not get player quest data for player with UID: " + identity.GetId());
			return;
		}

		ExpansionQuestState questState = playerQuestData.GetQuestStateByQuestID(1);
		ModuleDebugPrint("::AfterQuestModuleClientInit - Main quest quest-state: " + questState);
		ModuleDebugPrint("::AfterQuestModuleClientInit - Main quest quest-state name: " + typename.EnumToString(ExpansionQuestState, questState));

		if (questState == ExpansionQuestState.NONE)
		{
			//! Show the first quest to the player
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ShowMainQuest, 3000, false, identity);
		}
		*/
	}

	protected void ShowMainQuest(PlayerIdentity identity)
	{
		ExpansionQuestModule.GetModuleInstance().RequestOpenQuestMenu(identity, 1);
	}
#endif
#endif

#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL
	//! @note: Condition check if a EVR storm is currently active.
	bool IsEVRStormActive()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);

	#ifdef NAMALSK_SURVIVAL
	    NamEventManager event_manager;
	    g_Script.CallFunction(GetGame().GetMission(), "GetNamEventManager", event_manager, null);

	    if (!event_manager)
	        return false;

		EVRStorm storm = EVRStorm.Cast(event_manager.GetEvent(EVRStorm));
		EVRStormDeadly stormDeadly = EVRStormDeadly.Cast(event_manager.GetEvent(EVRStormDeadly));
		if (storm && storm.GetEVRStormPhase() > ExpansionEVRStormPhase.NONE || stormDeadly && stormDeadly.GetEVRStormPhase() > ExpansionEVRStormPhase.NONE)
	   		return true;
	#endif

		return false;
	}

	void SpawnSatelliteAntennaObjects()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		Object obj = GetGame().CreateObject("SV_Abandoned_Sattelite_Antenna", Vector(1202.799561, 14.207986, 11784.280273));
		m_AbdonedSatellite = SV_Abandoned_Sattelite_Antenna.Cast(obj);
		if (!m_AbdonedSatellite)
		{
			Error(ToString() + "::SpawnSatelliteAntennaObjects - Could not spawn antenna object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_AbdonedSatellite.SetPosition(Vector(1202.799561, 14.207986, 11784.280273));
		m_AbdonedSatellite.SetOrientation(Vector(81.209969, -0.000000, -0.000000));
		m_AbdonedSatellite.Update();
		
	#ifdef EXPANSIONMODTELEPORTER
		obj = GetGame().CreateObject("Expansion_Teleporter_Big", Vector(1200.880127, 4.619668, 11780.145508));
		m_SatelliteTeleporter = Expansion_Teleporter_Big.Cast(obj);
		if (!m_SatelliteTeleporter)
		{
			Error(ToString() + "::SpawnSatelliteAntennaObjects - Could not spawn antenna teleporter object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_SatelliteTeleporter.SetPosition(Vector(1200.880127, 4.619668, 11780.145508));
		m_SatelliteTeleporter.SetOrientation(Vector(-100.711388, -0.000000, -0.000000));
		m_SatelliteTeleporter.SetTeleporterID(100);
		m_SatelliteTeleporter.SetActive(false);
		m_SatelliteTeleporter.Update();
		
		ExpansionTeleportData teleporterData = new ExpansionTeleportData();
		teleporterData.SetID(100);
		teleporterData.SetDisplayName("???");
		teleporterData.SetObjectPosition(Vector(1200.880127, 4.619668, 11780.145508));

		ExpansionTeleportPosition teleportPos = new ExpansionTeleportPosition();
		teleportPos.SetData("???");
		teleportPos.AddPosition(Vector(5079.959961, 2085.610107, 11720.700195), Vector(39.183594, 0.000000, 0.000000));
		teleportPos.AddPosition(Vector(5075.359863, 2085.610107, 11715.299805), Vector(132.597000, 0.000000, 0.000000));

		teleporterData.AddTeleportPosition(teleportPos);
		
		ExpansionTeleporterModule.GetModuleInstance().AddTeleporterData(teleporterData);
	#endif

		obj = GetGame().CreateObject("Expansion_Satellite_Control", Vector(1204.062256, 5.146724, 11782.631836));
		m_SatelliteController = Expansion_Satellite_Control.Cast(obj);
		if (!m_SatelliteController)
		{
			Error(ToString() + "::SpawnSatelliteAntennaObjects - Could not spawn antenna teleporter object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_SatelliteController.SetPosition(Vector(1204.062256, 5.146724, 11782.631836));
		m_SatelliteController.SetOrientation(Vector(171.544205, 0.000000, 0.000000));
		if (m_AbdonedSatellite)
			m_SatelliteController.SetLinkedSatellite(m_AbdonedSatellite);
	#ifdef EXPANSIONMODTELEPORTER
		if (m_SatelliteTeleporter)
			m_SatelliteController.SetLinkedTeleporter(m_SatelliteTeleporter);
	#endif
		m_SatelliteController.Update();
	}
	
	void PlaySatelliteCrySFX()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		string soundSet = "Expansion_Satellite_Cry_Distance_SoundSet";
		Param3<bool, vector, int> satelliteCry = new Param3<bool, vector, int>(true, Vector(1202, 14, 11784), soundSet.Hash());
		GetGame().RPCSingleParam(null, ERPCs.RPC_SOUND_HELICRASH, satelliteCry, true);
	}
	
	void SetSatelitteActive(bool state)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		if (state == m_SatelitteState)
			return;
				
		if (state)
		{
			ActivateSatellite();
		}
		else
		{
			DeactivateSatellite();
		}
		
		m_SatelitteState = state;
	}
	
	void ActivateSatellite()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		if (!m_AbdonedSatellite)
			return;
		
		m_SatelliteController.SetActivateState(true);
		m_AbdonedSatellite.SetSatelliteActive(true);
	}
	
	void DeactivateSatellite()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		if (!m_AbdonedSatellite)
			return;
		
		if (!m_SatelliteController.IsActive())
		{
			m_SatelliteController.SetActivateState(false);
			m_AbdonedSatellite.SetSatelliteActive(false);
		}
	}
	
	SV_Abandoned_Sattelite_Antenna GetSatelliteAntenna()
	{
		return m_AbdonedSatellite;
	}

	Expansion_Teleporter_Big GetSatelliteTeleporter()
	{
		return m_SatelliteTeleporter;
	}
	
	Expansion_Satellite_Control GetSatelliteController()
	{
		return m_SatelliteController;
	}
	
	void SpawnA1Bunker()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		//! Entrance trigger
		ExpansionBunkerEntranceTriggerBase trigger = ExpansionBunkerEntranceTriggerBase.Cast(GetGame().CreateObjectEx("ExpansionBunkerEntranceTriggerBase", m_A1_Bunker_EntrancePos, ECE_NONE));
		if (trigger)
		{
			trigger.SetPosition(m_A1_Bunker_EntrancePos);
			trigger.SetTriggerRadius(1);
			trigger.Update();
		}
		
		//! Teleporter
		Object obj = GetGame().CreateObject("Expansion_Teleporter_Big", m_A1_Bunker_TeleporterPos);
		m_A1BunkerTeleporter = Expansion_Teleporter_Big.Cast(obj);
		if (!m_A1BunkerTeleporter)
		{
			Error(ToString() + "::SpawnA1Bunker - Could not spawn teleporer object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_A1BunkerTeleporter.SetPosition(m_A1_Bunker_TeleporterPos);
		m_A1BunkerTeleporter.SetOrientation(m_A1_Bunker_TeleporterOri);
		m_A1BunkerTeleporter.SetTeleporterID(101);
		m_A1BunkerTeleporter.SetActive(false);
		m_A1BunkerTeleporter.Update();
		
		ExpansionTeleportData teleporterData = new ExpansionTeleportData();
		teleporterData.SetID(101);
		teleporterData.SetDisplayName("???");
		teleporterData.SetObjectPosition(m_A1_Bunker_TeleporterPos);
		teleporterData.SetIsExit(true);

		ExpansionTeleportPosition teleportPos = new ExpansionTeleportPosition();
		teleportPos.SetData("???");
		teleportPos.AddPosition(Vector(4272.379883, 98.819305, 8398.056641), Vector(0.000000, 0.000000, 0.000000));
		teleporterData.AddTeleportPosition(teleportPos);
		ExpansionTeleporterModule.GetModuleInstance().AddTeleporterData(teleporterData);
		
		//! Generator
		obj = GetGame().CreateObject("Expansion_Bunker_Generator", m_A1_Bunker_GeneratorPos);
		m_A1BungerGenerator = Expansion_Bunker_Generator.Cast(obj);
		if (!m_A1BungerGenerator)
		{
			Error(ToString() + "::SpawnA1Bunker - Could not spawn generator object!");
			GetGame().ObjectDelete(obj);
			return;
		}

		m_A1BungerGenerator.SetPosition(m_A1_Bunker_GeneratorPos);
		m_A1BungerGenerator.SetOrientation(m_A1_Bunker_GeneratorOri);
		if (m_A1BunkerTeleporter)
			m_A1BungerGenerator.SetLinkedTeleporter(m_A1BunkerTeleporter);
		m_A1BungerGenerator.Update();
		
		//! Locker quest object - @note: Make this a config param class in the namalsk settings class you lazy ass!
		ExpansionNamalskQuestHolder questHolder = new ExpansionNamalskQuestHolder(1000, "ExpansionQuestObjectLocker", true, "Closed Locker");
		ExpansionNamalskQuestHolderPosition pos = new ExpansionNamalskQuestHolderPosition("2984.784180 14.845232 8714.851563", "-47.022182 0.000000 0.000000");
		questHolder.AddPosition(pos);
		pos = new ExpansionNamalskQuestHolderPosition("2978.834473 14.824975 8701.199219", "45.000000 -0.000000 -0.000000");
		questHolder.AddPosition(pos);
		pos = new ExpansionNamalskQuestHolderPosition("2990.530518 15.530652 8729.129883", "45.000000 -0.000000 -0.000000");
		questHolder.AddPosition(pos);
		pos = new ExpansionNamalskQuestHolderPosition("3018.906982 15.742278 8713.576172", "126.000061 0.000000 -0.000000");
		questHolder.AddPosition(pos);
		
		if (!m_QuestHolders)
			m_QuestHolders = new array<ref ExpansionNamalskQuestHolder>;

		m_QuestHolders.Insert(questHolder);
		SpawnQuestHolder(questHolder);
		
		//! Bunker fake entrance panel
		obj = GetGame().CreateObject("Land_Underground_Panel_Lever", m_A1_Bunker_FakeEntranceLeaverPos);
		m_A1BunkerFakeEntranceLeaver = Land_Underground_Panel_Lever.Cast(obj);
		if (!m_A1BunkerFakeEntranceLeaver)
		{
			Error(ToString() + "::SpawnA1Bunker - Could not spawn fake bunker panel object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_A1BunkerFakeEntranceLeaver.SetPosition(m_A1_Bunker_FakeEntranceLeaverPos);
		m_A1BunkerFakeEntranceLeaver.SetOrientation(m_A1_Bunker_FakeEntranceLeaverOri);
		m_A1BunkerTeleporter.Update();
		
		//! Bunker entrance panel
		obj = GetGame().CreateObject("Land_Underground_Panel", m_A1_Bunker_EntrancePanelPos);
		m_A1BunkerEntrancePanel = Land_Underground_Panel.Cast(obj);
		if (!m_A1BunkerEntrancePanel)
		{
			Error(ToString() + "::SpawnA1Bunker - Could not spawn bunker panel object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_A1BunkerEntrancePanel.SetPosition(m_A1_Bunker_EntrancePanelPos);
		m_A1BunkerEntrancePanel.SetOrientation(m_A1_Bunker_EntrancePanelOri);
		m_A1BunkerEntrancePanel.SetLinkedFakePanel(m_A1BunkerFakeEntranceLeaver);
		m_A1BunkerTeleporter.Update();

		//! Bunker entrance panel leaver
		obj = GetGame().CreateObject("Land_Underground_Panel_Lever", m_A1_Bunker_EntranceLeaverPos);
		m_A1BunkerEntranceLeaver = Land_Underground_Panel_Lever.Cast(obj);
		if (!m_A1BunkerEntranceLeaver)
		{
			Error(ToString() + "::SpawnA1Bunker - Could not spawn bunker leaver object!");
			GetGame().ObjectDelete(obj);
			return;
		}
		
		m_A1BunkerEntranceLeaver.SetPosition(m_A1_Bunker_EntranceLeaverPos);
		m_A1BunkerEntranceLeaver.SetOrientation(m_A1_Bunker_EntranceLeaverOri);
		m_A1BunkerEntranceLeaver.SetLinkedFakePanel(m_A1BunkerFakeEntranceLeaver);
		m_A1BunkerTeleporter.Update();
	}
#endif

	protected void SpawnQuestHolder(ExpansionNamalskQuestHolder questHolder)
	{
		TStringArray questNPCs = {"ExpansionQuestNPCBase"};
		TStringArray questAINPCs = {"ExpansionQuestNPCAIBase"};
		TStringArray questObject = {"ExpansionQuestStaticObject"};
		
		ExpansionQuestNPCData questNPCData = new ExpansionQuestNPCData;
		questNPCData.SetID(questHolder.ID); //! Unique NPC ID
		questNPCData.SetClassName(questHolder.ClassName); //! Class name of the NPC entity
		questNPCData.SetIsStatic(questHolder.IsStatic);
		questNPCData.SetNPCName(questHolder.Name);
		questNPCData.SetDefaultNPCText(questHolder.DefaultText);
		
		ExpansionNamalskQuestHolderPosition randomPos = questHolder.Positions.GetRandomElement();
		if (!randomPos)
			return;

		questNPCData.SetPosition(randomPos.Position); //! Quest NPC position
		questNPCData.SetOrientation(randomPos.Orientation); //! Quest NPC orientation
		
		ExpansionQuestModule.GetModuleInstance().AddQuestNPCData(questHolder.ID, questNPCData);
		
		if (ExpansionStatic.IsAnyOf(questHolder.ClassName, questObject))
		{
			ExpansionQuestStaticObject object = SpawnQuestObject(questNPCData);
			if (!object)
				return;
	
			object.SetQuestNPCID(questHolder.ID);
			object.SetQuestNPCData(questNPCData);
			ExpansionQuestModule.GetModuleInstance().AddStaticQuestObject(questHolder.ID, object);
		}
		else if (ExpansionStatic.IsAnyOf(questHolder.ClassName, questNPCs))
		{
			ExpansionQuestNPCBase questNPC = SpawnQuestNPC(questNPCData);
			if (!questNPC)
				return;
	
			questNPC.SetQuestNPCID(questHolder.ID);
			questNPC.SetQuestNPCData(questNPCData);
			ExpansionQuestModule.GetModuleInstance().AddQuestNPC(questHolder.ID, questNPC);
		}
	#ifdef EXPANSIONMODAI
		else if (ExpansionStatic.IsAnyOf(questHolder.ClassName, questAINPCs))
		{
			ExpansionQuestNPCAIBase questNPCAI = SpawnQuestNPCAI(questNPCData);
			if (!questNPCAI)
				return;
	
			questNPCAI.SetQuestNPCID(questHolder.ID);
			questNPCAI.SetQuestNPCData(questNPCData);
			ExpansionQuestModule.GetModuleInstance().AddQuestNPCAI(questHolder.ID, questNPCAI);
		}
	#endif
	}
	
	ExpansionQuestStaticObject SpawnQuestObject(ExpansionQuestNPCData questNPCData)
	{
	    Object obj = GetGame().CreateObjectEx(questNPCData.GetClassName(), questNPCData.GetPosition(), ECE_ROTATIONFLAGS | ECE_PLACE_ON_SURFACE);
	    ExpansionQuestStaticObject questObject;
	    if (!ExpansionQuestStaticObject.CastTo(questObject, obj))
	    {
			GetGame().ObjectDelete(obj);
	        return null;
	    }

	    questObject.SetPosition(questNPCData.GetPosition());
	    questObject.SetOrientation(questNPCData.GetOrientation());
		questObject.Update();
		
		if (questNPCData.GetNPCName() != string.Empty)
	    	questObject.m_Expansion_NetsyncData.Set(0, questNPCData.GetNPCName());

	    return questObject;
	}
	
	ExpansionQuestNPCBase SpawnQuestNPC(ExpansionQuestNPCData questNPCData)
	{
		Object obj = GetGame().CreateObjectEx(questNPCData.GetClassName(), questNPCData.GetPosition(), ECE_INITAI | ECE_CREATEPHYSICS | ECE_ROTATIONFLAGS | ECE_PLACE_ON_SURFACE);
 		ExpansionQuestNPCBase questNPC;
		if (!ExpansionQuestNPCBase.CastTo(questNPC, obj))
	    {
			GetGame().ObjectDelete(obj);
	        return null;
	    }

	    questNPC.SetPosition(questNPCData.GetPosition());
	    questNPC.SetOrientation(questNPCData.GetOrientation());

		questNPC.Update();
		questNPC.m_Expansion_NetsyncData.Set(0, questNPCData.GetNPCName());
		ExpansionHumanLoadout.Apply(questNPC, questNPCData.GetLoadoutFile(), false);

		return questNPC;
	}

#ifdef EXPANSIONMODAI
	ExpansionQuestNPCAIBase SpawnQuestNPCAI(ExpansionQuestNPCData questNPCData)
	{
		vector position = ExpansionAIPatrol.GetPlacementPosition(questNPCData.GetPosition());
		
		Object obj = GetGame().CreateObject(questNPCData.GetClassName(), position);
		if (!obj)
			return null;

		ExpansionQuestNPCAIBase questNPC = ExpansionQuestNPCAIBase.Cast(obj);
		if (!questNPC)
		{
			GetGame().ObjectDelete(obj);
			return null;
		}

		questNPC.SetPosition(position);
		questNPC.SetOrientation(questNPCData.GetOrientation());
		questNPC.Update();
		questNPC.m_Expansion_NetsyncData.Set(0, questNPCData.GetNPCName());
		ExpansionHumanLoadout.Apply(questNPC, questNPCData.GetLoadoutFile(), false);
		questNPC.Expansion_SetCanBeLooted(false);
		questNPC.eAI_SetUnlimitedReload(true);
		questNPC.eAI_SetAccuracy(1.0, 1.0);
		questNPC.eAI_SetThreatDistanceLimit(800);

		eAIGroup aiGroup = questNPC.GetGroup();

		if (questNPCData.GetFaction() != string.Empty)
		{
			eAIFaction faction = eAIFaction.Create(questNPCData.GetFaction());
			if (faction && aiGroup.GetFaction().Type() != faction.Type())
				aiGroup.SetFaction(faction);
		}
		
		aiGroup.SetFormation(new eAIFormationColumn(aiGroup));
		aiGroup.SetWaypointBehaviour(eAIWaypointBehavior.ALTERNATE);
		
		array<vector> waypoints = questNPCData.GetWaypoints();
		for (int idx = 0; idx < waypoints.Count(); idx++)
		{
			aiGroup.AddWaypoint(waypoints[idx]);
			if (waypoints[idx] == position)
				aiGroup.m_CurrentWaypointIndex = idx;
		}
		
		return questNPC;
	}
#endif
	
	protected void SpawnSupplyCrates()
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
		
		//! Supply crates
		array<ref ExpansionSupplyCrateSetup> supplyCrateSpawns = GetExpansionSettings().GetNamalskAdventure().GetSupplyCrateSpawns();
		foreach (ExpansionSupplyCrateSetup supplyCrate: supplyCrateSpawns)
		{
			Object obj = GetGame().CreateObject(supplyCrate.ClassName, supplyCrate.Position);
			Expansion_SupplyCrate_Base supplyCareObj = Expansion_SupplyCrate_Base.Cast(obj);
			if (!supplyCareObj)
			{
				Error(ToString() + "::OnMissionLoaded - Could not spawn supply crate object!");
				GetGame().ObjectDelete(obj);
				continue;
			}
			
			supplyCareObj.SetPosition(supplyCrate.Position);
			supplyCareObj.SetOrientation(supplyCrate.Orientation);
			supplyCareObj.SetCrateLoot(supplyCrate.CrateLoot);
			supplyCareObj.Update();
		}
	}

	static ExpansionNamalskModule GetModuleInstance()
	{
		return s_ModuleInstance;
	}
	
	override void OnUpdate(Class sender, CF_EventArgs args)
	{
		super.OnUpdate(sender, args);
		
		auto update = CF_EventUpdateArgs.Cast(args);

		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			OnUpdateServer(update.DeltaTime);
		}
		
		if (GetGame().IsClient())
		{
			OnUpdateClient(update.DeltaTime);
		}
	}
	
	protected void OnUpdateServer(float deltaTime)
	{
	#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL		
		m_SatelliteCryTimer += deltaTime;
		if (m_SatelliteCryTimer >= SATELLITE_CRY_TIME)
		{
			PlaySatelliteCrySFX();
			m_SatelliteCryTimer = 0;
		}
		
		m_BunkerGeneratorsCheckTimer += deltaTime;
		if (m_BunkerGeneratorsCheckTimer >= BUNKER_GENERATORS_CHECK_TIME)
		{
			if (!s_BunkerGeneratorCheckSC)
		    	s_BunkerGeneratorCheckSC = ScriptCaller.Create(OnBunkerGeneratorCheck);

			m_CurrentCheckTime = CF_Date.Now(true).GetTimestamp();
			
			Expansion_Bunker_Generator.s_Expansion_AllBunkerGenerators.Each(s_BunkerGeneratorCheckSC);
			m_BunkerGeneratorsCheckTimer = 0;
		}
	#endif
		
		m_SupplyCratesCheckTimer += deltaTime;
		if (m_SupplyCratesCheckTimer >= SUPPLY_CRATES_CHECK_TIME)
		{
			if (!s_SupplyCratesCheckSC)
		    	s_SupplyCratesCheckSC = ScriptCaller.Create(OnSupplyCrateCheck);

			m_CurrentCheckTime = CF_Date.Now(true).GetTimestamp();
			
			Expansion_SupplyCrate_Base.s_Expansion_AllSupplyCrates.Each(s_SupplyCratesCheckSC);
			m_SupplyCratesCheckTimer = 0;
		}
	}

	protected void OnSupplyCrateCheck(Expansion_SupplyCrate_Base supplyCrate)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
			
		if (supplyCrate.HasLootDelay() && !supplyCrate.IsCrateOpened())
		{
			int crateCloseTime = supplyCrate.GetLastCloseTime();
			int crateReactivationTime = crateCloseTime + LOOT_DELAY_TIME;
			if (m_CurrentCheckTime > crateReactivationTime)
			{
				supplyCrate.EndLootDelay();
			}
		}
		else if (supplyCrate.IsCrateOpened())
		{
			int crateOpenTime = supplyCrate.GetLastOpenTime();
			int crateDelayTime = crateOpenTime + LOOT_DELAY_TIME;
			if (m_CurrentCheckTime > crateDelayTime)
			{
				supplyCrate.CloseCrate();
			}
		}
	}
	
#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL
	protected void OnBunkerGeneratorCheck(Expansion_Bunker_Generator generator)
	{
		auto trace = EXTrace.Start(EXTrace.NAMALSKADVENTURE, this);
			
		if (generator.IsActive())
		{
			int generatorActivationTime = generator.GetLastActivationTime();
			int generatorDisableTime = generatorActivationTime + BUNKER_GENERATOR_DEACTIVATION_TIME;
			if (m_CurrentCheckTime > generatorDisableTime)
			{
				int slot_id_key = InventorySlots.GetSlotIdFromString("Att_ExpansionGeneratorKey");
				Expansion_Bunker_Generator_Key key = Expansion_Bunker_Generator_Key.Cast(generator.GetInventory().FindAttachment(slot_id_key));
				if (!key)
					return;
				
				GetGame().ObjectDelete(key);
			}
		}
	}
#endif
	
	protected void OnUpdateClient(float deltaTime)
	{
	#ifdef EXPANSION_NAMALSK_ADVENTURE_SURVIVAL
		m_ClientUpdateTimer += deltaTime;
		if (m_ClientUpdateTimer >= CLIENT_UPDATE_TIME)
		{
			float dist = vector.Distance(m_A1_Bunker_EntrancePos, GetGame().GetPlayer().GetPosition());
			if (dist < 200) 
			{
				GetGame().PreloadObject("land_underground_stairs_exit", 3000);
				GetGame().PreloadObject("land_underground_stairs_block", 3000);
				GetGame().PreloadObject("land_underground_stairs_start", 3000);
				GetGame().PreloadObject("land_underground_floor_crew", 3000);
				GetGame().PreloadObject("land_underground_floor_comms", 3000);
				GetGame().PreloadObject("land_underground_stairs_collapsed", 3000);
				GetGame().PreloadObject("staticobj_wall_indcnc_10", 3000);
				GetGame().PreloadObject("land_underground_stairs_block_terminator", 3000);
				GetGame().PreloadObject("sv_tubes_underground", 3000);
			}
			
			m_ClientUpdateTimer = 0;
		}
	#endif
	}

	void ModuleDebugPrint(string text)
	{
	#ifdef EXPANSION_NAMALSK_ADVENTURE_DEBUG
		EXTrace.Print(EXTrace.NAMALSKADVENTURE, this, text);
	#endif
	}
};

#ifdef EXPANSIONMODQUESTS
modded class ExpansionQuestModule
{
	override void AfterClientInit(ExpansionQuestPersistentData playerQuestData, PlayerIdentity identity)
	{
		Print(ToString() + "::AfterClientInit - Start");

		super.AfterClientInit(playerQuestData, identity);
		ExpansionNamalskModule.GetModuleInstance().AfterQuestModuleClientInit(playerQuestData, identity);

		Print(ToString() + "::AfterClientInit - Start");
	}
};
#endif