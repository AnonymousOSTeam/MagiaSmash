/**
 * @file GameHUD.h
 * @date 20-Oct-2026
 *
 * MagiaSmash HUD system.  Bound to the [HUD] config section, reads the
 * shared [MatchState] section (written by the Fighter system) and drives
 * the HUD's child objects (health bar, labels, messages).
 */

#ifndef __GAMEHUD_H__
#define __GAMEHUD_H__

#include "MagiaSmash.h"

MakeScrollObject(GameHUD)
{
protected:

                void            OnCreate();
                void            Update(const orxCLOCK_INFO &_rstInfo);

private:

                orxOBJECT *     FindOwnedChildByName(const orxSTRING _zName);

private:

  orxOBJECT *     mpstHPFill;
  orxOBJECT *     mpstWaveLabel;
  orxOBJECT *     mpstFoesLabel;
  orxOBJECT *     mpstMessageLabel;
  orxFLOAT        mfFillBaseWidth;
  orxFLOAT        mfTime;
  orxFLOAT        mfLastHealthRatio;
  orxS32          msLastWave;
  orxS32          msLastFoes;
};

#endif // __GAMEHUD_H__
