#ifndef COUNTUP_H_
#define COUNTUP_H_

// ******************************************
// Defines section
#define COUNTUP_MODE_OFF	(0u)
#define COUNTUP_MODE_ON		(1u)

// ******************************************
// Global Variable section
struct countup
{
	// COUNTUP_MODE_OFF, COUNTUP_MODE_ON
	u8			mode;

	// TIME COUNT
	u8			drawFlag;
	s8			minute;
	s8 			second;

	// Display style
	u8 			viewStyle;
};
extern struct countup sCountup;

// ******************************************
// Extern section
extern void start_countup(void);
extern void stop_countup(void);
extern void reset_countup(void);
extern u8 is_countup_active(void);
extern void countup_tick(void);
extern void sx_countup(u8 line);
extern void mx_countup(u8 line);
extern void display_countup(u8 line, u8 update);

#endif /*COUNTUP_H_*/
