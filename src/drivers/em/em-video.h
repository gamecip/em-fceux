#ifndef __FCEU_SDL_VIDEO_H
#define __FCEU_SDL_VIDEO_H

void PtoV(int *x, int *y);
bool FCEUD_ShouldDrawInputAids();
bool FCEUI_AviDisableMovieMessages();
bool FCEUI_AviEnableHUDrecording();
void FCEUI_SetAviEnableHUDrecording(bool enable);
bool FCEUI_AviDisableMovieMessages();
void FCEUI_SetAviDisableMovieMessages(bool disable);
#endif

