/*
 * soft_table_pwm.cc
 *
 *  Created on: Jul 13, 2015
 *      Author: simeki
 */
#include "SoftTablePWM.h"
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>

// workaround for eclipse - does not detect correctly these
// avr-gcc gracefully skips this
#ifndef OCIE2A
#define OCIE2A NULL
#define TCCR2A NULL
#define TCCR2B NULL
#define TIMSK2 NULL
#define WGM22 NULL
#endif

#ifdef PWM_DEBUG
#define dprint(x) Serial.print(x)
#define dbprint(x) bprint(x)
#define dprintln() Serial.println()
#else
#define dprint(x) do { } while (0)
#define dbprint(x) do { } while (0)
#define dprintln() do { } while (0)
#endif

#define PORTS 3

// 8 pins per port, that is max 8+1 change values
static volatile uint8_t pwm_port_vals[][10] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

// +1 - stopper
static volatile uint8_t pwm_port_thresholds[][10] = {
		{0, 255, 0, 0, 0, 0, 0, 0, 0},
		{0, 255, 0, 0, 0, 0, 0, 0, 0},
		{0, 255, 0, 0, 0, 0, 0, 0, 0},
};

static uint8_t pwm_port_top[] = {
		0,
		0,
		0
};

static volatile uint8_t pwm_mask[] = {
		0xff,
		0xff,
		0xff
};

typedef struct {
	int port : 4;
	int pin  : 4;
} PWMRegistration;


static PWMRegistration pwm_registration[MAX_PWM];
static int             pwm_level[MAX_PWM];
static uint8_t         pwm_fade_start[MAX_PWM];
static uint8_t         pwm_fade_level[MAX_PWM];
static uint8_t         pwm_fade_steps = 255;
static uint8_t         pwm_fade_step  = 255;
static uint8_t 		   pwm_registration_length = 0;

static volatile int   		    pwm_counter = 255;

static volatile int             pwm_len_mask;

static void update_masks();

/*
 *
 * public functions
 *
 */

#ifndef __AVR_ATmega328P__
#error "Unsupported architecture"
#endif

void pwm_init(bool use_interrupt, int len) {
	pwm_len_mask = len;

	if (use_interrupt) {
		cli();

		// disable interrupts on timer2
		TIMSK2 = 0;

		// pure counting, no pwm
		TCCR2A &= ~((1<<WGM21) | (1<<WGM20));


		TCCR2B =
				// no clear on match, just overflow (and generate interrupt)
				// ~(1<<WGM22);

				// prescaler: /1
				_BV(CS20)
//				_BV(CS21)
//				_BV(CS22)
		;

		// internal clock source (should be default, just for sure)
		ASSR &= ~(1<<AS2);

		// enable the timer
		TIMSK2 |= (1<<TOIE2);
		sei();
	}
}

uint8_t pwm_add(uint8_t port, uint8_t pin) {
	if (pwm_registration_length == MAX_PWM) {
		return -1;
	}
	pwm_registration[pwm_registration_length].port = port;
	pwm_registration[pwm_registration_length].pin  = pin;
	pwm_level[pwm_registration_length]             = 0;

	pwm_registration_length++;
	update_masks();

	switch (port) {
	case 0: DDRB |= 1<<pin; break;
	case 1: DDRC |= 1<<pin; break;
	case 2: DDRD |= 1<<pin; break;
	}

	return pwm_registration_length - 1;
}

static void bprint(uint8_t x) {
	uint8_t i = 128;
	do {
		if (x&i) {
			Serial.print('1');
		} else {
			Serial.print('0');
		}
		i>>=1;
	} while (i);
}

static void _pwm_update() {
	uint8_t tmp_threshold[10];
	uint8_t tmp_mask[10];
	for (int p=0; p<PORTS; p++) {
		// if there is no pwm on this port, skip it.
		if (pwm_mask[p] == 0xff) {
			continue;
		}

		int     lastval  = -1;
		int     lasttop  = 0;
		int     foundval = 257;
		uint8_t mask = ~pwm_mask[p];

		while(true) {
			foundval = 257;
			// find the lowest pwm setting with value>lastval
			for (int r=0; r<pwm_registration_length; r++) {
				if (pwm_registration[r].port != p) {
					continue;
				}
				if (pwm_level[r]>lastval && pwm_level[r]<foundval) {
					foundval = pwm_level[r];
				}
			}
			// for each of the pwms, if its value == foundval, set the
			// output to low
			uint8_t nmask = mask;
			for (int r=0; r<pwm_registration_length; r++) {
				if (pwm_registration[r].port != p) {
					continue;
				}
				if (pwm_level[r] == foundval) {
					nmask &= ~_BV(pwm_registration[r].pin);
				}
			}
			// if we're here the first and the found threshold is greater
			// than 0, insert initial mask

			if (!lasttop && foundval != 0) {
				// first value
				tmp_threshold[lasttop] = 0;
				tmp_mask[lasttop++] = mask;
			}
			mask = nmask;
			// if there is a pwm value, add it
			if (foundval<256) {
				tmp_threshold[lasttop] = foundval;
				tmp_mask[lasttop++] = mask;
				lastval = foundval;
			} else {
				// otherwise we have ended ...
				break;
			}
		};
		// insert a fence so that it does not overflow ...
		tmp_threshold[lasttop] = 0;
		tmp_mask[lasttop]      = mask;

		dprint("Fence is at : ");
		dprint(lasttop);
		dprintln();
		// copy back to the pwm_port_vals, pwm_port_thresholds.
		// start from back so that we make sure there is no overflow
		for (; lasttop>=0; lasttop--) {
			pwm_port_vals[p][lasttop] = tmp_mask[lasttop];
			pwm_port_thresholds[p][lasttop] = tmp_threshold[lasttop];
		}

#ifdef PWM_DEBUG
		// a bit of debugging
		for (lasttop=0; lasttop<10; lasttop++) {
			Serial.print(pwm_port_thresholds[p][lasttop]);
			Serial.print(" [");
			bprint(pwm_port_vals[p][lasttop]);
			Serial.print("] ");
		}
		Serial.println();
#endif
	}
}

void pwm_set(uint8_t channel, int value) { // 0 .. 256 inclusive
	pwm_level[channel] = value; // fading will not work with 256 ...
	pwm_fade_start[channel] = pwm_fade_level[channel] = value;
	_pwm_update();
}

#define __SET(port, idx)											\
{																	\
	uint8_t top  = pwm_port_top[idx];								\
	uint8_t thr  = pwm_port_thresholds[idx][top];					\
	dprint(pwm_counter); dprint(" ");								\
	dprint(idx); dprint(":"); dprint(top); dprint(" thr:");			\
	dprint(thr);													\
	if (thr == pwm_counter) {										\
		port = (port & pwm_mask[idx]) | pwm_port_vals[idx][top];	\
		dprint(" msk:"); dbprint(pwm_port_vals[idx][top]);			\
		pwm_port_top[idx] = top + 1;								\
	}																\
	dprintln();														\
}

void pwm_cycle() {

	pwm_counter++;
	if (pwm_counter == pwm_len_mask) {
		pwm_counter = 0;
		pwm_port_top[0] = pwm_port_top[1] = pwm_port_top[2] = 0;
	}

#	if PORTS>=1
		__SET(PORTB, 0)
#	endif

#	if PORTS>=2
		__SET(PORTC, 1)
#	endif

#	if PORTS>=3
		__SET(PORTD, 2)
#	endif

#	if PORTS>=4
#error "Set up ports pwm_cycle for this number of ports"
#	endif
}

void pwm_fade(uint8_t channel, int value) {
	pwm_fade_level[channel] = value;
	for (int i=0; i<pwm_registration_length; i++) {
		pwm_fade_start[i] = pwm_level[i];
	}
	pwm_fade_step = 0;// reset step
}

bool pwm_run_fade() {
	// Serial.print("pwm fade step : ");
	// Serial.println(pwm_fade_step);
	if (pwm_fade_step == pwm_fade_steps) {
		return true;
	}
	for (int i=0; i<pwm_registration_length; i++) {
//		Serial.print(i);
//		Serial.print(" start: ");
//		Serial.print(pwm_fade_start[i]);
//		Serial.print(", end: ");
//		Serial.println(pwm_fade_level[i]);
		pwm_level[i] = map(pwm_fade_step, 0, pwm_fade_steps - 1,
				   pwm_fade_start[i], pwm_fade_level[i]);
	}
	_pwm_update();
	pwm_fade_step++;
	return false;
}

/*
 *
 * private functions
 *
 */

static void update_masks() {
	uint8_t temp_masks[PORTS];
	uint8_t i;
	for (i=0; i<PORTS; i++) {
		temp_masks[i] = 0;
	}
	for (i=0; i<pwm_registration_length; i++) {
		temp_masks[pwm_registration[i].port] |= 1<<pwm_registration[i].pin;
	}
	for (i=0; i<PORTS; i++) {
		pwm_mask[i] = (uint8_t)~temp_masks[i];
	}
}

