#ifndef NOCHEAT

void FCEU_CheatResetRAM(void);
void FCEU_CheatAddRAM(int s, uint32 A, uint8 *p);

void FCEU_LoadGameCheats(FILE *override);
void FCEU_FlushGameCheats(FILE *override, int nosave);
void FCEU_ApplyPeriodicCheats(void);
void FCEU_PowerCheats(void);

int FCEU_CheatGetByte(uint32 A);
void FCEU_CheatSetByte(uint32 A, uint8 V);

extern int savecheats;

#else

#define FCEU_CheatResetRAM(...)
#define FCEU_CheatAddRAM(...)
#define FCEU_LoadGameCheats(...)
#define FCEU_FlushGameCheats(...)
#define FCEU_ApplyPeriodicCheats(...)
#define FCEU_PowerCheats(...)
#define FCEU_CheatGetByte(...) (0)
#define FCEU_CheatSetByte(...)

#endif // NOCHEAT
