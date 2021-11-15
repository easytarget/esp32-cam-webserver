## A Simple C program used to debug and examine the log function I used for LED control

The bit width (precision) for the PWM can be specified (max. 16 bit)

The Input values are integer percentages from 0-100.

The Output is a logarithmically scaling integer between 0 and the max PWM value.

``` C
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
```

Adjust the width as necessary and compile with: 

    $ gcc linearled.c -o linearled -lm
    
And run to see the results:
```bash
$ ./linearled 9

The PWM Bit width (resolution) specified is 9, pwm range is from 0 to 511
-----------
   0 :     0
   1 :     2
   2 :     5
   3 :     7
   4 :    10
   5 :    12
   6 :    15
   7 :    17
   8 :    20
   9 :    23
  10 :    25
  11 :    28
  12 :    31
  13 :    34
  14 :    36
  15 :    39
  16 :    42
  17 :    45
  18 :    48
  19 :    51
  20 :    54
  21 :    58
  22 :    61
  23 :    64
  24 :    67
  25 :    71
  26 :    74
  27 :    77
  28 :    81
  29 :    84
  30 :    88
  31 :    91
  32 :    95
  33 :    99
  34 :   103
  35 :   106
  36 :   110
  37 :   114
  38 :   118
  39 :   122
  40 :   126
  41 :   130
  42 :   135
  43 :   139
  44 :   143
  45 :   148
  46 :   152
  47 :   156
  48 :   161
  49 :   166
  50 :   170
  51 :   175
  52 :   180
  53 :   185
  54 :   190
  55 :   195
  56 :   200
  57 :   205
  58 :   210
  59 :   216
  60 :   221
  61 :   226
  62 :   232
  63 :   238
  64 :   243
  65 :   249
  66 :   255
  67 :   261
  68 :   267
  69 :   273
  70 :   279
  71 :   285
  72 :   292
  73 :   298
  74 :   305
  75 :   311
  76 :   318
  77 :   325
  78 :   332
  79 :   339
  80 :   346
  81 :   353
  82 :   361
  83 :   368
  84 :   375
  85 :   383
  86 :   391
  87 :   399
  88 :   407
  89 :   415
  90 :   423
  91 :   431
  92 :   439
  93 :   448
  94 :   457
  95 :   465
  96 :   474
  97 :   483
  98 :   492
  99 :   502
 100 :   511
```

The code can be adapted into your custom LED setting function


