/**
 * ExpansionDataCollectionModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionDataCollectionModule
 * @brief		This class handles the data collection system
 **/
[CF_RegisterModule(ExpansionDataCollectionModule)]
class ExpansionDataCollectionModule: CF_ModuleWorld
{
	private autoptr map <string, ref ExpansionPlayerDataCollection> m_PlayerData;
	private ref ScriptInvoker m_ModuleSI;
	
	override bool IsServer()
	{
		return true;
	}

	override bool IsClient()
	{
		return true;
	}
	
	override void OnInit()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.DATACOLLECTION, this, "OnInit");
#endif
		super.OnInit();

		EnableClientDisconnect();
		EnableInvokeConnect();
		
		EnableRPC();
				
		m_PlayerData = new map <string, ref ExpansionPlayerDataCollection>;
		
		if (IsMissionClient() && !IsMissionHost())
			m_ModuleSI = new ScriptInvoker();
	}

	override int GetRPCMin()
	{
		return ExpansionDataCollectionRPC.INVALID;
	}

	override int GetRPCMax()
	{
		return ExpansionDataCollectionRPC.COUNT;
	}
	
	override void OnRPC(Class sender, CF_EventArgs args)
	{
		super.OnRPC(sender, args);

		auto rpc = CF_EventRPCArgs.Cast(args);

		switch ( rpc.ID )
		{
		case ExpansionDataCollectionRPC.RequestPlayerData:
			RPC_RequestPlayerData(rpc.Context, rpc.Sender);
			break;
		case ExpansionDataCollectionRPC.SendPlayerData:
			RPC_SendPlayerData(rpc.Context);
			break;
		}
	}
		
	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
		super.OnInvokeConnect(sender, args);
		
		auto cArgs = CF_EventPlayerArgs.Cast(args);

		if (!cArgs.Player)
			return;
		
		OnPlayerConnect(cArgs.Player);
	}
	
	private void OnPlayerConnect(PlayerBase player)
	{
		Print("ExpansionDataCollectionModule::OnPlayerConnect - Start");
		
		if (!IsMissionHost())
			return;
		
		string playerUID = player.GetIdentity().GetId();
		ExpansionPlayerDataCollection playerData;
		if (m_PlayerData.Get(playerUID))
		{
			playerData = m_PlayerData.Get(playerUID);
			playerData.SetFromPlayerBase(player);
			m_PlayerData.Set(playerUID, playerData);
		}
		else
		{
			playerData = new ExpansionPlayerDataCollection();
			playerData.SetFromPlayerBase(player);
			m_PlayerData.Insert(playerUID, playerData);
		}
		
		Print("ExpansionDataCollectionModule::OnPlayerConnect - End");
	}
	
	/*override void OnInvokeDisconnect(Class sender, CF_EventArgs args)
	{
		super.OnInvokeDisconnect(sender, args);
		
		auto cArgs = CF_EventPlayerDisconnectedArgs.Cast(args);

		if (!cArgs.UID)
			return;
		
		OnPlayerDisconnect(cArgs.UID);
	}*/
	
	override void OnClientDisconnect(Class sender, CF_EventArgs args)
	{
		super.OnClientDisconnect(sender, args);

		auto cArgs = CF_EventPlayerDisconnectedArgs.Cast(args);

		if (!cArgs.Identity)
			return;
		
		string playerID = cArgs.Identity.GetId();
		OnPlayerDisconnect(cArgs.UID);
	}
	
	private void OnPlayerDisconnect(string playerUID)
	{
		Print("ExpansionDataCollectionModule::OnPlayerDisconnect - Start");
		Print("ExpansionDataCollectionModule::OnPlayerDisconnect - Player UID: " + playerUID);
		
		if (!IsMissionHost())
			return;
		
		ExpansionPlayerDataCollection playerData;
		if (m_PlayerData.Get(playerUID))
		{
			playerData = m_PlayerData.Get(playerUID);
			m_PlayerData.Remove(playerUID);
		}
		
		Print("ExpansionDataCollectionModule::OnPlayerDisconnect - End");
	}
	
	//! Client
	void RequestPlayerData()
	{
		Print("ExpansionDataCollectionModule::RequestPlayerData - Start");
		
		if (!IsMissionClient())
			return;
		
		ScriptRPC rpc = new ScriptRPC();
 		rpc.Send(NULL, ExpansionDataCollectionRPC.RequestPlayerData, false);
		
		Print("ExpansionDataCollectionModule::RequestPlayerData - End");
	}
	
	//! Server
	void RPC_RequestPlayerData(ParamsReadContext ctx, PlayerIdentity sender)
	{
		Print("ExpansionDataCollectionModule::RPC_RequestPlayerData - Start");
		
		if (!IsMissionHost())
			return;
		
		ScriptRPC rpc = new ScriptRPC();		
		int playersClount = GetAllPlayers().Count();
		rpc.Write(playersClount);
	
		for (int i = 0; i < playersClount; i++)
		{
			ExpansionPlayerDataCollection player = GetAllPlayers().GetElement(i);
			if (!player)
				continue;
			
			player.OnSend(rpc);
		}
		
 		rpc.Send(NULL, ExpansionDataCollectionRPC.SendPlayerData, false, sender);
		
		Print("ExpansionDataCollectionModule::RPC_RequestPlayerData - End");
	}
	
	//! Client
	void RPC_SendPlayerData(ParamsReadContext ctx)
	{
		Print("ExpansionDataCollectionModule::RPC_SendPlayerData - Start");
		
		if (!IsMissionClient())
			return;
		
		int playersCount;
		if (!ctx.Read(playersCount))
			return;
		
		for (int i = 0; i < playersCount; i++)
		{
			if (!OnReceivePlayerData(ctx))
				continue;
		}
		
		GetModuleSI().Invoke();
		
		Print("ExpansionDataCollectionModule::RPC_SendPlayerData - End");
	}
	
	bool OnReceivePlayerData(ParamsReadContext ctx)
	{
		Print("ExpansionDataCollectionModule::OnReceivePlayerData - Start");
		
		string playerUID;
		if (Expansion_Assert_False(ctx.Read(playerUID), "Failed to read player UID"))
			return false;

		ExpansionPlayerDataCollection player = m_PlayerData.Get(playerUID);
		if (!player)
		{
			player = new ExpansionPlayerDataCollection();
			player.PlayerUID = playerUID;
		}

		if (Expansion_Assert_False(player.OnRecieve(ctx), "Failed to read player data"))
			return false;
		
		m_PlayerData.Insert(playerUID, player);
		
		Print("ExpansionDataCollectionModule::OnReceivePlayerData - End and return true");
		return true;
	}
	
	map <string, ref ExpansionPlayerDataCollection> GetAllPlayers()
	{
		return m_PlayerData;
	}
	
	ScriptInvoker GetModuleSI()
	{
		return m_ModuleSI;
	}
	
	bool IsPlayerOnline(string uid)
	{
		foreach (string playerUID,  ExpansionPlayerDataCollection playerData: m_PlayerData)
		{
			if (playerData.PlayerUID == uid)
			{
				return true;
			}
		}
		
		return false;
	}
};

class ExpansionDataCollection
{
	bool OnRecieve(ParamsReadContext ctx);
	void OnSend(ParamsWriteContext ctx);
};

class ExpansionPlayerDataCollection: ExpansionDataCollection
{
	string PlayerUID;
	string Name;
	
	void SetFromPlayerBase(PlayerBase player)
	{
		PlayerUID = player.GetIdentity().GetId();
		Name = player.GetIdentity().GetName();
		
		Print("ExpansionPlayerDataCollection::SetFromPlayerBase - Player UID: " + PlayerUID + " | Name: " + Name);
	}
	
	override void OnSend(ParamsWriteContext ctx)
	{
		ctx.Write(PlayerUID);
		ctx.Write(Name);
	}
	
	override bool OnRecieve(ParamsReadContext ctx)
	{
		if (!ctx.Read(Name))
			return false;
		
		return true;
	}
};