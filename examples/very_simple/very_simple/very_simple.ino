// A minimal program that uses the Crowpanel_2.1 library

#include <crowpanel_2_1.h>

// Pointer to Crowpanel controller object
Crowpanel& panel = Crowpanel::getInstance();

// Forward declaration of our callback functions
void clickCallback();

void setup() {

  Serial.begin(115200);

  // Initialize our panel object
  panel.begin();

  // Register a callback function that will respond to a click on the encoder
  panel.registerClickActionCallback(clickCallback);

  // Write a short text message on the display using LVGL
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text_static(label, "Hello Crowpanel 2.1!");
  lv_obj_center(label);
}

void clickCallback() {
  // Display a message on the serial output when the encoder is clicked
  // (Use Tools->Serial Monitor in Arduino IDE to see the message)

  Serial.println("Rotary encoder clicked");
}

void loop() {
  panel.loop();

  // LVGL event loop
  lv_timer_handler();

  // Let MCU breath a bit
  vTaskDelay(pdMS_TO_TICKS(15)); 
}
