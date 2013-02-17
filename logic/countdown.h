#ifndef COUNTDOWN_H_
#define COUNTDOWN_H_

// ******************************************
// Defines section
#define COUNTDOWN_MODE_OFF		(0u)
#define COUNTDOWN_MODE_ON		(1u)
#define COUNTDOWN_MODE_RESET	(2u)

// ******************************************
// Global Variable section
struct countdown
{
	// COUNTDOWN_MODE_OFF, COUNTDOWN_MODE_ON, COUNTDOWN_MODE_RESET
	u8			mode;

	// TIME COUNT
	u8			drawFlag;
	s8			minute;
	s8 			second;
	u8			speed;
	u8			buzzer;
	u8			thresholdlow[3];
	u8			thresholdhigh[3];
	u8			exceedcount[2];
	u8			exceeddefault[2];
	u8			recoverycount;
	u8			recoverydefault;
	s8			sw1presses;
	s8			sw1pressmax;
	u32			tickspeed[2];

	// Display style
	u8 			viewStyle;
};
extern struct countdown sCountdown;

// ******************************************
// Extern section
extern void start_countdown(void);
extern void stop_countdown(void);
extern void reset_countdown(void);
extern void countdown_droptozero(void);
extern void countdown_sw1recovery(void);
extern u8 is_countdown_active(void);
extern void countdown_tick(void);
extern void sx_countdown(u8 line);
extern void mx_countdown(u8 line);
extern void display_countdown(u8 line, u8 update);

#endif /*COUNTDOWN_H_*/
