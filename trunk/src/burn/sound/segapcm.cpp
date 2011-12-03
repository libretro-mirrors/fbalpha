#include "burnint.h"
#include "burn_sound.h"
#include "segapcm.h"

struct segapcm
{
	UINT8  *ram;
	UINT8 low[16];
	const UINT8 *rom;
	INT32 bankshift;
	INT32 bankmask;
	INT32 UpdateStep;
};

static struct segapcm *Chip = NULL;
static INT32 *Left = NULL;
static INT32 *Right = NULL;

void SegaPCMUpdate(INT16* pSoundBuf, INT32 nLength)
{
	INT32 Channel;
	
	memset(Left, 0, nLength * sizeof(INT32));
	memset(Right, 0, nLength * sizeof(INT32));

	for (Channel = 0; Channel < 16; Channel++) {
		UINT8 *Regs = Chip->ram + 8 * Channel;
		if (!(Regs[0x86] & 1)) {
			const UINT8 *Rom = Chip->rom + ((Regs[0x86] & Chip->bankmask) << Chip->bankshift);
			UINT32 Addr = (Regs[0x85] << 16) | (Regs[0x84] << 8) | Chip->low[Channel];
			UINT32 Loop = (Regs[0x05] << 16) | (Regs[0x04] << 8);
			UINT8 End = Regs[6] + 1;
			INT32 i;
			
			for (i = 0; i < nLength; i++) {
				INT8 v = 0;

				if ((Addr >> 16) == End) {
					if (Regs[0x86] & 2) {
						Regs[0x86] |= 1;
						break;
					} else {
						Addr = Loop;
					}
				}

				v = Rom[Addr >> 8] - 0x80;

				Left[i] += v * Regs[2];
				Right[i] += v * Regs[3];
				Addr = (Addr + ((Regs[7] * Chip->UpdateStep) >> 16)) & 0xffffff;
			}

			Regs[0x84] = Addr >> 8;
			Regs[0x85] = Addr >> 16;
			Chip->low[Channel] = Regs[0x86] & 1 ? 0 : Addr;
		}
	}
	
	for (INT32 i = 0; i < nLength; i++) {
		if (Left[i] > 32767) Left[i] = 32767;
		if (Left[i] < -32768) Left[i] = -32768;
		
		if (Right[i] > 32767) Right[i] = 32767;
		if (Right[i] < -32768) Right[i] = -32768;
		
		pSoundBuf[0] += Left[i];
		pSoundBuf[1] += Right[i];
		pSoundBuf += 2;
	}
}

void SegaPCMInit(INT32 clock, INT32 bank, UINT8 *pPCMData, INT32 PCMDataSize)
{
	INT32 Mask, RomMask;
	
	Chip = (struct segapcm*)malloc(sizeof(*Chip));
	memset(Chip, 0, sizeof(*Chip));

	Chip->rom = pPCMData;
	
	Chip->ram = (UINT8*)malloc(0x800);
	memset(Chip->ram, 0xff, 0x800);
	
	Left = (INT32*)malloc(nBurnSoundLen * sizeof(INT32));
	Right = (INT32*)malloc(nBurnSoundLen * sizeof(INT32));

	Chip->bankshift = bank;
	Mask = bank >> 16;
	if(!Mask)
		Mask = BANK_MASK7 >> 16;

	for (RomMask = 1; RomMask < PCMDataSize; RomMask *= 2) {}
	RomMask--;

	Chip->bankmask = Mask & (RomMask >> Chip->bankshift);
	
	double Rate = (double)clock / 128 / nBurnSoundRate;
	Chip->UpdateStep = (INT32)(Rate * 0x10000);
}

void SegaPCMExit()
{
	if (Chip) {
		free(Chip);
		Chip = NULL;
	}
	
	if (Left) {
		free(Left);
		Left = NULL;
	}
	
	if (Right) {
		free(Right);
		Right = NULL;
	}
}

INT32 SegaPCMScan(INT32 nAction,INT32 *pnMin)
{
	struct BurnArea ba;
	char szName[16];
	
	if ((nAction & ACB_DRIVER_DATA) == 0) {
		return 1;
	}
	
	if (pnMin != NULL) {
		*pnMin = 0x029680;
	}
	
	sprintf(szName, "SegaPCM");
	ba.Data		= &Chip;
	ba.nLen		= sizeof(struct segapcm);
	ba.nAddress = 0;
	ba.szName	= szName;
	BurnAcb(&ba);
	
	return 0;
}

UINT8 SegaPCMRead(UINT32 Offset)
{
	return Chip->ram[Offset & 0x07ff];
}

void SegaPCMWrite(UINT32 Offset, UINT8 Data)
{
	Chip->ram[Offset & 0x07ff] = Data;
}
