#include "timercontroller.h"
#include <Preferences.h>
#include "ui/ui.h"

#define PREFS_PAGE              "timer"
#define PREFS_TIMER_MIN_KEY     "minutes"
#define PREFS_TIMER_SEC_KEY     "seconds"

// Link SquareLine-generated callbacks to our CrowController timer object
extern "C" void startStopClicked(lv_event_t *e) { TimerController::getInstance().onClick(); }

// RTC variable for saving current timer value across deep sleep cycles
RTC_DATA_ATTR TimerController::TimerValue sleepTimerValue = {0, 0};

TimerController& TimerController::getInstance() {
    static TimerController *instance = NULL;

    if(instance == NULL) instance = new TimerController();
    
    return *static_cast<TimerController *>(instance);
}

TimerController::TimerController() {
    // Defer initialization to begin() method
}

void TimerController::begin(Crowpanel &panel) {
    debug << "[TIMER] Initializing..." << endl;
    CrowController::begin(panel);

    // Retrieve saved timer values from preferences, or set to 0:00
    Preferences prefs;
    prefs.begin(PREFS_PAGE, true);
    timerSetting.minutes = prefs.getInt(PREFS_TIMER_MIN_KEY, 0);
    timerSetting.seconds = prefs.getInt(PREFS_TIMER_SEC_KEY, 0);
    prefs.end();
    debug << "[TIMER] timerSetting retrieved or initialized (" << timerSetting.minutes << ":" << timerSetting.seconds << ")" << endl;

    updateDisplay();
}

TimerController::TimerValue &TimerController::getInitialTimerValue() {
    return timerSetting;
}

void TimerController::activate() {
    CrowController::activate();

    updateDisplay();
}

void TimerController::updateDisplay() {

    // If the controller is not activated, the program is displaying
    // another screen, so no need to update the value
    if(!isActivated()) return;

    static TimerValue *value;
    switch(state) {
        case STOPPED: value = &timerSetting; break;
        case RUNNING: value = &currentTimerValue; break;
        case FINISHED: value = &currentTimerValue; break;
    }
    
    lv_async_call([](void *data){
        TimerValue value = *(TimerValue *)data;
        char buf[6] = {0, 0, 0, 0, 0, 0};
        snprintf(buf, sizeof(buf), "%2d:%02d", value.minutes, value.seconds);
        lv_label_set_text(ui_timerLabel, buf);
    }, value);
}

void TimerController::onClick() {
    CrowController::onClick();

    debug << "[TIMER] Click: ";
    switch(state) {
        case STOPPED:
            debug << "starting timer" << endl;
            startTimer();
            break;
        case RUNNING:
            debug << "stopping timer" << endl;
            stopTimer();
            break;
        case FINISHED:            
            debug << "resetting timer" << endl;
            updateDisplay();
            setState(STOPPED);
            break;
    }
}

void TimerController::startTimer() {
    if(state == RUNNING) return; // Already running

    setState(RUNNING);
    debug << "[TIMER] Starting timer" << endl;
    currentTimerValue = getInitialTimerValue(); // Reset to initial value each time we start

    // Start a periodic timer to tick every second
    timer = lv_timer_create([](lv_timer_t *timer){
        TimerController *controller = static_cast<TimerController *>(lv_timer_get_user_data(timer));
                
        controller->tick();
        controller->resetSleepTimeout(); // Prevent from going to sleep while timer is running
    }, 1000, this);
}

void TimerController::stopTimer() {
    if(state != RUNNING) return; // Not running

    debug << "[TIMER] Stopping timer" << endl;
    if(timer) {
        lv_timer_delete(timer);
    }
    
    if(currentTimerValue.minutes == 0 && currentTimerValue.seconds == 0)
        setState(FINISHED); // If the timer has elapsed, let 0:00 on the display until acknowledged
    else
        setState(STOPPED); 
}

void TimerController::tick() {
    debug << "[TIMER] Tick: ";

    if(currentTimerValue.seconds == 0) {
        if(currentTimerValue.minutes > 0) {
            currentTimerValue.minutes--;
            currentTimerValue.seconds = 59;
        }
    } else {
        currentTimerValue.seconds--;
    }
    debug << " " << currentTimerValue.minutes << " minutes " << currentTimerValue.seconds << " seconds." << endl;    
    updateDisplay();

    if((currentTimerValue.minutes == 0) && (currentTimerValue.seconds == 0)) {
        // Timer has finished
        stopTimer();                        
        return;
    }
}

void TimerController::setState(TimerState newState) {
    state = newState;
    
    switch(state) {
        case STOPPED:
            lv_label_set_text(ui_buttonLabel, "START");
            updateDisplay();
            break;
        case RUNNING:
            lv_label_set_text(ui_buttonLabel, "STOP");
            break;
        case FINISHED:
            lv_label_set_text(ui_buttonLabel, "DONE");

            // Timer was successfully completed. Persist the current timer setting.
            debug << "[TIMER] Saving timerSetting (" << timerSetting.minutes << ":" << timerSetting.seconds << ")" << endl;
            Preferences prefs;
            prefs.begin(PREFS_PAGE, false);
            prefs.putInt(PREFS_TIMER_MIN_KEY, timerSetting.minutes);
            prefs.putInt(PREFS_TIMER_SEC_KEY, timerSetting.seconds);
            prefs.end();
            // Leave display at 0:00 until acknowledged            
            break;
    }
}

void TimerController::onEncoderRotation(int delta) {
    CrowController::onEncoderRotation(delta);

    if(state == RUNNING) return; // Don't allow changes while running
    if(state == FINISHED) {
        // Reset to stopped state if user rotates encoder after finishing
        setState(STOPPED);
    }

    debug << "[TIMER] Encoder rotation. Delta=" << delta << endl;
    switch(delta) {
        case 1:
            timerSetting.seconds++;
            if(currentTimerValue.seconds >= 60) {
                timerSetting.seconds = 0;
                timerSetting.minutes++;
            }
            break;
        case -1:
            timerSetting.seconds--;
            if(timerSetting.seconds < 0) {
                if(timerSetting.minutes > 0) {
                    timerSetting.minutes--;
                    timerSetting.seconds = 59;
                } else {
                    timerSetting.seconds = 0; // Don't go below 0:00
                }
                timerSetting.minutes--;                
            }
            break;
        case 2:
            timerSetting.minutes++;
            break;
        case -2:
            timerSetting.minutes--;
            if(timerSetting.minutes < 0) timerSetting.minutes = 0; // Don't go below 0:00
            break;
    }    
    debug << "[TIMER] New timer initial value: " << timerSetting.minutes << ":" << timerSetting.seconds << endl;
    updateDisplay();
}

void TimerController::saveStateBeforeDeepSleep() {    
    sleepTimerValue = timerSetting;
}

void TimerController::restoreStateAfterDeepSleep() {
    timerSetting = sleepTimerValue;    
    setState(STOPPED);
}
