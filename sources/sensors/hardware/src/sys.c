#include "sys.h"

/* THUMB instruction set only — implement WFI/interrupt control via inline assembly */

void WFI_SET(void)
{
	__ASM volatile("wfi");
}

void INTX_DISABLE(void)
{
	__ASM volatile("cpsid i");
}

void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");
}

/* Set Main Stack Pointer */
__asm void MSR_MSP(u32 addr)
{
	MSR MSP, r0
	BX  r14
}
