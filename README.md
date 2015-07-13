# SoftTablePWM
Arduino Software PWM library for enabling PWM on all pins.
The library uses pre-filled tables to minimize CPU overhead in timer invocation.

This is an implementation of a software PWM library which does the most 
of the processing outside of the timer2 interrupt vector. 

The advantage:
- is lower processing overhead for the PWM
 
The downside is the memory consumption 
- each port (PORTB, PORTC, PORTD) needs 256 bytes of memory (allocated on the fly when a pin from that port is used).

Beware that this library is not well tested, take it as a proof of concept for your own optimizations.

Enjoy!

ms.
