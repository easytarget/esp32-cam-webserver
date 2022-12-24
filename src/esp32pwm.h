/**
 * esp32pwm.h
 *
 *  Created on: Sep 22, 2018
 *      Author: hephaestus
 *  Refactored on: Dec 25, 2022
 *      Author: abratchik
 * 
 *  Refactored on: Dec 25, 2022
 */

#ifndef esp32pwm_h
#define esprepwm_h

#include "esp32-hal-ledc.h"

#define NUM_PWM 16
#define PWM_BASE_INDEX 0

#define MIN_PULSE_WIDTH       500     // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH      2500     // the longest pulse sent to a servo 
#define DEFAULT_PULSE_WIDTH  1500     // default pulse width when servo is attached
#define REFRESH_USEC        20000

#define USABLE_ESP32_PWM (NUM_PWM-PWM_BASE_INDEX)

#include <cstdint>

#include "Arduino.h"
class ESP32PWM {

    public:
        // setup
        ESP32PWM();
        virtual ~ESP32PWM();


        void detachPin(int pin);
        void attachPin(uint8_t pin, double freq, uint8_t resolution_bits);

        // write raw duty cycle
        void write(uint32_t duty);
        // Write a duty cycle to the PWM using a unit vector from 0.0-1.0
        void writeScaled(double duty);
        //Adjust frequency
        void adjustFrequency(double freq, double dutyScaled=-1);

        int usToTicks(int usec);

        // Read pwm data
        uint32_t read();
        
        double getDutyScaled();

        //Timer data
        static int timerAndIndexToChannel(int timer, int index);
        /**
         * allocateTimer
         * @param a timer number 0-3 indicating which timer to allocate in this library
         * Switch to explicate allocation mode
         *
         */
        static void allocateTimer(int timerNumber);

        
        // Helper functions
        int getPin() {return pin;};
        int getChannel();
        bool attached() {return attachedState;};
        double getFreq() {return myFreq;};
        int getTimer() {return timerNum;}
        uint8_t getResolutionBits() {return resolutionBits;};


        static bool hasPwm(int pin) {
#if defined(ARDUINO_ESP32S2_DEV)
            if ((pin >=1 && pin <= 21) || //20
                    (pin == 26) || //1
                    (pin >= 33 && pin <= 42)) //10
#else
            if ((pin == 2) || //1
                    (pin == 4) || //1
                    (pin == 5) || //1
                    ((pin >= 12) && (pin <= 19)) || //8
                    ((pin >= 21) && (pin <= 23)) || //3
                    ((pin >= 25) && (pin <= 27)) || //3
                    (pin == 32) || (pin == 33)) //2
#endif
                return true;
            return false;
        }

        static int channelsRemaining() {return NUM_PWM - PWMCount;};
        
        int allocatenext(double freq);

        bool checkFrequencyForSideEffects(double freq);
        void adjustFrequencyLocal(double freq, double dutyScaled);

        double setup(double freq, uint8_t resolution_bits=10);
        
        //channel 0-15 resolution 1-16bits freq limits depend on resolution9
        void attachPin(uint8_t pin);
        
        // pin allocation
        void deallocate();

        static double mapf(double x, double in_min, double in_max, double out_min,
                double out_max) {
            if(x>in_max)
                return out_max;
            if(x<in_min)
                return out_min;
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        };

    protected:
        static double _ledcSetupTimerFreq(uint8_t chan, double freq,
                uint8_t bit_num);
        void attach(int pin);

    private:
        static int PWMCount;                 // the total number of attached pwm
        static int timerCount[4];
        static ESP32PWM * ChannelUsed[NUM_PWM]; // used to track whether a channel is in service
        static long timerFreqSet[4];
        static bool explicateAllocationMode;

        int timerNum = -1;
        uint32_t myDuty = 0;


        int pwmChannel = 0;                         
        bool attachedState = false;
        int pin;
        uint8_t resolutionBits;
        double myFreq;

};

#endif /* esp32pwm_h */
