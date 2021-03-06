# SoftTablePWM

## Look at the lowmem branch, same API and lower memory usage

Arduino Software PWM library for enabling PWM on all pins, supports 'set' and 'fade'.
The library uses pre-filled tables to minimize CPU overhead in timer invocation.
Default frequency of the PWM (on 16MHz processor) is 244Hz, might be increased
via decreasing the number of PWM levels in pwm_init function call.

Alternatively change the source code of pwm_init and the interrupt on timer2 
to occur on comparison rather than on overflow (which is once in 256 ticks) - 
if you then set the OCR2A register to lower value than 255, the timer will 
be called more often and the frequency will increase - see 
https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM for more details.

The advantage of this library:
- lower processing overhead for the PWM
 
The downside: 
- memory consumption - each port (PORTB, PORTC, PORTD) needs 256 bytes of memory (allocated on the fly when a pin from that port is used).

Beware that this library is not well tested, take it as a proof of concept for your own optimizations.

The library can be used with the timer2 enabled or disabled (in this case it is up to you to call pwm_cycle on a regular basis).

<h2>Usage</h2> 

See fade.ino in the example directory. 

Simple program:

```c++
#include <SoftTablePWM.h>

// use the timer2 interrupt handler
INSTALL_PWM_INTERRUPT

void setup()   {
  // tell the library that timer2 interrupt handler is used
  pwm_init(true);
  
  // will use PORTB, pin #1 (this is Arduino pin 9, 
  // see https://www.arduino.cc/en/uploads/Hacking/Atmega168PinMap2.png ) 
  int pwm_channel = pwm_add(PWM_B, 1);

  // set pwm value to 128
  pwm_set(pwm_channel, 128);
}

void loop() {}
```

Enjoy!

ms.
