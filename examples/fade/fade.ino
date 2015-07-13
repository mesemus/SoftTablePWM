#include <SoftTablePWM.h>

int pwm_channel;

INSTALL_PWM_INTERRUPT

// The setup() method runs once, when the sketch starts
void setup()   {
  // initialize the digital pin as an output:
  Serial.begin(115200);
  pwm_channel = pwm_add(PWM_B, 1);
  pwm_init(true, 256);
  pwm_set(pwm_channel, 0);
  pwm_fade(pwm_channel, 255);
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
		pwm_fade(pwm_channel, ramping_up?255:0);
	}
	delay(10);
}
