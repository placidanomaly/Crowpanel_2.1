
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

  // Set the USB TX timeout to 0 so it doesn't block waiting when unplugged
  Serial.setTxTimeoutMs(0);
  Serial.begin(115200);

  // Enable output of debug messages to Serial (set to false for prod)
  debug.begin(true);
  
  debug << "[BOOT] Initializing" << endl;

  // Initialize Crowpanel hardweare
  panel.begin();

  // Initialize UI
  ui_init();

  // Initialize our controllers
  stopwatch.begin(panel);
  timer.begin(panel);
  
  // Cause device to enter deep sleep if inactive for 2 minutes
  // while in countdown timer mode (will not go to sleep in stopwatch
  // mode unless the same command is issued against that controller)
  timer.setDeepSleepTimeout(120);
  
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
  // Note that there is no need to call panel.loop() because
  // the activeController object will do it in its own loop()
  // function
  activeController->loop();

  // Let MCU breath a bit
  vTaskDelay(pdMS_TO_TICKS(15)); 
}
