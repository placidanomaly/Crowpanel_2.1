//! Crowpanel_2_1 library
/*! The Crowpanel_2_1 library provides an easy interface to use all the functinality
    of a Crowpanel 2.1" LCD display with touchscreen and rotary encoder
*/
#ifndef CROWPANEL_2_1_H
#define CROWPANEL_2_1_H


//#define DEBUG // Uncomment this line to get verbose debug messages on the output stream

#include <Arduino.h>
#include <esp_system.h>
#include <Adafruit_CST8XX.h>
#include <PCF8574.h> 
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <atomic>

// Hardware pin assignments. These are fixed on the Crowpanel.
#define I2C_SDA_PIN 38
#define I2C_SCL_PIN 39
#define BREATH_LED_PIN 43
#define SCREEN_BACKLIGHT_PIN 6
#define ENCODER_A_PIN 42
#define ENCODER_B_PIN 4

// Touch controller I2C address for the CST8XX capacitive touch panel
#define I2C_TOUCH_ADDR 0x15

// PCF8574 I2C address for the I/O expander that provides reset,
// power, interrupt, and encoder-button I/O.
#define I2C_PCF8574_ADDR 0x21

// Screen dimensions
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480


class Crowpanel {
public:
    /**
     * @brief The Crowpanel object factory
     * 
     * Because there can only ever be a single Crowpanel in a given project,
     * this method will take care of creating a global Crowpanel object when it is called
     * the first time, and will return a pointer to the same object on subsequent
     * invocations (singleton pattern).* 
     * 
     * Usage example:
     * 
     *      Global variable declaration:
     * 
     *          CrowPanel& panel = CrowPanel::getInstance();
     * 
     *      In setup() function:
     * 
     *          panel.begin();
     *          panel.setBrightness(128);
     * 
     * Note: the display starts powered ON, is reset, and set to maximum brightness
     * when initialized.
     * @return A pointer to a Crowpanel object
     */   
    static Crowpanel& getInstance();    

    /**
     * @brief Initializes the panel, display, and all devices
     * 
     * Must be called before using the panel in the setup() function.
     */
    void begin(Stream *stream = NULL);

    /**
     * @brief The event loop for the Crowpanel
     * 
     * This loop function MUST be called from the main Arduino loop() 
     * function to process button and encoder events and invoke the registered 
     * callbacks for encoder clicks and rotations. It also handles touchscreen
     * events and feeds them to LVGL.
     */
    void loop();

    
    enum DisplayPowerState : uint8_t { ON = HIGH, OFF = LOW};
    /**
     * @brief Turns the LCD display ON or Off
     * @param [in] state An optional parameter to specify whether to turn the screen
     * ON or OFF using the values in teh DisplayPowerState enum. If left empty, it
     * will turn on the LCD.
     * 
     * Usage example:
     * 
     *      panel->setLCDPower(Crowpanel::DisplayPowerState::OFF);
     */
    void setLCDPower(DisplayPowerState state = ON);

    /**
     * @brief Resets the LCD
     * 
     * This function sends a square reset signal to pin 3 of the I2C expander in
     * the Crowpanel to reset the LCD. It is called automatically at 
     * initialization and probably never needs to be called again.
     */
    void resetLCD();    

    /**
     * @brief Resets the touchscreen
     * 
     * This function sends a square signal to pin 0 of the I2C expander to
     * reset the touchscreen. It is called automatically at initialization and
     * probably never needs to be called again.
     */
    void resetTouchscreen();

    /**
     * @brief Sets the backlight intensity of the LCD display
     * @param [in] intensity An optional 0-255 value of intensity 
     * which corresponds to the duty cycle of the PWM controller
     * of the backlight, with 255 being 100% intensity. If left
     * empty, the intensity will be set to 255.
     */
    void setBacklight(uint8_t intensity = 255);

    /**
     * @brief Sets the intensity of the LED underneath the rotary control
     * @param [in] intensity An optional 0-255 value of intensity for the
     * rotary control LED. Leaving this empty sets the intensity to 255.
     */
    void setBreathLEDIntensity(uint8_t intensity = 255);

    enum DisplayRotation : int8_t { NO_ROTATION = 0, ROTATION_90_CW, ROTATION_180, ROTATION_90_CCW };
    /**
     * @brief Sets the screen rotation of the LCD display and touchscreen
     * 
     * This controls the rotation of the display. It can be rotated 90, 180, or 270
     * degrees relative to its normal up direction. Touchscreen inputs are also
     * adjusted to match the selected orientation. This is useful if the Crowpanel
     * cannot be mounted with the normal Up direction facing upward.
     * 
     * Usage example:
     * 
     *      panel->setDisplayRotation(Crowpanel::DisplayRotation::ROTATION_90_CW);
     * 
     * @param rotationFlag A value from the DisplayRotation enum to specify the
     * rotation angle desired
     */
    void setDisplayRotation(DisplayRotation rotationFlag);

    /**
     * @brief Sets how long the encoder must be held down to trigger a long press event,
     * which is set to 1000ms by default.
     * @param [in] longPressDelay The delay in milliseconds
     */
    void setLongPressDelayMS(uint32_t longPressDelay);

    /**
     * @brief Sets the maximum delay between the moment the encoder is released
     * and pressed again for it to be considered a double-click (and not 2
     * single clicks). This is set to 300ms by default. Because of this, if
     * a double-click callback function is specified, click events will only
     * be triggered once that delay past the button being released has
     * elapsed (i.e. when we're sure the click was not the first of a double-click)
     * @param [in] doubleClickDelay
     */
    void setDoubleClickDelayMS(uint32_t doubleClickDelay);

    /**
     * @brief Specified a callback function to be invoked when a click event occurs
     * @param [in] clickActionCB A pointer to the callback function
     * 
     * Usage example:
     * 
     *      void clickCallback() {
     *          Serial.println("Click event received");
     *      }
     *      ...
     *      panel.setClickActionCallback(clickCallback);
     */
    void registerClickActionCallback(void (*clickActionCB)());
    
    /**
     * @brief Specified a callback function to be invoked when a long press event occurs
     * @param [in] longPressActionCB A pointer to the callback function
     */
    void registerLongPressActionCallback(void (*longPressActionCB)());
    
    /**
     * @brief Specified a callback function to be invoked when a double-click event occurs
     */
    void registerDoubleClickActionCallback(void (*doubleClickActionCB)());
    
        /**
     * @brief This enum contains the various signals that the encoder can pass
     * to its callback functions.
     */
    enum EncoderActionType : int8_t {
        ENCODER_ROTATE_CW = 1,  /**< The encoder was rotated clockwise */
        ENCODER_ROTATE_CCW,     /**< The encoder was rotated counter-clockwise */
        ENCODER_CLICK,          /**< The encoder was pushed down and released */
        ENCODER_LONG_PRESS,     /**< The encoder was pushed down and held */
        ENCODER_DOUBLE_CLICK    /**< The encoder was pushed down and released twice in quick succession */
    };

    /**
     * @brief This struct contains an encoder event.
     */
    struct EncoderAction {
        EncoderActionType type;     /**< The EncoderActionType of the event */
        uint32_t rotationSequence;  /**< The position of the rotary encoder relative to where it was when it was initialized */
    };

    /**
     * @brief Registers a callback that will receive all encoder events
     * 
     * The callback function must take a reference to an EncoderAction object as parameter.
     * Note that if this callback function is set, it is called IN ADDITION to
     * the click, double-click, long press, and encoder rotation callbacks.
     * 
     * param [in] A pointer to the callback function, which must take a reference
     * to an EncoderAction object as parameter
     */
    void registerRawEncoderActionCallback(void (*encoderActionCB)(EncoderAction&));

    /**
     * @brief Specified a callback function to be invoked when the rotary encoder
     * is moved.
     * 
     * The movement of the encoder is passed as a parameter to the callback function.
     * The value passed is 1 if the encoder was moved clockwise and -1 if moved
     * counter-clockwise.
     * @param [in] encoderActionCB A pointer to the callback function which must take
     * an int as parameter
     */
    void registerEncoderRotationActionCallback(void (*encoderActionCB)(int));

    /**
     * @brief Returns a pointer to the GFX object of the LCD display.
     * @return A pointer to an Arduino_RGB_Display
     */
    Arduino_RGB_Display *getDisplayGfx();

private:
    /**
     * @brief The constructor. It is private because this needs to be a singleton.
     * 
     * The user should never call the constructor and use getInstance() to get a
     * reference to a single global instance. If two objects were to be created,
     * they would interfere with each other and result in unpredictable behavior.
     */
    Crowpanel();    
    
    Stream *debugOutputStream = NULL;
    inline static Crowpanel *globalInstance = NULL; // Pointer to singleton instance

    DisplayRotation rotation = NO_ROTATION;

    // PCF8574 I/O controller embedded in the Crowpanel provides reset, power, interrupt, and encoder-button I/O.
    PCF8574 pcf8574{I2C_PCF8574_ADDR};

    // Timing values used to debounce the encoder button and detect double clicks.
    unsigned long debounceTime = 50;    
    unsigned long doubleClickTime = 300;  
    unsigned long longPressTime = 1000;

    // Shared PWM settings for the screen backlight and the onboard indicator LED.
    const int pwmFreq = 5000;
    const int pwmResolution = 8;

    // Internal rotation encoder state tracking
    QueueHandle_t encoderActionQueue;
    static void encTaskSafe(void *pvParameters);    
    void pollEncoderButton();
    QueueHandle_t &getEncoderActionQueue();
    void handleEncoderAction(EncoderAction &action);        

    // Represents the CST8XX capacitive touch controller.
    Adafruit_CST8XX *tsPanel;
    static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
    static void touch_hardware_task(void *pvParameters);

    // Full-size LVGL draw buffers allocated in external PSRAM during setup().
    uint8_t *buf1 = NULL;
    uint8_t *buf2 = NULL;

    // RGB display driver configuration
    Arduino_DataBus *panelInitBus;
    Arduino_ESP32RGBPanel *rgbPanel;
    Arduino_RGB_Display *gfx;

    // LVGL display object
    lv_display_t *display;

    // Touchscreen gesture thresholds reject slow
    // or strongly vertical movements.
    const int SWIPE_THRESHOLD = 100;  
    const int TIME_THRESHOLD = 300;   
    const int VERTICAL_LIMIT = 100;        

    // Counts completed button clicks until the single/double-click window closes.
    volatile int pressCount;

    /* Incremented by the GPIO sampling task for every rotary step. Button clicks
    * are accepted only if this value stays unchanged for the complete gesture
    * and the following double-click decision window. */
    volatile uint32_t encoderRotationSequence = 0;

    // Marks when a pending single click may be dispatched.
    volatile unsigned long singleClickTimeout = 0; 

    // Touch tracking variables
    int startX = 0;
    int startY = 0;
    unsigned long startTime = 0;
    bool trackingSwipe = false;
    bool first_click = false;
    bool swipeHandled = false;

    // Simple shared state variables
    std::atomic<int> shared_touch_x{0};
    std::atomic<int> shared_touch_y{0};
    std::atomic<bool> shared_is_pressed{false};

    // Callback function pointers for encoder events
    void (*performClickAction)() = NULL;
    void (*performLongPressAction)() = NULL;
    void (*performDoubleClickAction)() = NULL;
    void (*performEncoderRotationAction)(int) = NULL;    
    void (*handleRawEncoderAction)(EncoderAction&) = NULL;
};

#endif // CROWPANEL_2_1_H