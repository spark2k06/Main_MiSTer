// loadscreen.h
// 2024, Aitor Gomez Garcia (info@aitorgomez.net)

#ifndef __LOADSCREEN_H__
#define __LOADSCREEN_H__

#include "lib/imlib2/Imlib2.h"

Imlib_Image load_screen_bg();
void fade_in_screen(const char *s);
void fade_out_screen();

#endif // __LOADSCREEN_H__