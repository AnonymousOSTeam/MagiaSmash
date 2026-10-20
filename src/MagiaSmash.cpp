/**
 * @file MagiaSmash.cpp
 * @date 20-Oct-2026
 */

#define __SCROLL_IMPL__
#include "MagiaSmash.h"
#undef __SCROLL_IMPL__

/* orxExtensions.h transitively includes the bundled third-party FastNoiseLite.h,
   whose cellular-noise loops trigger -Waggressive-loop-optimizations under -O2.
   The warning is confined to that third-party header, so silence it locally. */
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
#endif

#include "orxExtensions.h"

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif

#ifdef __orxMSVC__

/* Requesting high performance dedicated GPU on hybrid laptops */
__declspec(dllexport) unsigned long NvOptimusEnablement        = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // __orxMSVC__

/** Update function, it has been registered to be called every tick of the core clock
 */
void MagiaSmash::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  // Call Python update (game director + all game logic)
  orxPy_Update(&_rstClockInfo, orxNULL);
}

/** Camera Update function, it has been registered to be called every tick of the core clock, after physics & objects have been updated
*/
void MagiaSmash::CameraUpdate(const orxCLOCK_INFO &_rstClockInfo)
{
  // Call Python camera update
  orxPy_CameraUpdate(&_rstClockInfo, orxNULL);
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS MagiaSmash::Init()
{
  // Init extensions
  InitExtensions();
  // Done!
  return orxSTATUS_SUCCESS;
}

/** Run function, it should not contain any game logic
 */
orxSTATUS MagiaSmash::Run()
{
  // Return orxSTATUS_FAILURE to instruct orx to quit
  return orxSTATUS_SUCCESS;
}

/** Exit function, it is called before exiting from orx
 */
void MagiaSmash::Exit()
{
  // Exit from extensions
  ExitExtensions();

  // Let orx clean all our mess automatically. :)
}

/** Bootstrap function, it is called before config is initialized, allowing for early resource storage definitions
 */
orxSTATUS MagiaSmash::Bootstrap() const
{
  // Bootstrap extensions
  BootstrapExtensions();

  // Return orxSTATUS_FAILURE to prevent orx from loading the default config file
  return orxSTATUS_SUCCESS;
}

/** Main function
 */
int main(int argc, char **argv)
{
  // Execute our game
  MagiaSmash::GetInstance().Execute(argc, argv);

  // Done!
  return EXIT_SUCCESS;
}
