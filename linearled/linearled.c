/* Dump linear led values */
#include "stdio.h"
#include <stdlib.h>
#include <math.h>

//int pwmBits = 9;  // Number of PWM bits

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("\nERROR: This utility expects a single parameter; the bit-width of the PWM stream\n\n");
    return 1; }  
  
  int pwmBits = atoi(argv[1]);

  if (pwmBits < 2 || pwmBits > 16) {
    printf("\nERROR: '%s' is not an integer in the range 2-16.\n\n", argv[1]);
    return 1; }  
  
  int pwmMax = pow(2,pwmBits)-1;

  printf("\nThe PWM Bit width (resolution) specified is %i, pwm range is from 0 to %i\n-----------\n", pwmBits, pwmMax);

  for (int val = 0; val <= 100; val++) {
    int pwm = round((pow(2,(1+(val*0.02)))-2)/6*pwmMax);
    printf(" %3i : %5i\n", val, pwm);
  }
  printf("\n");
  return 0;
}
