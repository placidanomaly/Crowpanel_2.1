#include "stopwatchcontroller.h"
#include "ui/ui.h"

// Link SquareLine-generated callbacks to our CrowController timer object
extern "C" void stopwatchClicked(lv_event_t *e) { StopwatchController::getInstance().click(); }

StopwatchController& StopwatchController::getInstance() {
    static StopwatchController *instance = NULL;

    if(instance == NULL) instance = new StopwatchController();
    
    return *static_cast<StopwatchController *>(instance);
}

void StopwatchController::begin(Crowpanel &panel) {
    debug << "[STOPWATCH] Initializing..." << endl;
    CrowController::begin(panel);
}

void StopwatchController::start() {
    if(state != READY) return;

    lv_label_set_text(ui_stopwatchButtonLabel, "STOP");
    state = RUNNING;
    startTimeMillis = millis();
}

void StopwatchController::stop() {
    if(state != RUNNING) return;

    lv_label_set_text(ui_stopwatchButtonLabel, "RESET");
    state = STOPPED;
}

void StopwatchController::reset() {
    if(state != STOPPED) return;

    time = {0, 0, 0};
    updateDisplay();

    lv_label_set_text(ui_stopwatchButtonLabel, "START");
    state = READY;
}

StopwatchController::StopwatchState StopwatchController::getStopwatchState() {
    return state;
}

void StopwatchController::click() {
    switch(state) {
        case READY: start(); break;
        case RUNNING: stop(); break;
        case STOPPED: reset(); break;
    }
}

StopwatchController::StopwatchController() {}

void StopwatchController::updateDisplay() {    
    char buf[12];
    sprintf(buf, "%d:%02d", time.minutes, time.seconds);
    lv_label_set_text(ui_stopwatchLabel, buf);
    sprintf(buf, "%02d", time.hundredths);
    lv_label_set_text(ui_hundredthsLabel, buf);    
}

void StopwatchController::loop() {
    if(!isActivated()) return;

    CrowController::loop();

    if(state == RUNNING) {

        static unsigned long previousElapsed = 0;
        unsigned long elapsedMillis = millis() - startTimeMillis;
        if(elapsedMillis / 10 == previousElapsed / 10) return; // No need to update the display if there's less than 1/100 second since last update
        previousElapsed = elapsedMillis;

        time.minutes = int(elapsedMillis / 60000.0f);
        time.seconds = (elapsedMillis - time.minutes * 60000) / 1000.0f;
        time.hundredths = (elapsedMillis % 1000) / 10;        
        
        // Prevent our counter from exceeding the limit of our 2-digit display
        if(time.minutes > 99) {
            time.hundredths = 99;
            stop();            
        } else {
            updateDisplay();
        }
    }
}
