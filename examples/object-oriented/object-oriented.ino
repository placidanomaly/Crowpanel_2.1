// A minimal program that uses the Crowpanel_2.1 library

#include <crowpanel_2_1.h>
#include "src/timercontroller.h"
#include "src/ui/ui.h"

// Get a hold of our CrowPanel hardware object
Crowpanel &panel = Crowpanel::getInstance();

// Create a reference to our CrowController timer object
TimerController &timer = TimerController::getInstance();

void setup() {

  Serial.begin(115200);
  debug.begin(true); // Enable output of debug messages to Serial
    
  debug << "Initializing" << endl;

  // Initialize Crowpanel hardweare
  panel.begin();

  // Initialize UI
  ui_init();

  // Initialize our panel object
  timer.begin(panel);
  timer.setDeepSleepTimeout(120);

  // Activate events on our controller
  timer.activate();

  debug << "Timer program initialized." << endl;
}

void loop() {
  timer.loop();

  // Let MCU breath a bit
  vTaskDelay(pdMS_TO_TICKS(15)); 
}
