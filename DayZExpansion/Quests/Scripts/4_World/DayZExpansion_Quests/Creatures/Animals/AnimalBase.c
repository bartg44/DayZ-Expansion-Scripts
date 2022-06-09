/**
 * AnimalBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class AnimalBase
{
	// ------------------------------------------------------------
	// AnimalBase EEKilled
	// ------------------------------------------------------------
	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		EntityAI killSource = EntityAI.Cast(killer);

		ExpansionQuestModule questModule = ExpansionQuestModule.Cast(CF_ModuleCoreManager.Get(ExpansionQuestModule));
		PlayerBase player;
		EntityAI target;

		if (killSource.GetHierarchyParent())
		{
			Object parent = killSource.GetHierarchyParent();
			if (parent.IsMan() || parent.IsKindOf("PlayerBase") || parent.IsInherited(PlayerBase))
			{
				player = PlayerBase.Cast(killSource.GetHierarchyParent());
				target = this;
				if (player && target)
				{
					questModule.OnEntityKilled(this, killSource, player.GetIdentity().GetId());
				}
			}
		}
		else if (killSource.IsMan() || killSource.IsKindOf("PlayerBase") || killSource.IsInherited(PlayerBase))
		{
			player = PlayerBase.Cast(killSource);
			target = this;
			if (player && target)
			{
				questModule.OnEntityKilled(this, killSource, player.GetIdentity().GetId());
			}
		}
	}
};