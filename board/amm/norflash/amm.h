/* ----------------------------------------------------------------------------
 * KABA AG, CC EAC - swyss@kbr.kaba.com
 *
 * File Name           : amm.h
 * Object              :
 * Creation            : Jul 21th 2011
 *-----------------------------------------------------------------------------
 */
#ifndef _AMM_H
#define _AMM_H

/* ******************************************************************* */
/* PMC Settings                                                        */
/*                                                                     */
/* The main oscillator is enabled as soon as possible in the c_startup */
/* and MCK is switched on the main oscillator.                         */
/* PLL initialization is done later in the hw_init() function          */
/* ******************************************************************* */
#define MASTER_CLOCK		(180000000/2)
#define PLL_LOCK_TIMEOUT	1000000

// 9260: PLLA min. 150 MHz max. 240 MHz -> Field OUT of CKGR_PLL is 10 
#define PLLA_9260       ((2<<28)|( 35<<16)|(2<<14)|(63<<8)|(5))  // 180 MHz = 25 MHz * ( 35+1)/5 

// 9G20: PLLA min. 695 MHz max. 750 MHz -> ICPLLA=0 OUT=01
#define PLLA_9G20       ((2<<28)|(143<<16)|(1<<14)|(63<<8)|(5))  // 720 MHz = 25 MHz * (143+1)/5 

// 9260: PLLB min. 70 MHz max. 130 MHz -> Field OUT of CKGR_PLL is 01 
// 9G20: PLLB min. 30 MHz max. 100 MHz -> Field OUT of CKGR_PLL is 00
#define PLLB_AMM_9260   ((1<18)|( 127<<16)|(1<<14)|(63<<8)|(25)) // 128 MHz = 25MHz * (127+1)/25 
#define PLLB_AMM_9G20   ((1<18)|(  63<<16)|(0<<14)|(63<<8)|(25)) // 64 MHz = 25MHz * (127+1)/25 
#define PLLB_AML_9260	((1<18)|(  95<<16)|(1<<14)|(63<<8)|(25)) // 96 MHz = 25MHz * (95+1)/25 
#define PLLB_AML_9G20	((1<18)|(  22<<16)|(0<<14)|(63<<8)|(06)) // 95.833 MHz = 25MHz * (22+1)/6 

// 9260: PCK = PLLA = 180MHz, MCK = PCK : 2 = 90 MHz 
#define MCKR_9260		(AT91C_PMC_PRES_CLK | AT91C_PMC_MDIV_2)
#define MCKR_CSS_9260	(AT91C_PMC_CSS_PLLA_CLK | MCKR_9260)

// 9G20: PCK = PLLA:4 = 180 MHz, MCK = PCK:2 = 90 MHz
#define MCKR_9G20		(AT91C_PMC_PRES_CLK_4 | AT91C_PMC_MDIV_2)
#define MCKR_CSS_9G20	(AT91C_PMC_CSS_PLLA_CLK | MCKR_9G20)

/* ******************************************************************* */
/* NandFlash Settings                                                  */
/*                                                                     */
/* ******************************************************************* */
#define AT91C_SMARTMEDIA_BASE	0x40000000

#define AT91_SMART_MEDIA_ALE    (1 << 21)	/* our ALE is AD21 */
#define AT91_SMART_MEDIA_CLE    (1 << 22)	/* our CLE is AD22 */

#define NAND_DISABLE_CE() do { *(volatile unsigned int *)AT91C_PIOC_SODR = AT91C_PIO_PC14;} while(0)
#define NAND_ENABLE_CE() do { *(volatile unsigned int *)AT91C_PIOC_CODR = AT91C_PIO_PC14;} while(0)

#define NAND_WAIT_READY() while (!(*(volatile unsigned int *)AT91C_PIOC_PDSR & AT91C_PIO_PC13))


/* ******************************************************************** */
/* SMC Chip Select Timings for NorFlash for MASTER_CLOCK = 90'000'000.*/
/* Please refer to SMC section in AT91SAM datasheet to learn how 	*/
/* to generate these values. 						*/
/* ******************************************************************** */
#define AT91C_SM_NWE_SETUP	(2 << 0)
#define AT91C_SM_NCS_WR_SETUP	(1 << 8)
#define AT91C_SM_NRD_SETUP	(3 << 16)
#define AT91C_SM_NCS_RD_SETUP	(2 << 24)
  
#define AT91C_SM_NWE_PULSE 	(7 << 0)
#define AT91C_SM_NCS_WR_PULSE	(10 << 8)
#define AT91C_SM_NRD_PULSE	(11 << 16)
#define AT91C_SM_NCS_RD_PULSE	(13 << 24)
  
#define AT91C_SM_NWE_CYCLE 	(13 << 0)
#define AT91C_SM_NRD_CYCLE	(17 << 16)
#define AT91C_SM_TDF	        (2 << 16)

/* ******************************************************************* */
/* BootStrap Settings                                                  */
/*                                                                     */
/* ******************************************************************* */
#define IMG_ADDRESS 		0x10020000		/* Image Address in NorFlash */
#define	IMG_SIZE		0x40000		/* Image Size in NorFlash    */

#define MACH_TYPE       	0x44B		/* AT91SAM9260-EK */
#define JUMP_ADDR		0x23F00000	/* Final Jump Address 	      */

/* ******************************************************************* */
/* Application Settings                                                */
/* ******************************************************************* */
#define CFG_DEBUG
#undef CFG_DATAFLASH

#define CFG_NORFLASH
#undef	NANDFLASH_SMALL_BLOCKS	/* NANDFLASH_LARGE_BLOCKS used instead */

#define CFG_HW_INIT
#define CFG_SDRAM

#endif	/* _AMM_H */
