extern UINT8 *NMK004OKIROM0;
extern UINT8 *NMK004OKIROM1;
extern UINT8 *NMK004PROGROM;

void NMK004_init();

void NMK004Write(INT32, INT32 data);
UINT8 NMK004Read();

void NMK004_irq(INT32 irq);
INT32 NMK004Scan(INT32 nAction, INT32* pnMin);
