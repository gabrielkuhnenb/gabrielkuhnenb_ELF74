//*****************************************************************************
//
// blinky.c - Simple example to blink the on-board LED.
//
// Copyright (c) 2013-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.2.0.295 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <iostream>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Blinky (blinky)</h1>
//!
//! A very simple example that blinks the on-board LED using direct register
//! access.
//
//*****************************************************************************

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1);
}
#endif

//*****************************************************************************
//
// Blink the on-board LED.
//
//*****************************************************************************

//*****************************************************************************
//
// Declaracoes auxiliares do lab2
//

/**
 * contador de interrupcoes
 */
volatile uint32_t interrupt_counter = 0;

void SysTickIntHandler(void)
{
    interrupt_counter++;
}
//*****************************************************************************

int
main(void)
{
    volatile uint32_t cycle_counter;
    volatile uint32_t pin_read;

    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    /**
     * mudanca de pino 0 para pino 1
     */
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    
    /**
     * configurar sw1 portj como entrada e conectar resistor de pullup
     */
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,
                     GPIO_PIN_0,
                     GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
    
    /**
     * interrupcao
     */
    IntMasterEnable();
    SysTickIntRegister(SysTickIntHandler);  
    SysTickPeriodSet(10000000);  
    SysTickIntEnable();
    SysTickEnable();
    
    interrupt_counter = 0;
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);
    
    while (interrupt_counter < 12)
    {
        // espera 12 interrupcoes (1 s) para acender o led
    }
    
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
    
    pin_read = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0);
    
    cycle_counter = SysTickValueGet();
    
    interrupt_counter = 0;
    
    /**
     * enquanto o botao nao for pressionado, ate 36 interrupcoes (3 s),
     * le o pino
     */
    while (pin_read == 1 && interrupt_counter < 36)
    {
        pin_read = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0);
    }

    /**
     * quantidade atual do systick + (quantidade de interrupcoes * periodo)
     */
    cycle_counter = (SysTickValueGet() - cycle_counter) + 
      (10000000 * interrupt_counter);
    
    printf("%d ciclos", cycle_counter);
}
