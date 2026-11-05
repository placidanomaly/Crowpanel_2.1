// A minimal program that uses the Crowpanel_2.1 library

#include <crowpanel_2_1.h>
#include "src/timercontroller.h"
#include "src/stopwatchcontroller.h"
#include "src/ui/ui.h"

// Get a hold of our CrowPanel hardware object
Crowpanel &panel = Crowpanel::getInstance();

// Create a reference to our CrowController objects
TimerController &timer = TimerController::getInstance();
StopwatchController &stopwatch = StopwatchController::getInstance();

CrowController *activeController = NULL;

void setup() {

  Serial.begin(115200);
  debug.begin(true); // Enable output of debug messages to Serial
  
  debug << "[BOOT] Initializing" << endl;

  // Initialize Crowpanel hardweare
  panel.begin();

  // Initialize UI
  ui_init();

  // Initialize our controllers
  stopwatch.begin(panel);
  timer.begin(panel);
  timer.setDeepSleepTimeout(120);

  // If we woke up from deep sleep, restore the controller state
  // that was set at the time we went to sleep
  if(panel.wokeUpFromDeepSleep()) timer.restoreStateAfterDeepSleep();
  
  // Activate the timer controller
  setActiveController(timer);

  // Cause a long press on the encoder to switch mode
  panel.registerLongPressActionCallback(longPressHandler);

  debug << "[BOOT] Timer program initialized." << endl;
}

void setActiveController(CrowController& controller) {
  if(activeController == &controller) return;

  if(activeController != NULL) activeController->deactivate();

  if(&controller == &timer)
    lv_scr_load(ui_timerScreen);
  else if(&controller == &stopwatch)
    lv_scr_load(ui_stopwatchScreen);
  
  activeController = &controller;
  activeController->activate();
}

void longPressHandler() {
  if(activeController == &timer) 
    setActiveController(stopwatch);
  else
    setActiveController(timer);
}

void loop() {
  activeController->loop();

  // Let MCU breath a bit
  vTaskDelay(pdMS_TO_TICKS(15)); 
}
