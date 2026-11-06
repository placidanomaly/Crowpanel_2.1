#ifndef STOPWATCHCONTROLLER_H
#define STOPWATCHCONTROLLER_H

#include <crowpanel_2_1.h>

class StopwatchController: public CrowController {
    public:        
        static StopwatchController& getInstance();

        virtual void begin(Crowpanel &panel);
    
        void start();
        void stop();
        void reset();

        struct StopwatchValue {
            int minutes;
            int seconds;
            int hundredths;
        };        

        enum StopwatchState { READY, RUNNING, STOPPED };
        StopwatchState getStopwatchState();

        // Override CrowController methods to handle user input events
        virtual void onClick() override;        
        virtual void loop() override;

    private:
        StopwatchController();        
        void updateDisplay();        
        
        StopwatchValue time;
        StopwatchState state = READY;        
        unsigned long startTimeMillis = 0;
    };

#endif
