#ifndef _EM_THROTTLE_H_
#define _EM_THROTTLE_H_

#ifndef EMSCRIPTEN
void RefreshThrottleFPS(void);
int SpeedThrottle(void);
void IncreaseEmulationSpeed(void);
void DecreaseEmulationSpeed(void);
#else
#define RefreshThrottleFPS()
#define SpeedThrottle() 0
#define IncreaseEmulationSpeed()
#define DecreaseEmulationSpeed()
#endif //EMSCRIPTEN

#endif
