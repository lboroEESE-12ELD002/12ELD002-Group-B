// ******************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "countdown.h"
#include "ports.h"
#include "timer.h"
#include "countup.h"
#include "buzzer.h"
#include "menu.h"
#include "acceleration.h"


// ******************************************
// Global Variable section
struct countdown sCountdown;

// ******************************************
// Extern section

// ******************************************
// @fn          reset_countdown
// @brief       Reset / INIT countdown data.
// @param       none
// @return      none
// ******************************************
void reset_countdown(void)
{
	// USER VARIABLES //

	// Number of seconds required under threshold before returning speed to normal (1)
	sCountdown.recoverydefault = 5;

	// Number of seconds required over threshold before increasing speed to 3
	sCountdown.exceeddefault[0] = 10;

	// Number of seconds required over threshold before dropping counts remaining to 0
	sCountdown.exceeddefault[1] = 10;

	// Set xaxis thresholds
	sCountdown.thresholdlow[0] = 45;
	sCountdown.thresholdhigh[0] = 215;

	// Set yaxis thresholds
	sCountdown.thresholdlow[1] = 40;
	sCountdown.thresholdhigh[1] = 215;

	// Set zaxis thresholds (DISABLED)
	//sCountdown.thresholdlow[2] = 10;
	//sCountdown.thresholdhigh[2] = 245;

	// Countdown tick speed for SPEED = 2
	sCountdown.tickspeed[0] = 32768/2;

	// Countdown tick speed for SPEED = 3
	sCountdown.tickspeed[1] = 32768/4;


	// DO NOT EDIT //

	sCountdown.speed = 1; // Default speed

	// Recovery & Exceed count INIT
	sCountdown.exceedcount[0] = sCountdown.exceeddefault[0];
	sCountdown.exceedcount[1] = sCountdown.exceeddefault[1];
	sCountdown.recoverycount = sCountdown.recoverydefault;

	// Set default counts remaining
	sCountdown.minute = 10;
	sCountdown.second = 0;

	// SW1 - cannot have MORE presses than this
	sCountdown.sw1pressmax = 10;
	sCountdown.sw1presses = sCountdown.sw1pressmax;

	// Buzzer is off by default
	sCountdown.buzzer = 0;
	stop_buzzer();
	reset_buzzer();

	// Switch heart icon off
	display_symbol(LCD_ICON_HEART, SEG_OFF);

	// Set countdown mode to reset - allows for start alteration
	sCountdown.mode = COUNTDOWN_MODE_RESET;

	// Default display style is MM:SS
	sCountdown.viewStyle = DISPLAY_DEFAULT_VIEW;
}

// *************************************************************************************************
// @fn          start_countdown
// @brief       Sets countdown mode to on.
// @param       none
// @return      none
// *************************************************************************************************
void start_countdown(void)
{
	// Set countdown mode to on
	sCountdown.mode = COUNTDOWN_MODE_ON;

	// Start acceleration measurement
	start_acceleration_measurement();
}

// *************************************************************************************************
// @fn          stop_countdown
// @brief       Sets countdown mode to off.
//				Does not reset countdown count.
// @param       none
// @return      none
// *************************************************************************************************
void stop_countdown(void)
{
	// Clear countdown run flag
	sCountdown.mode = COUNTDOWN_MODE_OFF;

	// Stop acceleration measurement
	stop_acceleration_measurement();

	// Call draw routine immediately
	display_countdown(LINE1, DISPLAY_LINE_UPDATE_FULL);

	// Clear timer interrupt enable
	TA0CCTL2 &= ~CCIE;
}


// *************************************************************************************************
// @fn          mx_countdown
// @brief       Unused
// @param       u8 line	LINE1
// @return      none
// *************************************************************************************************
void mx_countdown(u8 line)
{
}

// ******************************************
// @fn          sx_countdown
// @brief       Unused
// @param       u8 line		LINE1
// @return      none
// ******************************************
void sx_countdown(u8 line)
{
}

// *************************************************************************************************
// @fn          display_countdown
// @brief       Countdown user routine.
// @param       u8 line	LINE1
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_countdown(u8 line, u8 update)
{
	// Partial update
 	if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
 		if (display.flag.update_countdown)
 		{
 			if(sCountdown.drawFlag != 0)
 			{
 				if (sCountdown.viewStyle == DISPLAY_DEFAULT_VIEW)
 				{
 					switch(sCountdown.drawFlag)
 					{
						// Display minute
						case 2: display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L1_3_2), itoa(sCountdown.minute, 2, 0), SEG_ON);

						// Display second
						case 1: display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L1_1_0), itoa(sCountdown.second, 2, 0), SEG_ON);
 					}
 				}
 			}
 		}
	}
 	// Full update
	else if (update == DISPLAY_LINE_UPDATE_FULL)
	{
		if (sCountdown.viewStyle == DISPLAY_DEFAULT_VIEW)
		{
			// Display minute
			display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L1_3_2), itoa(sCountdown.minute, 2, 0), SEG_ON);
			display_symbol(switch_seg(line, LCD_SEG_L1_COL, LCD_SEG_L2_COL0), SEG_ON_BLINK_ON);

			// Display second
			display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L1_1_0), itoa(sCountdown.second, 2, 0), SEG_ON);
		}
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		display_symbol(switch_seg(line, LCD_SEG_L1_COL, LCD_SEG_L2_COL0), SEG_OFF_BLINK_OFF);
		sCountdown.viewStyle = DISPLAY_DEFAULT_VIEW;
	}
}

// ******************************************
// @fn          is_countdown_active
// @brief       Returns 1 if countdown module is currently active.
// @param       none
// @return      u8		1 = countdown module active
// ******************************************
u8 is_countdown_active(void)
{
	return ((sCountdown.mode == COUNTDOWN_MODE_ON) && (ptrMenu_L1 == &menu_L1_Countdown));
}

// *************************************************************************************************
// @fn          countdown_tick
// @brief       Decreases countdown counter and triggers display update.
// @param       none
// @return      none
// *************************************************************************************************
void countdown_tick(void)
{
		// Using sCountdown.drawFlag to minimize display updates
		// sCountdown.drawFlag = 1: second
		// sCountdown.drawFlag = 2: minute
		sCountdown.drawFlag = 1;

		// Decrement second count
		sCountdown.second--;

		// Detect 00:00
		if (sCountdown.minute == 0 && sCountdown.second == 0)
		{
			countdown_droptozero();
		}

		// Decrement minute
		if (sCountdown.second == -1)
		{
			sCountdown.second = 59;
			sCountdown.minute--;
			sCountdown.drawFlag++;
		}

		if (sCountdown.mode == COUNTDOWN_MODE_ON)
		{
			display.flag.update_countdown = 1;

			// Heart icon flash
			if (sCountdown.second%2 == 0)
			{
				display_symbol(LCD_ICON_HEART, SEG_ON);
			}
			else
			{
				display_symbol(LCD_ICON_HEART, SEG_OFF);
			}

			// Beep
			if (sCountdown.buzzer == 1)
			{
				if (sCountdown.speed == 1)
				{
					start_buzzer(1, CONV_MS_TO_TICKS(100), CONV_MS_TO_TICKS(250));
				}
				if (sCountdown.speed == 2)
				{
					start_buzzer(1, CONV_MS_TO_TICKS(50), CONV_MS_TO_TICKS(200));
				}
				if (sCountdown.speed == 3)
				{
					start_buzzer(1, CONV_MS_TO_TICKS(20), CONV_MS_TO_TICKS(150));
				}
			}
		}
}


// *************************************************************************************************
// @fn          countdown_droptozero
// @brief       Drops countdown to zero, stops and buzzes
// @param       none
// @return      none
// *************************************************************************************************
void countdown_droptozero(void)
{
	sCountdown.minute = 0;
	sCountdown.second = 0;
	stop_countdown();
	//stop_countup();
	stop_buzzer();
	reset_buzzer();
	start_buzzer(60, CONV_MS_TO_TICKS(1000), CONV_MS_TO_TICKS(1));
}

// *************************************************************************************************
// @fn          countdown_sw1recovery
// @brief       Decrement recovery count (0 is reached faster, meaning recovery is faster)
// @param       none
// @return      none
// *************************************************************************************************
void countdown_sw1recovery(void)
{
	if (sCountdown.mode == COUNTDOWN_MODE_ON)
	{
		if (sCountdown.sw1presses != 0)
		{
			sCountdown.sw1presses--;

			// Speed up recovery process
			if (sCountdown.recoverycount != 0)
			{
				sCountdown.recoverycount--;
			}
			if (sCountdown.recoverydefault != 0)
			{
				sCountdown.recoverydefault--;
			}
		}
		else
		{
			countdown_droptozero();
		}
	}
}
