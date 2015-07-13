/*
 * soft_table_pwm.h
 *
 *  Created on: Jul 13, 2015
 *      Author: simeki
 */

#ifndef SOFT_TABLE_PWM_H_
#define SOFT_TABLE_PWM_H_

#include <Arduino.h>

#ifndef MAX_PWM
#define MAX_PWM 10
#endif

#define PWM_B 0
#define PWM_C 1
#define PWM_D 2

void pwm_init(bool use_interrupt, int len = 256);

uint8_t pwm_add(uint8_t port, uint8_t pin);

void pwm_set(uint8_t channel, int value);

void pwm_fade(uint8_t channel, int value);

// call only if not using the interrupt mode ...
void pwm_cycle();

// call this for a fade effect ...
bool pwm_run_fade();

#define INSTALL_PWM_INTERRUPT 				\
		ISR(TIMER2_OVF_vect) {				\
			pwm_cycle();					\
		}


#endif /* SOFT_TABLE_PWM_H_ */
