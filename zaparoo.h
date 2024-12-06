// zaparoo.h
// 2024, Aitor Gomez Garcia (info@aitorgomez.net)

#ifndef __ZAPAROO_H__
#define __ZAPAROO_H__

#include "lib/imlib2/Imlib2.h"

extern char *zaparoo;
void getZaparoo();
bool zaparoo_waiting(int menu_bgn, Imlib_Image bg1, Imlib_Image bg2);

#endif // __ZAPAROO_H__