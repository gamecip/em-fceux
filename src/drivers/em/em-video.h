#ifndef __FCEU_SDL_VIDEO_H
#define __FCEU_SDL_VIDEO_H
#ifdef _SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

void PtoV(int *x, int *y);
bool FCEUD_ShouldDrawInputAids();
bool FCEUI_AviDisableMovieMessages();
static SDL_Surface *s_screen;
bool FCEUI_AviEnableHUDrecording();
void FCEUI_SetAviEnableHUDrecording(bool enable);
bool FCEUI_AviDisableMovieMessages();
void FCEUI_SetAviDisableMovieMessages(bool disable);
#endif

