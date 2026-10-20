/**
 * @file Fighter.h
 * @date 20-Oct-2026
 *
 * MagiaSmash combat system.
 *
 * A single C++ class drives every combatant (player + enemies).  All of its
 * stats and moves are read from config, so new characters/moves are pure INI
 * content: sections deriving from [Fighter] automatically get this class
 * through Scroll's name-based binder.
 */

#ifndef __FIGHTER_H__
#define __FIGHTER_H__

#include "MagiaSmash.h"

MakeScrollObject(Fighter)
{
public:

  enum State
  {
    StateIdle = 0,
    StateRun,
    StateAir,
    StateAttack,
    StateDash,
    StateHitstun,
    StateDead
  };

  //! Attack move definition, parsed from its config section
  struct MoveDef
  {
    orxFLOAT  fStartup;
    orxFLOAT  fActive;
    orxFLOAT  fRecovery;
    orxFLOAT  fDamage;
    orxFLOAT  fHitstun;
    orxFLOAT  fHitStop;
    orxFLOAT  fLunge;
    orxVECTOR vKnockback;
    orxVECTOR vHitboxSize;
    orxVECTOR vHitboxOffset;
  };

  static Fighter  *GetPlayer()                   {return s_poPlayer;}

  orxFLOAT        GetHealth() const              {return mfHealth;}
  orxFLOAT        GetMaxHealth() const           {return mfMaxHealth;}
  orxBOOL         IsDead() const                 {return (meState == StateDead) ? orxTRUE : orxFALSE;}
  orxS32          GetTeam() const                {return msTeam;}
  orxFLOAT        GetFacing() const              {return mfFacing;}

protected:

                void            OnCreate();
                void            OnDelete();
                void            Update(const orxCLOCK_INFO &_rstInfo);

private:

                void            SetState(State _eState);
                void            ReadMove(const orxSTRING _zMoveName, MoveDef &_rstMove);
                void            StartMove(const MoveDef &_rstMove);
                void            CheckHits();
                orxBOOL         TakeDamage(orxFLOAT _fDamage, const orxVECTOR &_rvKnockback, orxFLOAT _fHitstun, orxFLOAT _fHitStop, Fighter *_poAttacker);
                void            Die(const orxVECTOR &_rvKnockback);
                Fighter        *FindNearestOpponent();
                void            UpdatePlayerInput();
                void            UpdateAI(orxFLOAT _fDT);
                void            Integrate(orxFLOAT _fDT);
                void            ExportMatchState();

                static void     AddToRegistry(Fighter *_poFighter);
                static void     RemoveFromRegistry(Fighter *_poFighter);
                static void     FireCameraShake();
                static void     SpawnEffect(const orxSTRING _zConfigName, const orxVECTOR &_rvPosition, orxFLOAT _fFacing);

private:

  // Config-driven stats
  orxFLOAT        mfMaxHealth;
  orxFLOAT        mfMoveSpeed;
  orxFLOAT        mfJumpVelocity;
  orxFLOAT        mfGravity;
  orxFLOAT        mfDashSpeed;
  orxFLOAT        mfDashTime;
  orxFLOAT        mfKnockbackDamping;
  orxFLOAT        mfDeathFade;
  orxFLOAT        mfDamageScale;
  orxFLOAT        mfAIAttackRange;
  orxFLOAT        mfAIAttackDelay;
  orxFLOAT        mfArenaGroundY;
  orxFLOAT        mfArenaWallX;
  orxVECTOR       mvSize;
  const orxSTRING mzInputSet;
  orxS32          msTeam;
  orxBOOL         mbPlayer;
  orxBOOL         mbAI;
  orxBOOL         mbHasHeavyMove;

  // Moves
  MoveDef         mastCombo[4];
  orxS32          msComboCount;
  MoveDef         stHeavyMove;
  MoveDef         mastAIMoves[4];
  orxS32          msAIMoveCount;

  // Runtime state
  State           meState;
  orxVECTOR       mvVelocity;
  orxFLOAT        mfFacing;
  orxFLOAT        mfHealth;
  orxFLOAT        mfStateTime;
  orxFLOAT        mfStateDuration;
  orxFLOAT        mfHitstop;
  orxFLOAT        mfIframes;
  orxFLOAT        mfLungeTime;
  orxFLOAT        mfFlashTimer;
  orxFLOAT        mfAICooldown;
  orxS32          msComboIndex;
  orxBOOL         mbComboQueued;
  orxBOOL         mbGrounded;
  orxBOOL         mbHitRegistered;
  orxBOOL         mbCountedInMatch;
  const MoveDef  *mpoCurrentMove;
  orxCOLOR        stBaseColor;

  // Registry (single linked list for combat queries)
  Fighter        *mpoPrev;
  Fighter        *mpoNext;
  static Fighter *s_poFirst;
  static Fighter *s_poPlayer;
};

#endif // __FIGHTER_H__
