#include <SoftTablePWM.h>

int pwm_channels[8];

INSTALL_PWM_INTERRUPT

// The setup() method runs once, when the sketch starts
void setup()   {
  // initialize the digital pin as an output:
  Serial.begin(115200);
  pwm_init(true);
  // do not touch PB6, PB7 as this is a crystal :)
  for (int i=0; i<6; i++) {
	  pwm_channels[i] = pwm_add(PWM_B, i);
	  pwm_set(pwm_channels[i], 1);
	  pwm_fade(pwm_channels[i], 255);
  }
  Serial.println("initialized");
}

// the loop() method runs over and over again,
// as long as the Arduino has power

bool ramping_up = true;

void loop()
{
	Serial.println("tick :)");
	if (pwm_run_fade()) {
		ramping_up = !ramping_up;
		  for (int i=0; i<6; i++) {
			  pwm_fade(pwm_channels[i], ramping_up?255:0);
		}
	}
	delay(1);
}
