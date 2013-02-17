// ******************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "countup.h"
#include "ports.h"
#include "timer.h"
#include "countdown.h"
#include "buzzer.h"
#include "menu.h"


// ******************************************
// Global Variable section
struct countup sCountup;

// ******************************************
// Extern section

// ******************************************
// @fn          reset_countup
// @brief       Reset / INIT countup data.
// @param       none
// @return      none
// ******************************************
void reset_countup(void)
{
	// Start point
	sCountup.minute = 0;
	sCountup.second = 0;

	// Set countup mode to reset
	sCountup.mode = COUNTUP_MODE_OFF;

	// Default display style is MM:SS
	sCountup.viewStyle = DISPLAY_DEFAULT_VIEW;
}

// *************************************************************************************************
// @fn          start_countup
// @brief       Sets countup mode to on.
// @param       none
// @return      none
// *************************************************************************************************
void start_countup(void)
{
	// Set countup mode to on
	sCountup.mode = COUNTUP_MODE_ON;
}

// *************************************************************************************************
// @fn          stop_countup
// @brief       Sets countup mode to off.
//				Does not reset countup count.
// @param       none
// @return      none
// *************************************************************************************************
void stop_countup(void)
{
	// Clear countup run flag
	sCountup.mode = COUNTUP_MODE_OFF;

	// Call draw routine immediately
	display_countup(LINE2, DISPLAY_LINE_UPDATE_FULL);
}


// *************************************************************************************************
// @fn          mx_countup
// @brief       Unused
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void mx_countup(u8 line)
{
}

// ******************************************
// @fn          sx_countup
// @brief       Unused
// @param       u8 line		LINE2
// @return      none
// ******************************************
void sx_countup(u8 line)
{
}

// *************************************************************************************************
// @fn          display_countup
// @brief       Countup user routine.
// @param       u8 line	LINE2
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_countup(u8 line, u8 update)
{
	// Partial update
 	if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
 		if (display.flag.update_countup)
 		{
 			if(sCountup.drawFlag != 0)
 			{
 				if (sCountup.viewStyle == DISPLAY_DEFAULT_VIEW)
 				{
 					switch(sCountup.drawFlag)
 					{
						// Display minute
						case 2: display_chars(switch_seg(line, LCD_SEG_L2_3_0, LCD_SEG_L2_3_0), itoa(sCountup.minute, 2, 0), SEG_ON);

						// Display second
						case 1: display_chars(switch_seg(line, LCD_SEG_L2_1_0, LCD_SEG_L2_1_0), itoa(sCountup.second, 2, 0), SEG_ON);
 					}
 				}
 			}
 		}
	}
 	// Full update
	else if (update == DISPLAY_LINE_UPDATE_FULL)
	{
		if (sCountup.viewStyle == DISPLAY_DEFAULT_VIEW)
		{
			// Display minute
			display_chars(switch_seg(line, LCD_SEG_L2_3_0, LCD_SEG_L2_3_0), itoa(sCountup.minute, 2, 0), SEG_ON);
			display_symbol(switch_seg(line, LCD_SEG_L2_COL1, LCD_SEG_L2_COL0), SEG_ON_BLINK_ON);

			// Display second
			display_chars(switch_seg(line, LCD_SEG_L2_1_0, LCD_SEG_L2_1_0), itoa(sCountup.second, 2, 0), SEG_ON);
		}
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		display_symbol(switch_seg(line, LCD_SEG_L2_COL1, LCD_SEG_L2_COL0), SEG_OFF_BLINK_OFF);
		sCountup.viewStyle = DISPLAY_DEFAULT_VIEW;
	}
}

// ******************************************
// @fn          is_countup_active
// @brief       Returns 1 if countup module is currently active.
// @param       none
// @return      u8		1 = countup module active
// ******************************************
u8 is_countup_active(void)
{
	return ((sCountup.mode == COUNTUP_MODE_ON) && (ptrMenu_L2 == &menu_L2_Countup));
}

// *************************************************************************************************
// @fn          countup_tick
// @brief       Increases countup counter and triggers display update.
// @param       none
// @return      none
// *************************************************************************************************
void countup_tick(void)
{
		// Using sCountup.drawFlag to minimize display updates
		// sCountup.drawFlag = 1: second
		// sCountup.drawFlag = 2: minute

		sCountup.drawFlag = 1;

		// Detect 10:00
		if (sCountup.minute == 10 && sCountup.second == 0)
		{
			stop_countdown();
			stop_countup();
			stop_buzzer();
			reset_buzzer();
			start_buzzer(10, CONV_MS_TO_TICKS(100), CONV_MS_TO_TICKS(250));
		}
		else
		{
			// Increment second count
			sCountup.second++;

			// Increment minute
			if (sCountup.second == 60)
			{
				sCountup.second = 0;
				sCountup.minute++;
				sCountup.drawFlag++;
			}
		}

		// Display output
		display.flag.update_countup = 1;
}

