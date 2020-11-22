/**
 * DayZPlayerCamera3rdPerson.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2020 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class DayZPlayerCamera3rdPerson
{
	override void OnActivate( DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult )
	{
		#ifdef EXPANSIONEXPRINT
		EXPrint("DayZPlayerCamera1stPersonVehicle::OnActivate Start");
		#endif
		
		super.OnActivate( pPrevCamera, pPrevCameraResult );

		#ifdef EXPANSIONEXPRINT
		EXPrint("DayZPlayerCamera1stPersonVehicle::OnActivate End");
		#endif
	}

	override void OnUpdate( float pDt, out DayZPlayerCameraResult pOutResult )
	{
		super.OnUpdate( pDt, pOutResult );
		
		#ifdef EXPANSIONEXPRINT
		EXPrint("DayZPlayerCamera1stPersonVehicle::OnUpdate Start");
		#endif

		PlayerBase.Cast( m_pPlayer ).SetHeadInvisible( false );
		
		#ifdef EXPANSIONEXPRINT
		EXPrint("DayZPlayerCamera1stPersonVehicle::OnUpdate End");
		#endif
	}
}