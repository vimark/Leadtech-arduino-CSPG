//Hardware pin assignment on Gizduino X board

//#define USING_VENDOR_HARDWARE //if using vendor hardware instead of client hardware, the two hardware have different wiring
#define DEBUG_MODE //censor some info e.g. keys

#define USING_SELF_OSC_BUZZER

#define PIN_SENSE_CURRENT A0
#define ADC_MIN 240
#define ADC_MAX 720
#define Vmin 0 //1182
#define Vmax 1543 //3425

#ifdef USING_VENDOR_HARDWARE
#define PIN_WAKE 45 // vendor hardware, screen wake pin
#else
#define PIN_WAKE 2 // client hardware, screen wake pin
#endif

#define DISPLAY_CLK 21
#define DISPLAY_SDA 20
