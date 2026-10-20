/**
 * @file GameHUD.cpp
 * @date 20-Oct-2026
 *
 * MagiaSmash HUD system implementation.
 */

#include "GameHUD.h"

#include <cstdio>
#include <cstring>
#include <cmath>

orxOBJECT *GameHUD::FindOwnedChildByName(const orxSTRING _zName)
{
  for(orxOBJECT *poChild = orxObject_GetOwnedChild(GetOrxObject());
      poChild != orxNULL;
      poChild = orxObject_GetOwnedSibling(poChild))
  {
    const orxSTRING zChildName = orxObject_GetName(poChild);
    if((zChildName != orxNULL) && (strcmp(zChildName, _zName) == 0))
    {
      return poChild;
    }
  }

  return orxNULL;
}

void GameHUD::OnCreate()
{
  // Cache child objects
  mpstHPFill        = FindOwnedChildByName("HPBarFill");
  mpstWaveLabel     = FindOwnedChildByName("WaveLabel");
  mpstFoesLabel     = FindOwnedChildByName("FoesLabel");
  mpstMessageLabel  = FindOwnedChildByName("MessageLabel");

  // Base width of the health bar fill (scaled down as health drops;
  // its pivot is 'left' so it shrinks from the right)
  if(mpstHPFill != orxNULL)
  {
    orxVECTOR vSize;
    (void)orxObject_GetSize(mpstHPFill, &vSize);
    mfFillBaseWidth = vSize.fX;
  }
  else
  {
    mfFillBaseWidth = orxFLOAT_1;
  }

  mfTime            = orxFLOAT_0;
  mfLastHealthRatio = -orxFLOAT_1;
  msLastWave        = -1;
  msLastFoes        = -1;
}

void GameHUD::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  orxFLOAT fDT        = _rstClockInfo.fDT;
  orxFLOAT fHealth    = orxFLOAT_0;
  orxFLOAT fMaxHealth = orxFLOAT_1;
  orxFLOAT fMsgTimer  = orxFLOAT_0;
  orxS32 s32Wave      = 0;
  orxS32 s32Foes      = 0;
  orxSTRING zGameState;
  const orxSTRING zMessage;
  orxCHAR acBuffer[64];

  mfTime = mfTime + fDT;

  // Read shared match state
  orxConfig_PushSection("MatchState");
  fHealth      = orxConfig_GetFloat("PlayerHealth");
  fMaxHealth   = orxConfig_GetFloat("PlayerMaxHealth");
  s32Wave      = orxConfig_GetS32("Wave");
  s32Foes      = orxConfig_GetS32("EnemiesAlive");
  zGameState   = (orxSTRING)orxConfig_GetString("GameState");
  zMessage     = orxConfig_GetString("Message");
  fMsgTimer    = orxConfig_GetFloat("MessageTimer");

  // Tick down the transient message timer
  if(fMsgTimer > orxFLOAT_0)
  {
    fMsgTimer = fMsgTimer - fDT;
    if(fMsgTimer < orxFLOAT_0)
    {
      fMsgTimer = orxFLOAT_0;
    }
    orxConfig_SetFloat("MessageTimer", fMsgTimer);
  }
  orxConfig_PopSection();

  // Health bar
  if((mpstHPFill != orxNULL) && (fMaxHealth > orxFLOAT_0))
  {
    orxFLOAT fRatio = orxCLAMP(fHealth / fMaxHealth, orxFLOAT_0, orxFLOAT_1);
    if(fabsf(fRatio - mfLastHealthRatio) > orx2F(0.001))
    {
      orxVECTOR vScale;
      vScale.fX = fRatio;
      vScale.fY = orxFLOAT_1;
      vScale.fZ = orxFLOAT_1;
      (void)orxObject_SetScale(mpstHPFill, &vScale);
      mfLastHealthRatio = fRatio;
    }

    // Low-health blink
    if((fRatio > orxFLOAT_0) && (fRatio < orx2F(0.25)))
    {
      (void)orxObject_SetAlpha(mpstHPFill, (fmodf(mfTime * orx2F(6), orxFLOAT_1) < orx2F(0.5f)) ? orx2F(0.45) : orxFLOAT_1);
    }
    else
    {
      (void)orxObject_SetAlpha(mpstHPFill, orxFLOAT_1);
    }
  }

  // Wave label
  if((mpstWaveLabel != orxNULL) && (s32Wave != msLastWave))
  {
    snprintf(acBuffer, sizeof(acBuffer), "WAVE %d", s32Wave);
    (void)orxObject_SetTextString(mpstWaveLabel, acBuffer);
    msLastWave = s32Wave;
  }

  // Foes label
  if((mpstFoesLabel != orxNULL) && (s32Foes != msLastFoes))
  {
    snprintf(acBuffer, sizeof(acBuffer), "FOES %d", (s32Foes > 0) ? s32Foes : 0);
    (void)orxObject_SetTextString(mpstFoesLabel, acBuffer);
    msLastFoes = s32Foes;
  }

  // Message label: game over banner wins, transient messages otherwise
  if(mpstMessageLabel != orxNULL)
  {
    const orxSTRING zText = orxSTRING_EMPTY;

    if(strcmp(zGameState, "Playing") != 0)
    {
      if(strcmp(zGameState, "GameOver") == 0)
      {
        zText = "GAME OVER - PRESS ENTER";
      }
      else if(strcmp(zGameState, "Victory") == 0)
      {
        zText = "VICTORY! - PRESS ENTER";
      }
    }
    else if(fMsgTimer > orxFLOAT_0)
    {
      zText = zMessage;
    }

    if(strcmp(orxObject_GetTextString(mpstMessageLabel), zText) != 0)
    {
      (void)orxObject_SetTextString(mpstMessageLabel, zText);
    }
  }
}
