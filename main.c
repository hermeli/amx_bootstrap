/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : main.c
 * Object              :
 * Creation            : ODi Apr 19th 2006
 *-----------------------------------------------------------------------------
 */
#include "include/part.h"
#include "include/main.h"
#include "include/debug.h"
#include "include/dataflash.h"
#include "include/nandflash.h"
#include "include/norflash.h"
#include <stdio.h>

/*------------------------------------------------------------------------------*/
/* Function Name       : PrintHex							*/
/* Input Parameters    : hex data, count							*/
/* Output Parameters   : none							*/
/*------------------------------------------------------------------------------*/
void PrintHex(unsigned int data, int cnt)
{
	int m;
	char cbuf[9];

	for(m=cnt-1; m>=0; m--)
	{		
		cbuf[m]=(data&15)>9?(data&15)+'A'-10:(data&15)+'0';
		data>>=4;		
	}
	cbuf[cnt]='\0';
	dbg_print(cbuf);
}

/*------------------------------------------------------------------------------*/
/* Function Name       : main							*/
/* Object              : Main function						*/
/* Input Parameters    : none							*/
/* Output Parameters   : True							*/
/*------------------------------------------------------------------------------*/
int main(void)
{
	unsigned int data;
/* ================== 1st step: Hardware Initialization ================= */
	/* Performs the hardware initialization */
#ifdef CFG_HW_INIT
	hw_init();
#endif

/* ==================== 2nd step: Load from media ==================== */
	/* Load from Dataflash in RAM */
#ifdef CFG_DATAFLASH
	load_df(AT91C_SPI_PCS_DATAFLASH, IMG_ADDRESS, IMG_SIZE, JUMP_ADDR);
#endif

	/* Load from Nandflash in RAM */
#ifdef CFG_NANDFLASH
	load_nandflash(IMG_ADDRESS, IMG_SIZE, JUMP_ADDR);
#endif

	/* Load from Norflash in RAM */
#ifdef CFG_NORFLASH
	norflash_hw_init();
#ifdef CFG_DEBUG	
	dbg_print("Checking data at flash address 0x");
	PrintHex(IMG_ADDRESS,8);
	
	data = *(unsigned int*)IMG_ADDRESS;
	
	dbg_print(" (first word = 0x");
	PrintHex(data,8);
	dbg_print(")\n\r");
	
	if ( (data & 0xFF000000) == 0xEA000000 )
	{
		dbg_print("Found valid jump instruction (0xEA..)\n\r");
		dbg_print("Loading image into SDRAM...\n\r");
	} else {
		dbg_print("Error: data is not a valid jump instruction (0xEA.. expected)!\n\r");
		while(1);
	}
	
#endif
	load_norflash(IMG_ADDRESS, IMG_SIZE, JUMP_ADDR);
#endif
/* ==================== 3rd step:  Process the Image =================== */
	/* Uncompress the image */
#ifdef GUNZIP
	decompress_image((void *)IMG_ADDRESS, (void *)JUMP_ADDR, IMG_SIZE);	/* NOT IMPLEMENTED YET */
#endif /* GUNZIP */

/* ==================== 4th step: Start the application =================== */
	/* Set linux arguments */
#ifdef LINUX_ARG
	linux_arg(LINUX_ARG);	/* NOT IMPLEMENTED YET */
#endif /* LINUX_ARG */

	/* Jump to the Image Address */
#ifdef CFG_DEBUG
	dbg_print("Jumping to image at 0x");
	PrintHex(JUMP_ADDR,8);
	dbg_print(" (first word = 0x");
	PrintHex(*(unsigned int*)JUMP_ADDR,8);
	dbg_print(")...\n\r");
#endif
	return JUMP_ADDR;
}

