/**
 * @file Fighter.cpp
 * @date 20-Oct-2026
 *
 * MagiaSmash combat system implementation.
 *
 * Every combatant (Saya, chiropterans, ...) is a [Fighter]-derived config
 * section driven by this class.  Physics is a simple manual integrator
 * (deterministic, fighting-game friendly): velocity + gravity + ground/wall
 * clamping from the [Arena] section.  Hit detection is manual AABB between
 * active attack hitboxes and opponent hurtboxes.
 */

#include "Fighter.h"

//! Registry + player statics
Fighter *Fighter::s_poFirst  = orxNULL;
Fighter *Fighter::s_poPlayer = orxNULL;

//! Match-state section name (C++ -> Python data bridge)
static const orxSTRING szMatchStateSection = "MatchState";

//! Config helpers (with defaults)
static orxFLOAT MSXConfigFloat(const orxSTRING _zKey, orxFLOAT _fDefault)
{
  return orxConfig_HasValue(_zKey) ? orxConfig_GetFloat(_zKey) : _fDefault;
}

static const orxSTRING MSXConfigString(const orxSTRING _zKey, const orxSTRING _zDefault)
{
  const orxSTRING zValue = orxConfig_GetString(_zKey);
  return (*zValue != orxCHAR_NULL) ? zValue : _zDefault;
}

void Fighter::AddToRegistry(Fighter *_poFighter)
{
  _poFighter->mpoPrev = orxNULL;
  _poFighter->mpoNext = s_poFirst;
  if(s_poFirst != orxNULL)
  {
    s_poFirst->mpoPrev = _poFighter;
  }
  s_poFirst = _poFighter;
}

void Fighter::RemoveFromRegistry(Fighter *_poFighter)
{
  if(_poFighter->mpoPrev != orxNULL)
  {
    _poFighter->mpoPrev->mpoNext = _poFighter->mpoNext;
  }
  else
  {
    s_poFirst = _poFighter->mpoNext;
  }
  if(_poFighter->mpoNext != orxNULL)
  {
    _poFighter->mpoNext->mpoPrev = _poFighter->mpoPrev;
  }
}

void Fighter::FireCameraShake()
{
  // Gets the camera proxy object created by [MainCamera]'s OnCreate command
  orxOBJECT *poCamera = orxNULL;

  orxConfig_PushSection("MainCamera");
  if(orxConfig_HasValue("ID"))
  {
    poCamera = (orxOBJECT *)orxStructure_Get(orxConfig_GetU64("ID"));
  }
  orxConfig_PopSection();

  if(poCamera != orxNULL)
  {
    (void)orxObject_AddFX(poCamera, "MiniShake");
  }
}

void Fighter::SpawnEffect(const orxSTRING _zConfigName, const orxVECTOR &_rvPosition, orxFLOAT _fFacing)
{
  orxOBJECT *poEffect = orxObject_CreateFromConfig(_zConfigName);

  if(poEffect != orxNULL)
  {
    // Mirrored through 180 degrees when facing left (visually identical for
    // symmetric shapes, correctly mirrored for tilted ones)
    (void)orxObject_SetPosition(poEffect, &_rvPosition);
    (void)orxObject_SetRotation(poEffect, (_fFacing >= orxFLOAT_0) ? orxFLOAT_0 : orx2F(180));
  }
}

void Fighter::ReadMove(const orxSTRING _zMoveName, MoveDef &_rstMove)
{
  orxConfig_PushSection(_zMoveName);

  _rstMove.fStartup        = MSXConfigFloat("Startup", orxFLOAT_0);
  _rstMove.fActive         = MSXConfigFloat("Active", orxFLOAT_1);
  _rstMove.fRecovery       = MSXConfigFloat("Recovery", orxFLOAT_0);
  _rstMove.fDamage         = MSXConfigFloat("Damage", orxFLOAT_1);
  _rstMove.fHitstun        = MSXConfigFloat("Hitstun", orx2F(0.25));
  _rstMove.fHitStop        = MSXConfigFloat("HitStop", orxFLOAT_0);
  _rstMove.fLunge          = MSXConfigFloat("Lunge", orxFLOAT_0);
  orxConfig_GetVector("Knockback", &_rstMove.vKnockback);
  orxConfig_GetVector("HitboxSize", &_rstMove.vHitboxSize);
  orxConfig_GetVector("HitboxOffset", &_rstMove.vHitboxOffset);

  orxConfig_PopSection();
}

void Fighter::OnCreate()
{
  orxS32 i;

  // Instance section ("Saya", "Chiropteran", ...) is already pushed by Scroll
  mfMaxHealth        = MSXConfigFloat("Health", orx2F(100));
  mfMoveSpeed        = MSXConfigFloat("MoveSpeed", orx2F(600));
  mfJumpVelocity     = MSXConfigFloat("JumpVelocity", orx2F(1500));
  mfGravity          = MSXConfigFloat("Gravity", orx2F(3600));
  mfDashSpeed        = MSXConfigFloat("DashSpeed", orx2F(1400));
  mfDashTime         = MSXConfigFloat("DashTime", orx2F(0.16));
  mfKnockbackDamping = MSXConfigFloat("KnockbackDamping", orx2F(0.05));
  mfDeathFade        = MSXConfigFloat("DeathFade", orx2F(1.2));
  mfDamageScale      = MSXConfigFloat("DamageScale", orxFLOAT_1);
  mfAIAttackRange    = MSXConfigFloat("AIAttackRange", orx2F(150));
  mfAIAttackDelay    = MSXConfigFloat("AIAttackDelay", orx2F(1.2));
  mvSize             = orxVECTOR_0;
  orxConfig_GetVector("Size", &mvSize);
  if(mvSize.fX <= orxFLOAT_0) { mvSize.fX = orx2F(80); }
  if(mvSize.fY <= orxFLOAT_0) { mvSize.fY = orx2F(160); }
  // NB: reads <RGB> (0.0-1.0), matching orx's normalized color key. The
  // engine's <Color> key is byte-based (0-255), which would need /255 here.
  stBaseColor.vRGB   = orxVECTOR_1;
  orxConfig_GetVector("RGB", &stBaseColor.vRGB);
  stBaseColor.fAlpha = orxFLOAT_1;
  mzInputSet         = MSXConfigString("Input", orxSTRING_EMPTY);
  msTeam             = orxConfig_HasValue("Team") ? orxConfig_GetS32("Team") : 1;
  mbPlayer           = orxConfig_GetBool("IsPlayer");
  mbAI               = orxConfig_GetBool("AI");

  // Arena bounds
  orxConfig_PushSection("Arena");
  mfArenaGroundY = MSXConfigFloat("GroundY", orx2F(-400));
  mfArenaWallX   = MSXConfigFloat("WallX", orx2F(900));
  orxConfig_PopSection();

  // Moves: combo chain + heavy + AI list
  msComboCount = orxConfig_GetListCount("ComboList");
  if(msComboCount > 4) { msComboCount = 4; }
  for(i = 0; i < msComboCount; i++)
  {
    ReadMove(orxConfig_GetListString("ComboList", i), mastCombo[i]);
  }
  mbHasHeavyMove = orxConfig_HasValue("HeavyMove");
  if(mbHasHeavyMove)
  {
    ReadMove(orxConfig_GetString("HeavyMove"), stHeavyMove);
  }
  else
  {
    orxMemory_Zero(&stHeavyMove, sizeof(MoveDef));
  }
  msAIMoveCount = orxConfig_GetListCount("AIMoveList");
  if(msAIMoveCount > 4) { msAIMoveCount = 4; }
  for(i = 0; i < msAIMoveCount; i++)
  {
    ReadMove(orxConfig_GetListString("AIMoveList", i), mastAIMoves[i]);
  }

  // Runtime state
  meState         = StateIdle;
  mvVelocity      = orxVECTOR_0;
  mfFacing        = orxFLOAT_1;
  mfHealth        = mfMaxHealth;
  mfStateTime     = orxFLOAT_0;
  mfStateDuration = orxFLOAT_0;
  mfHitstop       = orxFLOAT_0;
  mfIframes       = orxFLOAT_0;
  mfLungeTime     = orxFLOAT_0;
  mfFlashTimer    = orxFLOAT_0;
  mfAICooldown    = orxMath_GetRandomFloat(orx2F(0.4), orx2F(1.2));
  msComboIndex    = 0;
  mbComboQueued   = orxFALSE;
  mbGrounded      = orxTRUE;
  mbHitRegistered = orxFALSE;
  mbCountedInMatch = orxFALSE;
  mpoCurrentMove  = orxNULL;
  mpoPrev         = orxNULL;
  mpoNext         = orxNULL;

  // Enable our input set once (player fighters)
  if(*mzInputSet != orxCHAR_NULL)
  {
    static orxBOOL sbInputSetEnabled = orxFALSE;
    if(!sbInputSetEnabled)
    {
      (void)orxInput_EnableSet(mzInputSet, orxTRUE);
      sbInputSetEnabled = orxTRUE;
    }
  }

  // Snap to ground (spawn Y is arbitrary; callers only set X)
  {
    orxVECTOR vPos;
    GetPosition(vPos, orxTRUE);
    vPos.fY = mfArenaGroundY + mvSize.fY * orx2F(0.5);
    SetPosition(vPos, orxTRUE);
  }

  // Apply base visual
  (void)orxObject_SetColor(GetOrxObject(), &stBaseColor);

  // Match-state bookkeeping
  if(mbPlayer)
  {
    s_poPlayer = this;
    orxConfig_PushSection(szMatchStateSection);
    orxConfig_SetFloat("PlayerHealth", mfHealth);
    orxConfig_SetFloat("PlayerMaxHealth", mfMaxHealth);
    orxConfig_SetFloat("PlayerPosX", orxFLOAT_0);
    orxConfig_PopSection();
  }
  else
  {
    mbCountedInMatch = orxTRUE;

    // Wave scaling: tougher enemies on later waves
    orxConfig_PushSection(szMatchStateSection);
    orxFLOAT fWaveScale = MSXConfigFloat("WaveScale", orxFLOAT_1);
    orxS32 s32Alive = orxConfig_GetS32("EnemiesAlive");
    orxConfig_SetS32("EnemiesAlive", s32Alive + 1);
    orxConfig_PopSection();

    mfHealth      = mfHealth * fWaveScale;
    mfMaxHealth   = mfHealth;
    mfDamageScale = mfDamageScale * fWaveScale;
  }

  AddToRegistry(this);
}

void Fighter::OnDelete()
{
  // Unlink from registry
  RemoveFromRegistry(this);

  // Bookkeeping
  if(mbPlayer)
  {
    if(s_poPlayer == this)
    {
      s_poPlayer = orxNULL;
    }
  }
  else if(mbCountedInMatch)
  {
    orxConfig_PushSection(szMatchStateSection);
    orxS32 s32Alive = orxConfig_GetS32("EnemiesAlive") - 1;
    orxConfig_SetS32("EnemiesAlive", (s32Alive > 0) ? s32Alive : 0);
    orxConfig_PopSection();
  }
}

void Fighter::SetState(State _eState)
{
  meState     = _eState;
  mfStateTime = orxFLOAT_0;
}

Fighter *Fighter::FindNearestOpponent()
{
  Fighter *poResult       = orxNULL;
  orxFLOAT fBestDistance   = orxFLOAT_MAX;

  for(Fighter *poFighter = s_poFirst; poFighter != orxNULL; poFighter = poFighter->mpoNext)
  {
    if((poFighter != this) && (poFighter->meState != StateDead) && (poFighter->msTeam != msTeam))
    {
      orxVECTOR vMyPos, vTheirPos;
      GetPosition(vMyPos, orxTRUE);
      poFighter->GetPosition(vTheirPos, orxTRUE);
      orxFLOAT fDistance = orxMath_Abs(vTheirPos.fX - vMyPos.fX);
      if(fDistance < fBestDistance)
      {
        fBestDistance = fDistance;
        poResult      = poFighter;
      }
    }
  }

  return poResult;
}

void Fighter::StartMove(const MoveDef &_rstMove)
{
  mpoCurrentMove  = &_rstMove;
  mbComboQueued   = orxFALSE;
  mbHitRegistered = orxFALSE;
  mfLungeTime     = orxFLOAT_0;
  mfStateDuration = _rstMove.fStartup + _rstMove.fActive + _rstMove.fRecovery;
  SetState(StateAttack);

  // Auto-face the nearest opponent when starting an attack (grounded)
  if(mbGrounded)
  {
    Fighter *poTarget = FindNearestOpponent();
    if(poTarget != orxNULL)
    {
      orxVECTOR vMyPos, vTheirPos;
      GetPosition(vMyPos, orxTRUE);
      poTarget->GetPosition(vTheirPos, orxTRUE);
      mfFacing = (vTheirPos.fX >= vMyPos.fX) ? orxFLOAT_1 : -orxFLOAT_1;
    }
  }

  // Kill horizontal momentum when grounded
  if(mbGrounded)
  {
    mvVelocity.fX = orxFLOAT_0;
  }
}

void Fighter::CheckHits()
{
  MoveDef &roMove = *(MoveDef *)mpoCurrentMove;

  // One hit per attack
  if(mbHitRegistered)
  {
    return;
  }

  orxVECTOR vMyPos;
  GetPosition(vMyPos, orxTRUE);

  // World-space hitbox
  orxFLOAT fBoxX     = vMyPos.fX + roMove.vHitboxOffset.fX * mfFacing;
  orxFLOAT fBoxY     = vMyPos.fY + roMove.vHitboxOffset.fY;
  orxFLOAT fBoxHalfW = roMove.vHitboxSize.fX * orx2F(0.5);
  orxFLOAT fBoxHalfH = roMove.vHitboxSize.fY * orx2F(0.5);

  for(Fighter *poFighter = s_poFirst; poFighter != orxNULL; poFighter = poFighter->mpoNext)
  {
    if((poFighter == this) || (poFighter->meState == StateDead) || (poFighter->msTeam == msTeam))
    {
      continue;
    }

    orxVECTOR vTheirPos;
    poFighter->GetPosition(vTheirPos, orxTRUE);

    orxFLOAT fTheirHalfW = poFighter->mvSize.fX * orx2F(0.5);
    orxFLOAT fTheirHalfH = poFighter->mvSize.fY * orx2F(0.5);

    // AABB overlap test
    if((orxMath_Abs(fBoxX - vTheirPos.fX) <= fBoxHalfW + fTheirHalfW)
    && (orxMath_Abs(fBoxY - vTheirPos.fY) <= fBoxHalfH + fTheirHalfH))
    {
      // Signed knockback (away from attacker)
      orxVECTOR vKnockback = roMove.vKnockback;
      vKnockback.fX = vKnockback.fX * mfFacing;

      mbHitRegistered = orxTRUE;

      // Apply
      if(poFighter->TakeDamage(roMove.fDamage * mfDamageScale, vKnockback, roMove.fHitstun, roMove.fHitStop, this))
      {
        // Hit feedback: spark at contact point + camera shake
        orxFLOAT fSide = (vTheirPos.fX > fBoxX) ? orxFLOAT_1 : -orxFLOAT_1;
        orxVECTOR vSpark;
        vSpark.fX = vTheirPos.fX + fTheirHalfW * orx2F(0.6) * fSide;
        vSpark.fY = vTheirPos.fY + fTheirHalfH * orx2F(0.4);
        vSpark.fZ = orx2F(0.2);
        SpawnEffect("HitSpark", vSpark, mfFacing);
        FireCameraShake();
      }

      // Only one victim per swing
      break;
    }
  }
}

orxBOOL Fighter::TakeDamage(orxFLOAT _fDamage, const orxVECTOR &_rvKnockback, orxFLOAT _fHitstun, orxFLOAT _fHitStop, Fighter *_poAttacker)
{
  // Invulnerable (dashing) or already down
  if((meState == StateDead) || (mfIframes > orxFLOAT_0))
  {
    return orxFALSE;
  }

  // Damage
  mfHealth = mfHealth - _fDamage;
  if(mfHealth < orxFLOAT_0)
  {
    mfHealth = orxFLOAT_0;
  }

  // Hitstop for both sides
  mfHitstop = orxMAX(mfHitstop, _fHitStop * orx2F(1.5));
  if(_poAttacker != orxNULL)
  {
    _poAttacker->mfHitstop = orxMAX(_poAttacker->mfHitstop, _fHitStop);
  }

  // White flash
  mfFlashTimer = orx2F(0.09);
  {
    orxCOLOR stFlash;
    stFlash.vRGB.fX = stFlash.vRGB.fY = stFlash.vRGB.fZ = orxFLOAT_1;
    stFlash.fAlpha = orxFLOAT_1;
    (void)orxObject_SetColor(GetOrxObject(), &stFlash);
  }

  // Knockback + hitstun
  mvVelocity = _rvKnockback;
  if(_rvKnockback.fY > orx2F(50))
  {
    mbGrounded = orxFALSE;
  }
  mfStateDuration = _fHitstun;
  SetState(StateHitstun);
  mbComboQueued = orxFALSE;

  if(mbPlayer)
  {
    ExportMatchState();
  }

  // Lethal?
  if(mfHealth <= orxFLOAT_0)
  {
    Die(_rvKnockback);
  }

  return orxTRUE;
}

void Fighter::Die(const orxVECTOR &_rvKnockback)
{
  SetState(StateDead);
  mfStateTime = orxFLOAT_0;

  // Corpse flies a little further and spins
  mvVelocity.fX = _rvKnockback.fX * orx2F(1.4);
  mvVelocity.fY = orxMAX(_rvKnockback.fY * orx2F(1.5), orx2F(700));
  mbGrounded    = orxFALSE;
  (void)orxObject_SetAngularVelocity(GetOrxObject(), orxMath_GetRandomFloat(orx2F(-360), orx2F(360)));
  (void)orxObject_SetLifeTime(GetOrxObject(), mfDeathFade + orx2F(0.05));

  if(mbPlayer)
  {
    orxConfig_PushSection(szMatchStateSection);
    orxConfig_SetFloat("PlayerHealth", orxFLOAT_0);
    orxConfig_SetString("GameState", "GameOver");
    orxConfig_PopSection();
  }

  FireCameraShake();
}

void Fighter::UpdatePlayerInput()
{
  orxFLOAT fMoveValue = orxFLOAT_0;
  orxBOOL bJump       = orxFALSE;
  orxBOOL bDash       = orxFALSE;
  orxBOOL bLight      = orxFALSE;
  orxBOOL bHeavy      = orxFALSE;

  if(*mzInputSet != orxCHAR_NULL)
  {
    (void)orxInput_PushSet(mzInputSet);
    fMoveValue = orxInput_GetValue("MoveRight") - orxInput_GetValue("MoveLeft");
    bJump      = orxInput_HasBeenActivated("Jump");
    bDash      = orxInput_HasBeenActivated("Dash");
    bLight     = orxInput_HasBeenActivated("LightAttack");
    bHeavy     = orxInput_HasBeenActivated("HeavyAttack");
    (void)orxInput_PopSet();
  }

  // Facing follows movement input
  if(fMoveValue > orxFLOAT_0) { mfFacing = orxFLOAT_1; }
  else if(fMoveValue < orxFLOAT_0) { mfFacing = -orxFLOAT_1; }

  // Jump
  if(bJump && mbGrounded)
  {
    mvVelocity.fY = mfJumpVelocity;
    mbGrounded    = orxFALSE;
    SetState(StateAir);
  }

  // Dash (grounded burst with i-frames)
  if(bDash && mbGrounded && (mfIframes <= orxFLOAT_0))
  {
    orxFLOAT fDirection = (fMoveValue != orxFLOAT_0) ? ((fMoveValue > orxFLOAT_0) ? orxFLOAT_1 : -orxFLOAT_1) : mfFacing;
    mfFacing            = fDirection;
    SetState(StateDash);
    mvVelocity.fX = fDirection * mfDashSpeed;
    mfIframes     = mfDashTime + orx2F(0.08);
    (void)orxObject_AddFX(GetOrxObject(), "DashPulse");
  }

  // Attacks
  if(bLight && (msComboCount > 0))
  {
    if(meState == StateAttack)
    {
      // Chain: queue the next combo hit during active/recovery frames
      if((mpoCurrentMove == &mastCombo[msComboIndex])
      && (msComboIndex + 1 < msComboCount)
      && (mfStateTime > orx2F(0.5) * (mpoCurrentMove->fStartup + mpoCurrentMove->fActive)))
      {
        mbComboQueued = orxTRUE;
      }
    }
    else if((meState != StateDash) && (meState != StateHitstun) && (meState != StateDead))
    {
      msComboIndex = 0;
      StartMove(mastCombo[0]);
    }
  }
  else if(bHeavy && mbHasHeavyMove)
  {
    if((meState != StateAttack) && (meState != StateDash) && (meState != StateHitstun) && (meState != StateDead))
    {
      StartMove(stHeavyMove);
    }
  }

  // Locomotion (only when free to act)
  if((meState == StateIdle) || (meState == StateRun) || (meState == StateAir))
  {
    mvVelocity.fX = fMoveValue * mfMoveSpeed;
    if(mbGrounded)
    {
      SetState((fMoveValue != orxFLOAT_0) ? StateRun : StateIdle);
    }
    else
    {
      SetState(StateAir);
    }
  }
}

void Fighter::UpdateAI(orxFLOAT _fDT)
{
  // Only acts when free to move
  if((meState != StateIdle) && (meState != StateRun) && (meState != StateAir))
  {
    return;
  }

  Fighter *poTarget = s_poPlayer;

  // No target or target down: idle sway
  if((poTarget == orxNULL) || (poTarget->meState == StateDead))
  {
    mvVelocity.fX = mvVelocity.fX * orxMath_Pow(orx2F(0.01), _fDT);
    if(mbGrounded)
    {
      SetState(StateIdle);
    }
    return;
  }

  orxVECTOR vMyPos, vTheirPos;
  GetPosition(vMyPos, orxTRUE);
  poTarget->GetPosition(vTheirPos, orxTRUE);

  orxFLOAT fDX   = vTheirPos.fX - vMyPos.fX;
  orxFLOAT fDY   = orxMath_Abs(vTheirPos.fY - vMyPos.fY);
  orxFLOAT fAbsX = orxMath_Abs(fDX);

  // Face the target
  mfFacing = (fDX >= orxFLOAT_0) ? orxFLOAT_1 : -orxFLOAT_1;

  if((fAbsX > mfAIAttackRange) || (fDY > orx2F(250)))
  {
    // Chase
    orxFLOAT fChase = (fDX >= orxFLOAT_0) ? mfMoveSpeed : -mfMoveSpeed;
    mvVelocity.fX   = fChase * (mbGrounded ? orxFLOAT_1 : orx2F(0.5));
    if(mbGrounded)
    {
      SetState(StateRun);
    }
  }
  else
  {
    // In range: pause, then strike
    mvVelocity.fX = mvVelocity.fX * orxMath_Pow(orx2F(0.01), _fDT);
    if(mbGrounded)
    {
      SetState(StateIdle);
      mfAICooldown = mfAICooldown - _fDT;
      if((mfAICooldown <= orxFLOAT_0) && (msAIMoveCount > 0))
      {
        orxS32 s32Index = (orxS32)orxMath_GetRandomFloat(orxFLOAT_0, (orxFLOAT)msAIMoveCount - orx2F(0.001));
        StartMove(mastAIMoves[s32Index]);
        mfAICooldown = mfAIAttackDelay * orxMath_GetRandomFloat(orx2F(0.75), orx2F(1.4));
      }
    }
  }
}

void Fighter::Integrate(orxFLOAT _fDT)
{
  orxVECTOR vPos;
  GetPosition(vPos, orxTRUE);

  // Gravity when airborne
  if(!mbGrounded)
  {
    mvVelocity.fY = mvVelocity.fY - mfGravity * _fDT;
  }

  // Integrate
  vPos.fX = vPos.fX + mvVelocity.fX * _fDT;
  vPos.fY = vPos.fY + mvVelocity.fY * _fDT;

  // Ground clamp (feet)
  orxFLOAT fFloorY = mfArenaGroundY + mvSize.fY * orx2F(0.5);
  if(vPos.fY <= fFloorY)
  {
    vPos.fY      = fFloorY;
    mvVelocity.fY = orxFLOAT_0;
    if(!mbGrounded)
    {
      mbGrounded = orxTRUE;
      if(meState == StateAir)
      {
        SetState(StateIdle);
      }
    }
  }
  else if(mbGrounded && (mvVelocity.fY > orxFLOAT_0))
  {
    mbGrounded = orxFALSE;
    if((meState == StateIdle) || (meState == StateRun))
    {
      SetState(StateAir);
    }
  }

  // Wall clamp
  if(vPos.fX < -mfArenaWallX)
  {
    vPos.fX       = -mfArenaWallX;
    mvVelocity.fX = orxFLOAT_0;
  }
  else if(vPos.fX > mfArenaWallX)
  {
    vPos.fX       = mfArenaWallX;
    mvVelocity.fX = orxFLOAT_0;
  }

  SetPosition(vPos, orxTRUE);
}

void Fighter::ExportMatchState()
{
  orxVECTOR vPos;
  GetPosition(vPos, orxTRUE);
  orxConfig_PushSection(szMatchStateSection);
  orxConfig_SetFloat("PlayerHealth", mfHealth);
  orxConfig_SetFloat("PlayerPosX", vPos.fX);
  orxConfig_PopSection();
}

void Fighter::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  orxFLOAT fDT = _rstClockInfo.fDT;

  // Hitstop: frozen frames on impact
  if(mfHitstop > orxFLOAT_0)
  {
    mfHitstop = mfHitstop - fDT;
    return;
  }

  // Hit flash restore
  if(mfFlashTimer > orxFLOAT_0)
  {
    mfFlashTimer = mfFlashTimer - fDT;
    if(mfFlashTimer <= orxFLOAT_0)
    {
      (void)orxObject_SetColor(GetOrxObject(), &stBaseColor);
      mfFlashTimer = orxFLOAT_0;
    }
  }

  // I-frames tick
  if(mfIframes > orxFLOAT_0)
  {
    mfIframes = mfIframes - fDT;
  }

  switch(meState)
  {
    case StateDead:
    {
      mfStateTime = mfStateTime + fDT;
      {
        orxFLOAT fAlpha = orxFLOAT_1 - orxCLAMP(mfStateTime / mfDeathFade, orxFLOAT_0, orxFLOAT_1);
        (void)orxObject_SetAlpha(GetOrxObject(), fAlpha);
      }
      break;
    }

    case StateHitstun:
    {
      mfStateTime = mfStateTime + fDT;
      if(mbGrounded)
      {
        mvVelocity.fX = mvVelocity.fX * orxMath_Pow(mfKnockbackDamping, fDT);
      }
      if(mfStateTime >= mfStateDuration)
      {
        SetState(mbGrounded ? StateIdle : StateAir);
      }
      break;
    }

    case StateDash:
    {
      mfStateTime = mfStateTime + fDT;
      if(mfStateTime >= mfDashTime)
      {
        SetState(mbGrounded ? StateIdle : StateAir);
      }
      break;
    }

    case StateAttack:
    {
      orxFLOAT fPreviousTime = mfStateTime;
      mfStateTime            = mfStateTime + fDT;

      MoveDef &roMove        = *(MoveDef *)mpoCurrentMove;
      orxFLOAT fActiveStart  = roMove.fStartup;
      orxFLOAT fActiveEnd    = roMove.fStartup + roMove.fActive;

      // Lunge burst at the start of the active window
      if((mfStateTime >= fActiveStart) && (fPreviousTime < fActiveStart))
      {
        mvVelocity.fX = mfFacing * roMove.fLunge;
        mfLungeTime   = orx2F(0.12);

        // Slash arc visual at the hitbox
        {
          orxVECTOR vPos;
          GetPosition(vPos, orxTRUE);
          orxVECTOR vArc;
          vArc.fX = vPos.fX + roMove.vHitboxOffset.fX * mfFacing;
          vArc.fY = vPos.fY + roMove.vHitboxOffset.fY;
          vArc.fZ = orx2F(0.15);
          SpawnEffect("SlashArc", vArc, mfFacing);
        }
      }

      // Active window: hit detection
      if((mfStateTime >= fActiveStart) && (mfStateTime < fActiveEnd))
      {
        CheckHits();
      }

      // Horizontal control during the attack
      if(mbGrounded)
      {
        if(mfLungeTime > orxFLOAT_0)
        {
          mfLungeTime = mfLungeTime - fDT;
        }
        else
        {
          mvVelocity.fX = mvVelocity.fX * orxMath_Pow(orx2F(0.0008), fDT);
        }
      }

      // Attack over: chain or return to neutral
      if(mfStateTime >= mfStateDuration)
      {
        if(mbComboQueued && (msComboIndex + 1 < msComboCount))
        {
          msComboIndex++;
          StartMove(mastCombo[msComboIndex]);
        }
        else
        {
          mpoCurrentMove = orxNULL;
          SetState(mbGrounded ? StateIdle : StateAir);
        }
      }
      break;
    }

    case StateIdle:
    case StateRun:
    case StateAir:
    default:
    {
      if(mbPlayer)
      {
        UpdatePlayerInput();
      }
      else if(mbAI)
      {
        UpdateAI(fDT);
      }
      break;
    }
  }

  // Physics
  Integrate(fDT);

  // Visual facing
  (void)orxObject_SetFlip(GetOrxObject(), (mfFacing < orxFLOAT_0) ? orxTRUE : orxFALSE, orxFALSE);

  // Bridge to the Python director
  if(mbPlayer)
  {
    ExportMatchState();
  }
}
