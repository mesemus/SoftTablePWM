# SoftTablePWM
Arduino Software PWM library for enabling PWM on all pins.
The library uses pre-filled tables to minimize CPU overhead in timer invocation.

This is an implementation of a software PWM library which does the most 
of the processing outside of the timer2 interrupt vector. 

The advantage:
- is lower processing overhead for the PWM
 
The downside: 
- memory consumption - each port (PORTB, PORTC, PORTD) needs 256 bytes of memory (allocated on the fly when a pin from that port is used).

Beware that this library is not well tested, take it as a proof of concept for your own optimizations.

The library can be used with the timer2 enabled or disabled (in this case it is up to you to call pwm_cycle on a regular basis).

Usage: see the example file. Simple program:

```c++
#include <soft_table_pwm.h>

// use the timer2 interrupt handler
INSTALL_PWM_INTERRUPT

void setup()   {
  // tell the library that timer2 interrupt handler is used
  pwm_init(true);
  
  // will use PORTB, pin #1 (this is Arduino pin 9, see https://www.arduino.cc/en/uploads/Hacking/Atmega168PinMap2.png) 
  int pwm_channel = pwm_add(PWM_B, 1);

  // set pwm value to 128
  pwm_set(pwm_channel, 128);
}

void loop() {}
```

Enjoy!

ms.
