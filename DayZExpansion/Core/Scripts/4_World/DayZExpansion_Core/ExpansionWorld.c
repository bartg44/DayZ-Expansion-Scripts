/**
 * ExpansionWorld.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionWorld: ExpansionGame
{
	void ExpansionWorld()
	{
		ExpansionAttachmentHelper.Init();
	}

	override void FirearmEffects(Object source, Object directHit, int componentIndex, string surface, vector pos, vector surfNormal, vector exitPos, vector inSpeed, vector outSpeed, bool isWater, bool deflected, string ammoType) 
	{
#ifdef DIAG
		string msg = "::FirearmEffects ";
		msg += "" + source + " ";
		msg += "" + directHit + " ";
		msg += "" + componentIndex + " ";
		msg += "" + surface + " ";
		msg += "" + pos + " ";
		msg += "" + surfNormal + " ";
		msg += "" + exitPos + " ";
		msg += "" + inSpeed + " ";
		msg += "" + outSpeed + " ";
		msg += "" + isWater + " ";
		msg += "" + deflected + " ";
		msg += "" + ammoType;
		EXTrace.Print(EXTrace.WEAPONS, this, msg);
#endif

		if (GetGame().IsServer())
		{
			if (source && source.ShootsExplosiveAmmo() && !deflected && outSpeed == vector.Zero)
			{
				if (GetExpansionSettings().GetDamageSystem().Enabled)
				{
					string explosionAmmoType = GetExpansionSettings().GetDamageSystem().ExplosiveProjectiles[ammoType];
					if (explosionAmmoType)
						ExpansionDamageSystem.OnBeforeExplode(EntityAI.Cast(source), DT_EXPLOSION, explosionAmmoType, pos);
				}
			}
		}

		super.FirearmEffects(source, directHit, componentIndex, surface, pos, surfNormal, exitPos, inSpeed, outSpeed, isWater, deflected, ammoType);
	}

	override void WriteRemovedWorldObjects(ParamsReadContext ctx)
	{
		ExpansionWorldObjectsModule.WriteRemovedObjects(ctx);
	}

	override void ReadRemovedWorldObjects(ParamsReadContext ctx)
	{
		ExpansionWorldObjectsModule.RPC_RemoveObjects(ctx);
	}

	static void CheckTreeContact(IEntity other, float impulse)
	{
		if (impulse < 7500)
			return;

		Plant plant;
		if (!Plant.CastTo(plant, other))
			return;

		if (!GetGame().IsDedicatedServer())
			PlayFellPlantSound(plant);

		if (GetGame().IsServer())
			plant.SetHealth(0);

		dBodyDestroy(plant);
	}

	static void PlayFellPlantSound(Object plant)
	{
		EffectSound sound;

		if (plant.IsTree())
		{
			if (plant.IsInherited(TreeHard))
				sound =	SEffectManager.PlaySound( "hardTreeFall_SoundSet", plant.GetPosition() );
			else if (plant.IsInherited(TreeSoft))
				sound =	SEffectManager.PlaySound( "softTreeFall_SoundSet", plant.GetPosition() );
		}

		if (plant.IsBush())
		{
			if (plant.IsInherited(BushHard))
				sound =	SEffectManager.PlaySound( "hardBushFall_SoundSet", plant.GetPosition() );
			else if (plant.IsInherited(BushSoft))
				sound =	SEffectManager.PlaySound( "softBushFall_SoundSet", plant.GetPosition() );
		}

		if (sound)
			sound.SetAutodestroy( true );
	}
};
