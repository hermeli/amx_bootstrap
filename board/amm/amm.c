/* ----------------------------------------------------------------------------
 * KABA AG, CC EAC - swyss@kbr.kaba.com
 *
 * File Name           : amm.c
 * Object              :
 * Creation            : Jul 21th 2011
 *-----------------------------------------------------------------------------
 */
#include "../../include/part.h"
#include "../../include/gpio.h"
#include "../../include/pmc.h"
#include "../../include/debug.h"
#include "../../include/sdramc.h"
#include "../../include/main.h"
#ifdef CFG_NORFLASH
#include "../../include/norflash.h"
#endif

static inline unsigned int get_cp15(void)
{
	unsigned int value;
	__asm__("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
	return value;
}

static inline void set_cp15(unsigned int value)
{
	__asm__("mcr p15, 0, %0, c1, c0, 0" : : "r" (value));
}

//	Prozessortyp Erkennung
#define SAM9G20 ((readl(AT91C_DBGU_CIDR)&0x70000)!=0)
#define SAM9260 ((readl(AT91C_DBGU_CIDR)&0x70000)==0)
#define AMM (board == 'M')
#define AML (board == 'L')

#ifdef CFG_HW_INIT
/*----------------------------------------------------------------------------*/
/* \fn    hw_init							      */
/* \brief This function performs very low level HW initialization	      */
/* This function is invoked as soon as possible during the c_startup	      */
/* The bss segment must be initialized					      */
/*----------------------------------------------------------------------------*/
void hw_init(void)
{
	unsigned int cp15, val;
	int board = 0;
	
	/* set OUT3 to red */
	writel(0x00800000, AT91C_BASE_PIOC + PIO_SODR(0));
	
	/* Detect Board type */
	writel(1<<AT91C_ID_PIOC, AT91C_PMC_PCER);	// PIOC clock enable (-> else inputs not functional!)	
	writel(3<<30,AT91C_PIOC_PPUER);				// PIOC30 & 31 pull-up enable
	writel(3<<30,AT91C_PIOC_ODR);				// output disable	

	if (readl(AT91C_PIOC_PDSR)&(1<<31))
	{
		if (readl(AT91C_PIOC_PDSR)&(1<<30))
		{	
			writel(0x0EE00280,AT91C_PIOC_SODR);	// board not supported
			while(1){};
		}
		else
			board = 'L';	// Access Manager LEGIC
	} 
	else
		board = 'M';		// Access Manager MIFARE
		
	/* Configure PIOs */
	const struct pio_desc hw_pio[] = {
#ifdef CFG_DEBUG
		{"RXD", AT91C_PIN_PB(14), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PB(15), 0, PIO_DEFAULT, PIO_PERIPH_A},
#endif
		{"SCK_AVR", AT91C_PIN_PB(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Disable watchdog */
	// writel(AT91C_WDTC_WDDIS, AT91C_BASE_WDTC + WDTC_WDMR);

	/* Main Oscillator Bypass (25MHz) */
	writel(AT91C_CKGR_OSCBYPASS, AT91C_PMC_MOR);
    // while (!(AT91C_PMC_SR & AT91C_PMC_MOSCS));

	if (SAM9260)
	{
		pmc_cfg_plla(PLLA_9260, PLL_LOCK_TIMEOUT);		/* Configure PLLA */
		pmc_cfg_mck(MCKR_9260, PLL_LOCK_TIMEOUT);		/* Configure Master Clock */
		pmc_cfg_mck(MCKR_CSS_9260, PLL_LOCK_TIMEOUT);	/* Switch MCK on PLLA output */
	}
	else // SAM9G20
	{	
		pmc_cfg_plla(PLLA_9G20, PLL_LOCK_TIMEOUT);		/* Configure PLLA */
		writel(AT91C_BASE_PMC+PMC_PLLICPR,0);			/* PLL Charge Pump Current to 0 */
		pmc_cfg_mck(MCKR_9G20, PLL_LOCK_TIMEOUT);		/* Configure Master Clock */
		pmc_cfg_mck(MCKR_CSS_9G20, PLL_LOCK_TIMEOUT);	/* Switch MCK on PLLA output */
	}	
	
	/* Configure PLLB */
	if (AMM)
	{
		if (SAM9260)
			pmc_cfg_pllb(PLLB_AMM_9260, PLL_LOCK_TIMEOUT);
		else // SAM9G20
			pmc_cfg_pllb(PLLB_AMM_9G20, PLL_LOCK_TIMEOUT);
	}
	else // AML
	{
		if (SAM9260)
			pmc_cfg_pllb(PLLB_AML_9260, PLL_LOCK_TIMEOUT);
		else // SAM9G20
			pmc_cfg_pllb(PLLB_AML_9G20, PLL_LOCK_TIMEOUT);
	}

	/* Configure CP15 */
	cp15 = get_cp15();
	cp15 |= I_CACHE;
	set_cp15(cp15);

	/* Configure the PIO controller */
	pio_setup(hw_pio);

	/* Configure the EBI Slave Slot Cycle to 64 */
	// writel( (readl((AT91C_BASE_MATRIX + MATRIX_SCFG3)) & ~0xFF) | 0x40, (AT91C_BASE_MATRIX + MATRIX_SCFG3));

#ifdef CFG_DEBUG
	/* Enable Debug messages on the DBGU */
	dbg_init(BAUDRATE(MASTER_CLOCK, 115200));
	dbg_print("\n\rStart AT91Bootstrap...\n\r");
#endif /* CFG_DEBUG */

	/* Setup Programmable Clocks - PLLB differs for each board/cpu. */  
	
	// Disable programmable clock and interrupt
	writel((1<<8), AT91C_BASE_PMC + PMC_IDR);	
	writel((1<<8), AT91C_BASE_PMC + PMC_SCDR);

	if (AMM)
	{
		// AVR & SECURITY CHIP programmable clock outputs on Port B
		writel(3<<30,AT91C_PIOB_PDR);
		writel(3<<30,AT91C_PIOB_ASR);	// peripheral function A

		if (SAM9260)
		{
			// AVR clock on PCK0/PB30, PLLB=128MHz -> PLLB/8 = 16 MHz
			writel(AT91C_PMC_PRES_CLK_8 | AT91C_PMC_CSS_PLLB_CLK, AT91C_PMC_PCKR);		
			// SC clock on PCK1/PB31 -> PLLB/32 = 4 MHz  	 
			writel(AT91C_PMC_PRES_CLK_32 | AT91C_PMC_CSS_PLLB_CLK,AT91C_PMC_PCKR+4);	
		}
		else // (SAM9G20)
		{
			// AVR clock on PCK0/PB30, PLLB=64MHz -> PLLB/4 = 16 MHz 		
			writel(AT91C_PMC_PRES_CLK_4 | AT91C_PMC_CSS_PLLB_CLK, AT91C_PMC_PCKR);
			// SC clock on PCK1/PB31 -> PLLB/16 = 4MHz 				
			writel(AT91C_PMC_PRES_CLK_16 | AT91C_PMC_CSS_PLLB_CLK, AT91C_PMC_PCKR+4);	
		}
		writel(AT91C_PMC_PCK0|AT91C_PMC_PCK1,AT91C_PMC_SCER);	
		
		// Setup Programmable Clock register
		writel((3 << 2) | 3, AT91C_BASE_PMC+PMC_PCKR); 	// clock div=8, select PLLB
		
		// Enable programmable clock
		writel(1<<8, AT91C_BASE_PMC + PMC_SCER); // PCK0 output enable

	}
	else // (AML)
	{	
		writel(3<<30,AT91C_PIOB_ODR);	// disable output of AVR & SECURITY CHIP on PB30/PB31 
		writel(3<<30,AT91C_PIOB_PER);	// enable PIO controller on PB30/PB31

		// enable clock on PB19 (with timer TC5 -> derived from MCK!)
		writel(1<<19,AT91C_PIOB_OER);	// output enable	
		writel(1<<19,AT91C_PIOB_BSR);	// function B
		writel(1<<19,AT91C_PIOB_PDR);	// PIO disable
		writel(1<<19,AT91C_PIOB_PPUDR);	// pull-up disable
		writel((1<<AT91C_ID_TC5)|(1<<AT91C_ID_PIOB),AT91C_PMC_PCER);	
		
		// TC5 Clock Eingang abschalten
		writel( (readl(AT91C_TCB1_BMR) & ~AT91C_TCB_TC2XC2S) | AT91C_TCB_TC2XC2S_NONE, AT91C_TCB1_BMR);
		
		// TC5 Timer5 setzen, TIMER_CLOCK1 = MCK / 2 = 45 MHz
		writel(AT91C_TC_CLKDIS,AT91C_TC5_CCR);
		writel(AT91C_TC_CLKS_TIMER_DIV1_CLOCK | AT91C_TC_WAVE | AT91C_TC_WAVESEL_UP_AUTO | \
				AT91C_TC_EEVT_XC0 | AT91C_TC_BCPB_SET | AT91C_TC_BCPC_CLEAR,AT91C_TC5_CMR);
	
		writel(2,AT91C_TC5_RB);		// TIOB5-Hi bei 1
		writel(3,AT91C_TC5_RC);		// TIOB5-Lo und Reset bei 2 (Tcyc = 66.66ns = 15MHz)
		writel(0,AT91C_TC5_CV);		// Counter Start bei 0
		writel(0,AT91C_TC5_SR);		// Alle Statusflags zurücksetzen
		writel(0xFFFFFFFF,AT91C_TC5_IDR);	// alle TC5 Interrupts abschalten
		writel(AT91C_TC_CLKEN | AT91C_TC_SWTRG,AT91C_TC5_CCR); 	// Timer 5 starten
	}
	
	/* set OUT1-3 to off, STATE to orange */
	writel(0x0EE00280, AT91C_BASE_PIOC + PIO_CODR(0));
	writel(0x00000280, AT91C_BASE_PIOC + PIO_SODR(0));
	
#ifdef CFG_SDRAM
	/* Initialize the matrix */
	// register must be OR-ed, because of 3.3V bit that must stay untouched
	writel(readl(AT91C_CCFG_EBICSA)|AT91C_EBI_CS1A_SDRAMC,AT91C_CCFG_EBICSA);
	
	/* Configure SDRAM Controller */
	sdram_init(	AT91C_SDRAMC_NC_10  |
				AT91C_SDRAMC_NR_13 |	
				AT91C_SDRAMC_NB_4_BANKS |
				AT91C_SDRAMC_CAS_2 |
				AT91C_SDRAMC_DBW_16_BITS |
				AT91C_SDRAMC_TWR_2 |
				AT91C_SDRAMC_TRC_7 |
				AT91C_SDRAMC_TRP_2 |
				AT91C_SDRAMC_TRCD_2 |
				AT91C_SDRAMC_TRAS_5 |
				AT91C_SDRAMC_TXSR_8,		/* Control Register */
				((MASTER_CLOCK/1000000)*7778)/1000,	/* Refresh Timer Register */
				AT91C_SDRAMC_MD_SDRAM);		/* SDRAM (no low power)   */ 
	
#endif /* CFG_SDRAM */
}
#endif /* CFG_HW_INIT */

#ifdef CFG_SDRAM
/*------------------------------------------------------------------------------*/
/* \fn    sdramc_hw_init							*/
/* \brief This function performs SDRAMC HW initialization			*/
/*------------------------------------------------------------------------------*/
void sdramc_hw_init(void)
{
	/* Configure PIOs */
/*	const struct pio_desc sdramc_pio[] = {
		{"D16", AT91C_PIN_PC(16), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D17", AT91C_PIN_PC(17), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D18", AT91C_PIN_PC(18), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D19", AT91C_PIN_PC(19), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D20", AT91C_PIN_PC(20), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D21", AT91C_PIN_PC(21), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D22", AT91C_PIN_PC(22), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D23", AT91C_PIN_PC(23), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D24", AT91C_PIN_PC(24), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D25", AT91C_PIN_PC(25), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D26", AT91C_PIN_PC(26), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D27", AT91C_PIN_PC(27), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D28", AT91C_PIN_PC(28), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D29", AT91C_PIN_PC(29), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D30", AT91C_PIN_PC(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D31", AT91C_PIN_PC(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};
*/
	/* Configure the SDRAMC PIO controller to output PCK0 */
/*	pio_setup(sdramc_pio); */

	// writel(0xFFFF0000, AT91C_BASE_PIOC + PIO_ASR(0));
	// writel(0xFFFF0000, AT91C_BASE_PIOC + PIO_PDR(0));

}
#endif /* CFG_SDRAM */

#ifdef CFG_DATAFLASH

/*------------------------------------------------------------------------------*/
/* \fn    df_recovery								*/
/* \brief This function erases DataFlash Page 0 if BP4 is pressed 		*/
/*        during boot sequence							*/
/*------------------------------------------------------------------------------*/
void df_recovery(AT91PS_DF pDf)
{
#if (AT91C_SPI_PCS_DATAFLASH == AT91C_SPI_PCS1_DATAFLASH)
	/* Configure PIOs */
	const struct pio_desc bp4_pio[] = {
		{"BP4", AT91C_PIN_PA(31), 0, PIO_PULLUP, PIO_INPUT},
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	writel((1 << AT91C_ID_PIOA), PMC_PCER + AT91C_BASE_PMC);
	pio_setup(bp4_pio);
	
	/* If BP4 is pressed during Boot sequence */
	/* Erase NandFlash block 0*/
	if ( !pio_get_value(AT91C_PIN_PA(31)) )
		df_page_erase(pDf, 0);
#endif
}

/*------------------------------------------------------------------------------*/
/* \fn    df_hw_init								*/
/* \brief This function performs DataFlash HW initialization			*/
/*------------------------------------------------------------------------------*/
void df_hw_init(void)
{
	/* Configure PIOs */
	const struct pio_desc df_pio[] = {
		{"MISO",  AT91C_PIN_PA(0), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MOSI",  AT91C_PIN_PA(1), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SPCK",  AT91C_PIN_PA(2), 0, PIO_DEFAULT, PIO_PERIPH_A},
#if (AT91C_SPI_PCS_DATAFLASH == AT91C_SPI_PCS0_DATAFLASH)
		{"NPCS0", AT91C_PIN_PA(3), 0, PIO_DEFAULT, PIO_PERIPH_A},
#endif
#if (AT91C_SPI_PCS_DATAFLASH == AT91C_SPI_PCS1_DATAFLASH)
		{"NPCS1", AT91C_PIN_PC(11), 0, PIO_DEFAULT, PIO_PERIPH_B},
#endif
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pio_setup(df_pio);
}
#endif /* CFG_DATAFLASH */


#ifdef CFG_NORFLASH
/*------------------------------------------------------------------------------*/
/* \fn    norflash_hw_init							*/
/* \brief NandFlash HW init							*/
/*------------------------------------------------------------------------------*/
void norflash_hw_init(void)
{
	/* Configure SMC CS0 */
 	writel((AT91C_SM_NWE_SETUP | AT91C_SM_NCS_WR_SETUP | AT91C_SM_NRD_SETUP | AT91C_SM_NCS_RD_SETUP), AT91C_BASE_SMC + SMC_SETUP0);
  	writel((AT91C_SM_NWE_PULSE | AT91C_SM_NCS_WR_PULSE | AT91C_SM_NRD_PULSE | AT91C_SM_NCS_RD_PULSE), AT91C_BASE_SMC + SMC_PULSE0);
	writel((AT91C_SM_NWE_CYCLE | AT91C_SM_NRD_CYCLE), AT91C_BASE_SMC + SMC_CYCLE0);
	
	// *** Mode Register ***
	// PS				Page Size					2xxxxxxx = 16 byte page
	// PMEN				Page Mode Enable			x1xxxxxx = async. burst mode
	// TDF_MODE			TDF optimization			xx0xxxxx = disabled
	// TDF_CYCLES		data release time after RD	xxx2xxxx = 2+1 cycles = 33.3ns
	// DBW				bus bandwidth				xxxx1xxx = 16 bit
	// BAT				byte access type			xxxxx1xx = byte write access type (with WR)
	// EXNW_MODE		nwait mode					xxxxxx0x = disabled
	// WRITE_MODE		write mode					xxxxxxx2 = write controlled by WR (not CS)
	// READ_MODE		read mode					xxxxxxx1 = read controlled by RD (not CS)
	writel(0x21021103, AT91C_BASE_SMC + SMC_CTRL0);

	/* Configure the PIO controller */
	writel((1 << AT91C_ID_PIOC), PMC_PCER + AT91C_BASE_PMC);
}

/*------------------------------------------------------------------------------*/
/* \fn    norflash_cfg_16bits_dbw_init						*/
/* \brief Configure SMC in 16 bits mode						*/
/*------------------------------------------------------------------------------*/
void norflash_cfg_16bits_dbw_init(void)
{
	writel(readl(AT91C_BASE_SMC + SMC_CTRL0) | AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS, AT91C_BASE_SMC + SMC_CTRL0);
}

/*------------------------------------------------------------------------------*/
/* \fn    norflash_cfg_8bits_dbw_init						*/
/* \brief Configure SMC in 8 bits mode						*/
/*------------------------------------------------------------------------------*/
void norflash_cfg_8bits_dbw_init(void)
{
	writel((readl(AT91C_BASE_SMC + SMC_CTRL0) & ~(AT91C_SMC_DBW)) | AT91C_SMC_DBW_WIDTH_EIGTH_BITS, AT91C_BASE_SMC + SMC_CTRL0);
}


#endif /* #ifdef CFG_NORFLASH */
