#ifndef TIMERCONTROLLER_H
#define TIMERCONTROLLER_H

#include <crowpanel_2_1.h>

class TimerController: public CrowController {
    public:        
        static TimerController& getInstance();

        virtual void begin(Crowpanel &panel);
    
        void startTimer();
        void stopTimer();

        struct TimerValue {
            int minutes;
            int seconds;
        };

        TimerValue &getInitialTimerValue();

        enum TimerState { STOPPED, RUNNING, FINISHED };
        TimerState getTimerState();

        // Override CrowController methods to handle user input events
        virtual void click() override;
        virtual void encoderRotation(int delta) override;

        // Functions to handle saving and restoring state across deep sleep cycles
        virtual void saveStateBeforeDeepSleep() override;
        virtual void restoreStateAfterDeepSleep() override;

    private:
        TimerController();        
        void setDisplayValue(TimerValue &value);
        void tick();
        
        TimerValue timerSetting = {0, 0};
        TimerValue currentTimerValue = {0, 0};        
        lv_timer_t *timer = NULL;

        TimerState state = STOPPED;
        void setState(TimerState newState);
    };

#endif
