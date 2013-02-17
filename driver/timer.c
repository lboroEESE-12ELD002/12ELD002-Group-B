// *************************************************************************************************
//
//	Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
//	 
//	 
//	  Redistribution and use in source and binary forms, with or without 
//	  modification, are permitted provided that the following conditions 
//	  are met:
//	
//	    Redistributions of source code must retain the above copyright 
//	    notice, this list of conditions and the following disclaimer.
//	 
//	    Redistributions in binary form must reproduce the above copyright
//	    notice, this list of conditions and the following disclaimer in the 
//	    documentation and/or other materials provided with the   
//	    distribution.
//	 
//	    Neither the name of Texas Instruments Incorporated nor the names of
//	    its contributors may be used to endorse or promote products derived
//	    from this software without specific prior written permission.
//	
//	  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//	  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//	  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//	  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//	  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//	  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//	  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//	  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//	  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//	  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//	  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *************************************************************************************************
// Timer service routines.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "timer.h"
#include "ports.h"
#include "buzzer.h"
#include "vti_ps.h"
#include "vti_as.h"
#include "display.h"

// logic
#include "clock.h"
#include "battery.h"
#include "stopwatch.h"
#include "alarm.h"
#include "altitude.h"
#include "display.h"
#include "rfsimpliciti.h"
#include "simpliciti.h"
#include "acceleration.h"
#include "bluerobin.h"
#include "temperature.h"
#include "countdown.h"
#include "countup.h"


// *************************************************************************************************
// Prototypes section
void Timer0_Init(void);
void Timer0_Stop(void);
void Timer0_A1_Start(void);
void Timer0_A1_Stop(void);
void Timer0_A3_Start(u16 ticks);
void Timer0_A3_Stop(void);
void Timer0_A4_Delay(u16 ticks);
void (*fptr_Timer0_A3_function)(void);
 

// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct timer sTimer;

// *************************************************************************************************
// Extern section
extern void BRRX_TimerTask_v(void);
extern void to_lpm(void);


// *************************************************************************************************
// @fn          Timer0_Init
// @brief       Set Timer0 to a period of 1 or 2 sec. IRQ TACCR0 is asserted when timer overflows.
// @param       none
// @return      none
// *************************************************************************************************
void Timer0_Init(void)
{
	// Set interrupt frequency to 1Hz
	TA0CCR0   = 32768 - 1;                

	// Enable timer interrupt    
	TA0CCTL0 |= CCIE;                     

	// Clear and start timer now   
	// Continuous mode: Count to 0xFFFF and restart from 0 again - 1sec timing will be generated by ISR
	TA0CTL   |= TASSEL0 + MC1 + TACLR;                       
}


// *************************************************************************************************
// @fn          Timer0_Start
// @brief       Start Timer0.
// @param       none
// @return      none
// *************************************************************************************************
void Timer0_Start(void)
{ 
	// Start Timer0 in continuous mode	 
	TA0CTL |= MC_2;          
}


// *************************************************************************************************
// @fn          Timer0_Stop
// @brief       Stop and reset Timer0.
// @param       none
// @return      none
// *************************************************************************************************
void Timer0_Stop(void)
{ 
	// Stop Timer0	 
	TA0CTL &= ~MC_2;

	// Set Timer0 count register to 0x0000
	TA0R = 0;                             
}


// *************************************************************************************************
// @fn          Timer0_A3_Start
// @brief       Trigger IRQ every "ticks" microseconds
// @param       ticks (1 tick = 1/32768 sec)
// @return      none
// *************************************************************************************************
void Timer0_A3_Start(u16 ticks)
{
	u16 value;
	
	// Store timer ticks in global variable
	sTimer.timer0_A3_ticks = ticks;
	
	// Delay based on current counter value
	value = TA0R + ticks;
	
	// Update CCR
	TA0CCR3 = value;   

	// Reset IRQ flag    
	TA0CCTL3 &= ~CCIFG; 
	          
	// Enable timer interrupt    
	TA0CCTL3 |= CCIE; 
}



// *************************************************************************************************
// @fn          Timer0_A3_Stop
// @brief       Stop Timer0_A3.
// @param       none
// @return      none
// *************************************************************************************************
void Timer0_A3_Stop(void)
{
	// Clear timer interrupt    
	TA0CCTL3 &= ~CCIE; 
}


// *************************************************************************************************
// @fn          Timer0_A4_Delay
// @brief       Wait for some microseconds
// @param       ticks (1 tick = 1/32768 sec)
// @return      none
// *************************************************************************************************
void Timer0_A4_Delay(u16 ticks)
{
	u16 value;
	
	// Exit immediately if Timer0 not running - otherwise we'll get stuck here
	if ((TA0CTL & (BIT4 | BIT5)) == 0) return;    

	// Disable timer interrupt    
	TA0CCTL4 &= ~CCIE; 	

	// Clear delay_over flag
	sys.flag.delay_over = 0;
	
	// Add delay to current timer value
	value = TA0R + ticks;
	
	// Update CCR
	TA0CCR4 = value;   

	// Reset IRQ flag    
	TA0CCTL4 &= ~CCIFG; 
	          
	// Enable timer interrupt    
	TA0CCTL4 |= CCIE; 
	
	// Wait for timer IRQ
	while (1)
	{
		// Delay in LPM
		to_lpm();

#ifdef USE_WATCHDOG		
		// Service watchdog
		WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
#endif
		// Redraw stopwatch display
		if (is_stopwatch()) display_stopwatch(LINE2, DISPLAY_LINE_UPDATE_PARTIAL);

		// Redraw countdown display
		if (is_countdown_active()) display_countdown(LINE1, DISPLAY_LINE_UPDATE_PARTIAL);

		// Redraw countup display
		if (is_countup_active()) display_countup(LINE2, DISPLAY_LINE_UPDATE_PARTIAL);

		// Check stop condition
		if (sys.flag.delay_over) break;
	}
}


// *************************************************************************************************
// @fn          TIMER0_A0_ISR
// @brief       IRQ handler for TIMER0_A0 IRQ
//				Timer0_A0	1/1sec clock tick 			(serviced by function TIMER0_A0_ISR)
//				Timer0_A1	 							(serviced by function TIMER0_A1_5_ISR)
//				Timer0_A2	1/100 sec Stopwatch			(serviced by function TIMER0_A1_5_ISR)
//				Timer0_A3	Configurable periodic IRQ	(serviced by function TIMER0_A1_5_ISR)
//				Timer0_A4	One-time delay				(serviced by function TIMER0_A1_5_ISR)
// @param       none
// @return      none
// *************************************************************************************************
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
	static u8 button_lock_counter = 0;
	
	// Disable IE 
	TA0CCTL0 &= ~CCIE;
	// Reset IRQ flag  
	TA0CCTL0 &= ~CCIFG;  
	// Add 1 sec to TACCR0 register (IRQ will be asserted at 0x7FFF and 0xFFFF = 1 sec intervals)
	TA0CCR0 += 32768;
	// Enable IE 
	TA0CCTL0 |= CCIE;
	
	// Add 1 second to global time
	clock_tick();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Count down and up
	if (sCountup.mode == COUNTUP_MODE_ON)
	{
		countup_tick();

		if (sCountdown.mode == COUNTDOWN_MODE_ON)
		{
			// Countdown acceleration measurement timeout
			sAccel.timeout--;

			// Stop measurement when timeout has elapsed
			if (sAccel.timeout == 0) as_stop();

			// If DRDY is (still) high, request data again
			if ((AS_INT_IN & AS_INT_PIN) == AS_INT_PIN) request.flag.acceleration_measurement = 1;

			// sAccel.xyz[0] = xaxis, sAccel.xyz[0] = yaxis, sAccel.xyz[0] = zaxis

			if ((sAccel.xyz[0] < sCountdown.thresholdlow[0] || sAccel.xyz[0] > sCountdown.thresholdhigh[0]) &&
				(sAccel.xyz[1] < sCountdown.thresholdlow[1] || sAccel.xyz[1] > sCountdown.thresholdhigh[1]))
			{
			// UNDER THRESHOLD VALUES
				if (sCountdown.speed == 1)
				{
					// Clear timer interrupt enable
					TA0CCTL2 &= ~CCIE;

					countdown_tick();
				}
				if (sCountdown.speed == 2)
				{
					// Recovery loop - set back to default as below threshold
					if (sCountdown.exceedcount[0] != sCountdown.exceeddefault[0])
					{
						sCountdown.exceedcount[0] = sCountdown.exceeddefault[0];
					}

					// Recovery loop - reduce recovery value if under threshold
					if (sCountdown.recoverycount != 0)
					{
						sCountdown.recoverycount--;
					}
					// Set speed to 1 if recovery is back to 0
					else
					{
						sCountdown.speed = 1;
						sCountdown.recoverycount = sCountdown.recoverydefault;
						countdown_tick();
					}
				}
				if (sCountdown.speed == 3)
				{
					// Recovery loop - set back to default as below threshold
					if (sCountdown.exceedcount[1] != sCountdown.exceeddefault[1])
					{
						sCountdown.exceedcount[1] = sCountdown.exceeddefault[1];
					}

					// Recovery loop - reduce recovery value if under threshold
					if (sCountdown.recoverycount != 0)
					{
						sCountdown.recoverycount--;
					}
					// Set speed to 1 if recovery is back to 0
					else
					{
						sCountdown.speed = 1;
						sCountdown.recoverycount = sCountdown.recoverydefault;
						countdown_tick();
					}
				}
			}
			// OVER THRESHOLD VALUES
			else
			{

				sCountdown.recoverycount = sCountdown.recoverydefault;

				if (sCountdown.speed == 1)
				{
					// Increase countdown speed
					sCountdown.speed = 2;

					// Init CCR register with current time
					TA0CCR2 = TA0R;

					// Load CCR register with next capture time
					TA0CCR2 += sCountdown.tickspeed[0];

					// Reset IRQ flag
					TA0CCTL2 &= ~CCIFG;

					// Enable timer interrupt
					TA0CCTL2 |= CCIE;

					countdown_tick();
				}
				if (sCountdown.speed == 2)
				{
					// Counting down from exceed2 limit
					if (sCountdown.exceedcount[0] != 0)
					{
						sCountdown.exceedcount[0]--;
					}
					// Speed up if equal to 0
					else
					{
						sCountdown.exceedcount[0] = sCountdown.exceeddefault[0];

						// Increase countdown speed
						sCountdown.speed = 3;
					}
				}
				if (sCountdown.speed == 3)
				{
					// Counting down from exceed3 limit
					if (sCountdown.exceedcount[1] != 0)
					{
						sCountdown.exceedcount[1]--;
					}
					// Speed up if equal to 0
					else
					{
						countdown_droptozero();
					}
				}
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Set clock update flag
	display.flag.update_time = 1;
	
	// While SimpliciTI stack operates or BlueRobin searches, freeze system state
	if (is_rf() || is_bluerobin_searching()) 
	{
		// SimpliciTI automatic timeout
		if (sRFsmpl.timeout == 0) 
		{
			simpliciti_flag |= SIMPLICITI_TRIGGER_STOP;
		}
		else
		{
			sRFsmpl.timeout--;
		}
		
		// Exit from LPM3 on RETI
		_BIC_SR_IRQ(LPM3_bits);     
		return;
	}
	
	// -------------------------------------------------------------------
	// Service modules that require 1/min processing
	if (sTime.drawFlag >= 2) 
	{
		// Measure battery voltage to keep track of remaining battery life
		request.flag.voltage_measurement = 1;
		
		// Check if alarm needs to be turned on
		check_alarm();
	}

	// -------------------------------------------------------------------
	// Service active modules that require 1/s processing
	
	// Generate alarm signal
	if (sAlarm.state == ALARM_ON) 
	{
		// Decrement alarm duration counter
		if (sAlarm.duration-- > 0)
		{
			request.flag.buzzer = 1;
		}
		else
		{
			sAlarm.duration = ALARM_ON_DURATION;
			stop_alarm();
		}
	}

	// Do a temperature measurement each second while menu item is active
	if (is_temp_measurement()) request.flag.temperature_measurement = 1;
	
	// Do a pressure measurement each second while menu item is active
	if (is_altitude_measurement()) 
	{
		// Countdown altitude measurement timeout while menu item is active
		sAlt.timeout--;

		// Stop measurement when timeout has elapsed
		if (sAlt.timeout == 0)	
		{
			stop_altitude_measurement();
			// Show ---- m/ft
			display_chars(LCD_SEG_L1_3_0, (u8*)"----", SEG_ON);
			// Clear up/down arrow
			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
		}
		
		// In case we missed the IRQ due to debouncing, get data now
		if ((PS_INT_IN & PS_INT_PIN) == PS_INT_PIN) request.flag.altitude_measurement = 1;
	}	

	// Count down timeout
	if (is_acceleration_measurement()) 
	{
		// Countdown acceleration measurement timeout 
		sAccel.timeout--;

		// Stop measurement when timeout has elapsed
		if (sAccel.timeout == 0) as_stop();	
		
		// If DRDY is (still) high, request data again
		if ((AS_INT_IN & AS_INT_PIN) == AS_INT_PIN) request.flag.acceleration_measurement = 1; 
	}	

	// If BlueRobin transmitter is connected, get data from API
	if (is_bluerobin()) get_bluerobin_data();
	
	// If battery is low, decrement display counter
	if (sys.flag.low_battery)
	{
		if (sBatt.lobatt_display-- == 0) 
		{
			message.flag.prepare = 1;
			message.flag.type_lobatt = 1;
			sBatt.lobatt_display = BATTERY_LOW_MESSAGE_CYCLE;
		}
	}
	
	// If a message has to be displayed, set display flag
	if (message.all_flags)
	{
		if (message.flag.prepare)
		{
			message.flag.prepare = 0;
			message.flag.show    = 1;
		}
		else if (message.flag.erase) // message cycle is over, so erase it
		{
			message.flag.erase       = 0;
			display.flag.full_update = 1;
		}	
	}
	
	// -------------------------------------------------------------------
	// Check idle timeout, set timeout flag
	if (sys.flag.idle_timeout_enabled)
	{
		if (sTime.system_time - sTime.last_activity > INACTIVITY_TIME) sys.flag.idle_timeout = 1; //setFlag(sysFlag_g, SYS_TIMEOUT_IDLE);
	}
	
	// -------------------------------------------------------------------
	// Detect continuous button high states
	
	// Trying to lock/unlock buttons?
	if (BUTTON_NUM_IS_PRESSED && BUTTON_DOWN_IS_PRESSED)
	{
		if (button_lock_counter++ > LEFT_BUTTON_LONG_TIME)
		{
			// Toggle lock / unlock buttons flag
			sys.flag.lock_buttons = ~sys.flag.lock_buttons;
	
			// Show "buttons are locked/unlocked" message synchronously with next second tick
			message.flag.prepare = 1;
			if (sys.flag.lock_buttons)	message.flag.type_locked   = 1;
			else						message.flag.type_unlocked = 1;
			
			// Reset button lock counter
			button_lock_counter = 0;
		}
	}
	else // Trying to create a long button press?
	{
		// Reset button lock counter
		button_lock_counter = 0;
		
		if (BUTTON_UP_IS_PRESSED)
		{
			sButton.up_timeout++;
			
			// Check if button was held low for some seconds
			if (sButton.up_timeout > LEFT_BUTTON_LONG_TIME)
			{
				button.flag.up_long = 1;
				sButton.up_timeout = 0;
			}
		}
		else
		{
			sButton.up_timeout = 0;
		}
	
		if (BUTTON_DOWN_IS_PRESSED)
		{
			sButton.down_timeout++;
		
			// Check if button was held low for some seconds
			if (sButton.down_timeout > LEFT_BUTTON_LONG_TIME)
			{
				button.flag.down_long = 1;
				sButton.down_timeout = 0;
			}
		}
		else
		{
			sButton.down_timeout = 0;
		}

		if (BUTTON_NUM_IS_PRESSED)
		{
			sButton.num_timeout++;

			// Check if button was held low for some seconds
			if (sButton.num_timeout > LEFT_BUTTON_LONG_TIME)
			{
				button.flag.num_long = 1;
				sButton.num_timeout = 0;
			}
		}
		else
		{
			sButton.num_timeout = 0;
		}
	}
	
	// Exit from LPM3 on RETI
	_BIC_SR_IRQ(LPM3_bits);               
}


// *************************************************************************************************
// @fn          Timer0_A1_5_ISR
// @brief       IRQ handler for timer IRQ.
//				Timer0_A0	1/1sec clock tick (serviced by function TIMER0_A0_ISR)
//				Timer0_A1	BlueRobin timer 
//				Timer0_A2	1/100 sec Stopwatch
//				Timer0_A3	Configurable periodic IRQ (used by button_repeat and buzzer)
//				Timer0_A4	One-time delay
// @param       none
// @return      none
// *************************************************************************************************
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_5_ISR(void)
{
	u16 value;
		
	switch (TA0IV)
	{
		// Timer0_A1	BlueRobin timer
		case 0x02:	// Timer0_A1 handler
					BRRX_TimerTask_v();
					break;

		// Timer0_A2	1/1 or 1/100 sec Stopwatch				
		case 0x04:	// Timer0_A2 handler
					// Disable IE 
					TA0CCTL2 &= ~CCIE;
					// Reset IRQ flag  
					TA0CCTL2 &= ~CCIFG;  

					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Count down speed change
					if ((sCountdown.mode == COUNTDOWN_MODE_ON) && (sCountdown.speed == 2))
					{
						TA0CCR2 += sCountdown.tickspeed[0];
					}
					if ((sCountdown.mode == COUNTDOWN_MODE_ON) && (sCountdown.speed == 3))
					{
						TA0CCR2 += sCountdown.tickspeed[1];
					}
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					// Load CCR register with next capture point
					//update_stopwatch_timer();
					//TA0CCR2 += (32768/100);

					// Enable timer interrupt    
					TA0CCTL2 |= CCIE; 	

					// Increase stopwatch counter
					//stopwatch_tick();

					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Count down speed change
					if (sCountdown.speed > 1)
					{
						countdown_tick();
					}
					/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					break;
					
		// Timer0_A3	Configurable periodic IRQ (used by button_repeat and buzzer)			
		case 0x06:	// Disable IE 
					TA0CCTL3 &= ~CCIE;
					// Reset IRQ flag  
					TA0CCTL3 &= ~CCIFG;  
					// Store new value in CCR
					value = TA0R + sTimer.timer0_A3_ticks; //timer0_A3_ticks_g;
					// Load CCR register with next capture point
					TA0CCR3 = value;   
					// Enable timer interrupt    
					TA0CCTL3 |= CCIE; 	
					// Call function handler
					fptr_Timer0_A3_function();
					break;
		
		// Timer0_A4	One-time delay			
		case 0x08:	// Disable IE 
					TA0CCTL4 &= ~CCIE;
					// Reset IRQ flag  
					TA0CCTL4 &= ~CCIFG;  
					// Set delay over flag
					sys.flag.delay_over = 1;
					break;
	}
	
	// Exit from LPM3 on RETI
	_BIC_SR_IRQ(LPM3_bits);               
}




