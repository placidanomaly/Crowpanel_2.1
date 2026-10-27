#ifndef DEBUGHELPER_H
#define DEBUGHELPER_H

#include <Arduino.h>

// This class provides a way to output debugging messages using the C++ stream
// syntax. It can also be set to suppress all debugging output to reduce CPU load.
// It can also direct all output to another stream device (e.g. file, Ethernet, etc.)

// Usage example:
//
// void setup() {
//
// 		// Initialize Serial object
//      Serial.begin(115200);
//
//		// Initialize debug object by activating debugging output and leaving
//      // the output stream to the default (Serial)
//		debug.begin(true);
//
//		// Write debugging message and send newline
//		debug << "Debugging started" << endl;
//
//		// Write a more complex message
//		debug << "[BOOT] ESP32 Arduino " << ESP_ARDUINO_VERSION_STR 
//		  << ", LVGL " << LVGL_VERSION_MAJOR << "." << LVGL_VERSION_MINOR 
//		  << "." << LVGL_VERSION_PATCH << "reset_reason=" << (int)esp_reset_reason() << endl;
// }

class DebugHelper {
    public:
		void begin(bool debugEnabled = false, Stream *outputStream = NULL) {
			getInstance()->setDebugEnabled(debugEnabled);
			getInstance()->setOutputStream(outputStream);
		}
			
        inline static DebugHelper *getInstance() {
            return instance == NULL ? new DebugHelper() : instance;
        }
        
        inline void printf(const char *msg, ...) { if(debugEnabled) { char buffer[256]; va_list args; va_start(args, msg); vsnprintf(buffer, sizeof(buffer), msg, args); va_end(args); print(buffer); } }
        inline void println(const char *msg) { if(debugEnabled) printf("%s\n", msg); }
        template<class T>
        inline void print(T arg) { if(debugEnabled) stream->print(arg); }        

        inline void setDebugEnabled(bool debugEnabled) { this->debugEnabled = debugEnabled; }
        inline bool getDebugEnabled() { return debugEnabled; }
		inline void setOutputStream(Stream *outputStream) { this->stream = outputStream == NULL ? &Serial : outputStream; }
        inline Stream *getStream() { return stream; }

    private:
        DebugHelper() { instance = this; }

        inline static DebugHelper *instance = NULL;
        
        bool debugEnabled;
        Stream *stream;
};

#define debug (*DebugHelper::getInstance())

template<class T> inline DebugHelper &operator <<(DebugHelper &helper, T arg) { 
    helper.print(arg);
    return helper; 
}

enum StreamModifiers { endl };
inline DebugHelper &operator <<(DebugHelper &helper, StreamModifiers) { 
    helper.printf("\n"); 
    return helper; 
}

#endif // DEBUGHELPER_H
