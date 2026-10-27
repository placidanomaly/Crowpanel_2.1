/*
MIT License

Copyright (c) 2026 Charles Vaillancourt charles.vaillancourt@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "crowpanel_2_1.h"

void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

Crowpanel& Crowpanel::getInstance() {
    if (!globalInstance) {
        globalInstance = new Crowpanel();
    }
    return *globalInstance;
}

Crowpanel::Crowpanel() {  
}

void Crowpanel::begin() {       

    tsPanel = new Adafruit_CST8XX();

    panelInitBus = new Arduino_SWSPI(
      GFX_NOT_DEFINED /* DC: ST7701 uses 9-bit SPI */, 16 /* CS */,
      2 /* SCK */, 1 /* SDA */, GFX_NOT_DEFINED /* MISO */);

    rgbPanel = new Arduino_ESP32RGBPanel(
      40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
      46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
      14 /* G0/P22 */, 13 /* G1/P23 */, 12 /* G2/P24 */, 11 /* G3/P25 */, 10 /* G4/P26 */, 9 /* G5 */,
      5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
      1 /* hsync polarity */, 10 /* front porch */, 4 /* pulse width */, 20 /* back porch */,
      1 /* vsync polarity */, 10 /* front porch */, 4 /* pulse width */, 20 /* back porch */,
      0 /* pclk active neg */, 12000000 /* pixel clock */, false /* native endian */,
      0 /* de idle high */, 0 /* pclk idle high */,
      480 * 20 /* two internal-DMA bounce buffers, 20 lines each */);

    gfx = new Arduino_RGB_Display(
      480 /* width */, 480 /* height */, rgbPanel, 0 /* rotation */, true /* auto flush */,
      panelInitBus, GFX_NOT_DEFINED /* RST */,
      st7701_type5_init_operations, sizeof(st7701_type5_init_operations));
   
    delay(100);
    debug << "[BOOT] ESP32 Arduino " << ESP_ARDUINO_VERSION_STR 
		  << ", LVGL " << LVGL_VERSION_MAJOR << "." << LVGL_VERSION_MINOR 
		  << "." << LVGL_VERSION_PATCH << "reset_reason=" << (int)esp_reset_reason() << endl;
    debug << "[BOOT] Crowpanel initialization starting" << endl;                    
	
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    debug << "[BOOT] I2C initialized\n" << endl;
    
    pcf8574.pinMode(P0, OUTPUT);        //tp RST
    pcf8574.pinMode(P2, OUTPUT);        //tp INT
    pcf8574.pinMode(P3, OUTPUT);        //lcd power
    pcf8574.pinMode(P4, OUTPUT);        //lcd reset
    pcf8574.pinMode(P5, INPUT_PULLUP);  //encoder SW

    // Set PWM mode for backlight light
    ledcAttachChannel(SCREEN_BACKLIGHT_PIN, pwmFreq, pwmResolution, 0);
    setBacklight();

    // Set breath
    ledcAttachChannel(BREATH_LED_PIN, pwmFreq, pwmResolution, 1);
    setBreathLEDIntensity();

    debug << "Init pcf8574... ";
    if (pcf8574.begin()) {
        debug << "pcf8574 OK";
    } else {
        debug << "pcf8574 KO";
    }
	debug << endl;

    // Initialize LCD and touchscreen
    setLCDPower(ON);
    resetLCD();
    resetTouchscreen();
    
    gfx->begin();
    
    // The new SquareLine LVGL 9 assets are native RGB565. Keep MADCTL's BGR
    // bit set for this ST7701 panel so blue (for example 0x0294) stays blue.
    panelInitBus->beginWrite();
    panelInitBus->writeCommand(0x36);
    panelInitBus->write(0x00);
    panelInitBus->endWrite();

    if (!tsPanel->begin(&Wire, I2C_TOUCH_ADDR)) {
        debug << "No touchscreen found" << endl;
    } else {
        debug << "Touchscreen found" << endl;
    }

    pinMode(ENCODER_A_PIN, INPUT);
    pinMode(ENCODER_B_PIN, INPUT);

    // Initialize LVGL which we need to process touchscreen events
    lv_init();
    lv_tick_set_cb(millis);
    debug << "[BOOT] Initializing LVGL with screen size " 
		  << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << endl;
    size_t buffer_size = sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT;
    buf1 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    buf2 = (uint8_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!buf1)
        debug << "Failed to allocate for LVGL---1---" << endl;
    if (!buf2)
        debug << "Failed to allocate for LVGL---2---" << endl;
    if (!buf1 || !buf2) {
        debug << "[FATAL] LVGL framebuffer allocation failed" << endl;
        while (true) delay(1000);
    }

    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    
    lv_display_set_flush_cb(display, my_disp_flush);
    /* Only copy invalidated regions. Full rendering copied all 480x480 pixels
        for every touch-state change and caused visible tearing on the RGB panel. */
    lv_display_set_buffers(display, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    delay(200);
    setBacklight();
    pcf8574.digitalWrite(P3, LOW);

    encoderActionQueue = xQueueCreate(16, sizeof(EncoderAction));
    if (!encoderActionQueue) {
        debug << "[FATAL] Failed to create encoder queue" << endl;
    }
    xTaskCreatePinnedToCore(encTaskSafe, "ENC", 2048, this, 1, NULL, 0);

    // Enable touchscreen
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touchpad_read);
    lv_indev_set_display(touch_indev, display);

    // This task decouples reading the touchscreen from the 
    // display and UI updates to prevent issues with screen
    // refreshes during dragging.
    xTaskCreatePinnedToCore(
            touch_hardware_task,   // Function pointer
            "Touch_HW_Task",       // Task Name
            3072,                  // Stack size
            this,                  // Parameter
            4,                     // Priority
            NULL,                  // Task handle
            0                      // Core ID (0)
        );
}

void Crowpanel::setDisplayRotation(DisplayRotation rotationFlag) {
  rotation = rotationFlag;
  gfx->setRotation(rotation);
}

/*---------------------------------------------------------------
 * LVGL display output
 * Convert LVGL's RGB565 output to the panel's channel order and
 * copy only the invalidated rectangle to the screen.
 *--------------------------------------------------------------*/

/**
 * @brief Flush an LVGL drawing area to the RGB display.
 *
 * Swaps the red and blue RGB565 fields required by this ST7701 wiring,
 * writes the resulting pixels, and then releases LVGL's draw buffer.
 *
 * @param display LVGL display that requested the flush operation.
 * @param area Inclusive rectangle that must be transferred.
 * @param px_map RGB565 pixel data produced by LVGL.
 * @return void
 * @note Called automatically by LVGL whenever an invalidated area is ready.
 */
void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    Crowpanel::getInstance().getDisplayGfx()->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

    lv_display_flush_ready(display);
}


// The touch hardware loop must be launched by the main program and
// pinned to Core 0 to prevent touchscreen events from disrupting
// display updates, potentially causing the entire display to shift
// or causing screen tearing.
void Crowpanel::touch_hardware_task(void *pvParameters) {
    Crowpanel *panel = (Crowpanel *)pvParameters;
    while(1) {
        // 1. Lower your I2C checking rate to 30ms to prevent bus clogging
        vTaskDelay(pdMS_TO_TICKS(30));
        
        if(panel->tsPanel->touched()) {
          CST_TS_Point p = panel->tsPanel->getPoint(0);
          /* CST8XX coordinates can move by a few pixels while a finger is held
            still. Keep the last point for tiny moves so LVGL doesn't redraw a
            pressed object on every noisy sample. */
          static int16_t stableX = -1;
          static int16_t stableY = -1;
          const int16_t touchX = constrain(p.x, 0, SCREEN_WIDTH - 1);
          const int16_t touchY = constrain(p.y - 20, 0, SCREEN_HEIGHT - 1);
          if (stableX < 0 || abs(touchX - stableX) >= 4 || abs(touchY - stableY) >= 4) {
            stableX = touchX;
            stableY = touchY;
            panel->shared_touch_x.store(stableX, std::memory_order_relaxed);
            panel->shared_touch_y.store(stableY, std::memory_order_relaxed);
          }
          panel->shared_is_pressed.store(true, std::memory_order_relaxed);

    
          debug << "[TOUCH] x=" << stableX << " y=" << stableY << endl;
        }
        else
        {
          panel->shared_is_pressed.store(false, std::memory_order_relaxed);
        }
    }
}

/**
 * @brief Read the touch controller and provide pointer data to LVGL.
 *
 * @param indev LVGL input device requesting a new sample.
 * @param data Output structure receiving pointer coordinates and state.
 * @return void
 * @note Called periodically by LVGL while lv_timer_handler() is running.
 */
void Crowpanel::my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    Crowpanel *panel = Crowpanel::globalInstance;

    if(panel->shared_is_pressed.load(std::memory_order_relaxed)) {
      data->point.x = panel->shared_touch_x.load(std::memory_order_relaxed);
      data->point.y = panel->shared_touch_y.load(std::memory_order_relaxed);

      switch(panel->rotation) {
        case 3: // 90 counter-clockwise
          {
            int32_t temp = data->point.x;
            data->point.x = SCREEN_HEIGHT - data->point.y;
            data->point.y = temp;
          }
          break;
        case 2: // 180 degrees
          data->point.x = SCREEN_WIDTH - data->point.x;
          data->point.y = SCREEN_HEIGHT - data->point.y;
          break;
        case 1: // 90 clockwise
          {
            int32_t temp = data->point.x;
            data->point.x = data->point.y;
            data->point.y = SCREEN_WIDTH - temp;
          }
          break;
      }
      data->state = LV_INDEV_STATE_PR;
    }
    else
    {
      data->state = LV_INDEV_STATE_REL;      
      panel->first_click = false;
      panel->trackingSwipe = false;
      panel->swipeHandled = false;
    }
}

void Crowpanel::setBacklight(uint8_t intensity) {  
  ledcWrite(SCREEN_BACKLIGHT_PIN, intensity);
}

void Crowpanel::setBreathLEDIntensity(uint8_t intensity) {
  ledcWrite(BREATH_LED_PIN, intensity);
}

void Crowpanel::setLongPressDelayMS(uint32_t longPressDelay) {
  this->longPressTime = longPressDelay;
}

void Crowpanel::setDoubleClickDelayMS(uint32_t doubleClickDelay) {
  this->doubleClickTime = doubleClickDelay;
}

void Crowpanel::registerClickActionCallback(void (*clickActionCB)()) {
    performClickAction = clickActionCB;
}

void Crowpanel::registerLongPressActionCallback(void (*longPressActionCB)()) {
    performLongPressAction = longPressActionCB;
}

void Crowpanel::registerDoubleClickActionCallback(void (*doubleClickActionCB)()) {
    performDoubleClickAction = doubleClickActionCB;
};

void Crowpanel::registerRawEncoderActionCallback(void (*encoderActionCB)(EncoderAction&)) {
    handleRawEncoderAction = encoderActionCB;
}

void Crowpanel::registerEncoderRotationActionCallback(void (*encoderActionCB)(int), bool accelerated) {
    performEncoderRotationAction = encoderActionCB;
    this->acceleratedRotation = accelerated;
}

void Crowpanel::handleEncoderAction(EncoderAction &action) {  
  if(handleRawEncoderAction != NULL)
    handleRawEncoderAction(action);

  const EncoderActionType type = action.type;

  if (type == ENCODER_CLICK) {

    debug << "[ENC] Click" << endl;

    if(performClickAction != NULL) performClickAction();    
    return;
  }
  if (type == ENCODER_LONG_PRESS) {

    debug << "[ENC] Long press"	<< endl;

    if(performLongPressAction != NULL) performLongPressAction();    
    return;
  }
  if (type == ENCODER_DOUBLE_CLICK) {
    if (action.rotationSequence != encoderRotationSequence) {
      
      debug << "[ENC] Stale double click cancelled by rotation" << endl;

      return;
    }

    debug << "[ENC] Double-click" << endl;

    if(performDoubleClickAction != NULL) performDoubleClickAction();
    performDoubleClickAction();
    return;
  }

  const int delta = (type == ENCODER_ROTATE_CW) ? 1 : -1;

  debug << "[ENC] Rotation" << endl;


  static unsigned long lastRotation = 0;
  if(performEncoderRotationAction != NULL) {
    const unsigned long now = millis();
    int accelerationFactor = 1;    
    
    if(now - lastRotation < 600) accelerationFactor = 2;
    if(now - lastRotation < 300) accelerationFactor = 3;

    debug << "[ENC] Rotation action " << delta * accelerationFactor 
		  << "(" << now-lastRotation << "msms since last rotation)" << endl;

    lastRotation = now;
    performEncoderRotationAction(delta * accelerationFactor);
  }
}

/**
 * @brief Sample the encoder rotation pins in a background task.
 *
 * Detects rising edges, determines direction from the second phase, and
 * queues a compact action without calling LVGL from the background core.
 *
 * @param pvParameters Unused FreeRTOS task parameter.
 * @return void; this task runs indefinitely.
 * @note Started once from setup() and scheduled by FreeRTOS.
 */
void Crowpanel::encTaskSafe(void *pvParameters) {
    Crowpanel *crowpanel = static_cast<Crowpanel *>(pvParameters);
    int previousA = digitalRead(ENCODER_A_PIN);
    while (true) {
        const int currentAState = digitalRead(ENCODER_A_PIN);
        if (currentAState != previousA && currentAState == HIGH && crowpanel->encoderActionQueue) {
          crowpanel->encoderRotationSequence = crowpanel->encoderRotationSequence + 1; // ++ on volatile is deprecated
          EncoderAction action;
          action.type = (digitalRead(ENCODER_B_PIN) != currentAState)
                          ? ENCODER_ROTATE_CCW : ENCODER_ROTATE_CW;
          action.rotationSequence = crowpanel->encoderRotationSequence;
          xQueueSend(crowpanel->encoderActionQueue, &action, 0);
        }
        previousA = currentAState;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/**
 * @brief Poll and debounce the encoder push button.
 *
 * Accepts only a debounced press followed by a debounced release. Any encoder
 * rotation during that gesture or its double-click decision window cancels
 * the pending click, preventing rotary contact noise from opening a screen.
 *
 * @param none.
 * @return void.
 * @note Called on every pass through loop().
 */
void Crowpanel::pollEncoderButton() {
  static uint8_t rawState = HIGH;
  static uint8_t stableState = HIGH;
  static unsigned long rawChangedMs = 0;
  static unsigned long pressedMs = 0;
  static bool longPressDispatched = false;
  static uint32_t pressRotationSequence = 0;
  static uint32_t pendingClickRotationSequence = 0;
  const uint8_t sampledState = pcf8574.digitalRead(P5, true);
  const unsigned long now = millis();

  if (sampledState != rawState) {
    rawState = sampledState;
    rawChangedMs = now;

    debug << "[ENC] P5 state changed" << endl;
  }

  if (rawState != stableState && now - rawChangedMs >= debounceTime) {    
    stableState = rawState;
    if (stableState == LOW) {      
      pressedMs = now;
      pressRotationSequence = encoderRotationSequence;
      longPressDispatched = false;
    } else {
      const bool completedWithoutRotation =
          pressedMs != 0 && encoderRotationSequence == pressRotationSequence;
      pressedMs = 0;
      if (completedWithoutRotation) {
        if (pressCount == 0) pendingClickRotationSequence = encoderRotationSequence;
        pressCount = pressCount + 1;        
        singleClickTimeout = now + doubleClickTime;
      } else {
        pressCount = 0;
      }
    }
  }

  // Rotation is selection only; it invalidates every pending confirmation.
  if (pressCount > 0 && encoderRotationSequence != pendingClickRotationSequence) {
    pressCount = 0;
  }

  EncoderAction action;
  bool ready = false;
  if(!longPressDispatched && (stableState == LOW) && (now - pressedMs > longPressTime)) {
    action.type = ENCODER_LONG_PRESS;
    action.rotationSequence = pendingClickRotationSequence;
    longPressDispatched = true;
    ready = true; 
    pressCount = -1; // So a CLICK event won't be generated when the button is released
  } else if (pressCount >= 2) {
    pressCount = 0;
    action.type = ENCODER_DOUBLE_CLICK;
    action.rotationSequence = pendingClickRotationSequence;
    ready = true;
  } else if (pressCount == 1 && (((long)(now - singleClickTimeout) >= 0) || (performDoubleClickAction == NULL))) {    
    pressCount = 0;
    action.type = ENCODER_CLICK;
    action.rotationSequence = pendingClickRotationSequence;
    ready = true;
  }
  
  if (ready && encoderActionQueue) xQueueSend(encoderActionQueue, &action, 0);
}

Arduino_RGB_Display *Crowpanel::getDisplayGfx() {
    return gfx;
}

QueueHandle_t &Crowpanel::getEncoderActionQueue() {
    return encoderActionQueue;
}

void Crowpanel::setLCDPower(DisplayPowerState state) {
  pcf8574.digitalWrite(P3, state);  
  delay(100);
}

void Crowpanel::resetLCD() {
  pcf8574.digitalWrite(P4, HIGH);
  delay(100);
  pcf8574.digitalWrite(P4, LOW);
  delay(120);
  pcf8574.digitalWrite(P4, HIGH);
  delay(120);
}

void Crowpanel::resetTouchscreen() {
  pcf8574.digitalWrite(P0, HIGH);
  delay(100);
  pcf8574.digitalWrite(P0, LOW);
  delay(120);
  pcf8574.digitalWrite(P0, HIGH);
  delay(120);
  
  /*tp INT*/
  pcf8574.digitalWrite(P2, HIGH);
  delay(120);
}

void Crowpanel::loop() {
    pollEncoderButton();
    Crowpanel::EncoderAction action;
    while(encoderActionQueue && xQueueReceive(encoderActionQueue, &action, 0) == pdTRUE) {

        debug << "[ENC] Action dequeued: type=" << action.type << ", rotationSequence=" << action.rotationSequence << endl;

        handleEncoderAction(action);
    }
}
