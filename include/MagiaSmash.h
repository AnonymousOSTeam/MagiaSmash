/**
 * @file MagiaSmash.h
 * @date 20-Oct-2026
 */

#ifndef __MagiaSmash_H__
#define __MagiaSmash_H__

#include "Scroll/Scroll.h"

/** Game Class
 */
class MagiaSmash : public Scroll<MagiaSmash>
{
public:


private:

                orxSTATUS       Bootstrap() const;

                void            Update(const orxCLOCK_INFO &_rstInfo);
                void            CameraUpdate(const orxCLOCK_INFO &_rstInfo);

                orxSTATUS       Init();
                orxSTATUS       Run();
                void            Exit();


private:
};

#endif // __MagiaSmash_H__
