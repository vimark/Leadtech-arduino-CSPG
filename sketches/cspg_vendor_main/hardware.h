//Hardware pin assignment on Gizduino

//#define USING_VENDOR_HARDWARE //if using vendor hardware instead of client hardware, the two hardware have different wiring
#define USING_SELF_OSC_BUZZER
//#define DEBUG_MODE //censor some info e.g. keys

#ifdef USING_VENDOR_HARDWARE
#define PIN_WAKE 45 // vendor hardware, screen wake pin
#else
#define PIN_WAKE 2 // client hardware, screen wake pin
#endif

//RTC
#define DISPLAY_CLK 21
#define DISPLAY_SDA 20

//printer
#define TX_PIN 3 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 2 // Arduino receive   GREEN WIRE   labeled TX on printer
