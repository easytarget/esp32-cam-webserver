/* Dump linear led values */
#include "stdio.h"
#include <math.h>

int pwmBits = 9;  // Number of PWM bits

void main() {
  int pwmMax = pow(2,pwmBits)-1;

  for (int val = 0; val <= 100; val++) {
    int pwm = round((pow(2,(1+(val*0.02)))-2)/6*pwmMax);
    printf("%i : %i\n", val, pwm);
  }
}
