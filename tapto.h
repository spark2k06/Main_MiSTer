// tapto.h
// 2024, Aitor Gomez Garcia (info@aitorgomez.net)

#ifndef __TAPTO_H__
#define __TAPTO_H__

#include "lib/imlib2/Imlib2.h"

extern char *tapto;
void getTapTo();
bool tapto_waiting(int menu_bgn, Imlib_Image bg1, Imlib_Image bg2);

#endif // __TAPTO_H__