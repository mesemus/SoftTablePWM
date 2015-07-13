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

#define PORTS 3

static volatile uint8_t *pwm_table[] = {
		NULL, // PWM_B
		NULL, // PWM_C
		NULL  // PWM_D
};

static volatile uint8_t pwm_mask[] = {
		(uint8_t)0xff,
		(uint8_t)0xff,
		(uint8_t)0xff
};

typedef struct {
	int port : 4;
	int pin  : 4;
} PWMRegistration;


static PWMRegistration pwm_registration[MAX_PWM];
static uint8_t         pwm_level[MAX_PWM];
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
	if (!pwm_table[port]) {
		pwm_table[port] = (uint8_t *)calloc(1, 256);
	}
	pwm_registration_length++;
	update_masks();

	switch (port) {
	case 0: DDRB |= 1<<pin; break;
	case 1: DDRC |= 1<<pin; break;
	case 2: DDRD |= 1<<pin; break;
	}

	return pwm_registration_length - 1;
}

static void _pwm_update(uint8_t channel, int value) {
	pwm_level[channel] = value; // fading will not work with 256 ...
	uint8_t port = pwm_registration[channel].port;
	uint8_t mask = 1 << pwm_registration[channel].pin;
	volatile uint8_t *table = pwm_table[port];
	int i;
	if (value>256) {
		value = 256;
	}
	for (i=0; i<value; i++) {
		*table++ |= mask;
	}
	mask = ~mask;
	for (i=value; i<256; i++) {
		*table++ &= mask;
	}
#ifdef DEBUG
	Serial.print("PWM table: ");
	table = pwm_table[port];
	for (i=0; i<256; i++) {
		Serial.print(table[i]);
		Serial.print(" ");
	}
	Serial.println();
#endif
}

void pwm_set(uint8_t channel, int value) { // 0 .. 256 inclusive
	pwm_fade_start[channel] = pwm_fade_level[channel] = value;
	_pwm_update(channel, value);
}

void pwm_cycle() {

	pwm_counter++;
	if (pwm_counter == pwm_len_mask) {
		pwm_counter = 0;
	}


#	define __SET(port, idx) 								\
		if (pwm_table[idx]) { 								\
			port = (port & pwm_mask[idx]) | pwm_table[idx][pwm_counter];\
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
		_pwm_update(i, map(pwm_fade_step, 0, pwm_fade_steps - 1,
				   pwm_fade_start[i], pwm_fade_level[i]));
	}
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
		if (pwm_table[i]) {
			pwm_mask[i] = (uint8_t)~temp_masks[i];
		}
	}
}

