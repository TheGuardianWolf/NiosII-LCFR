#include <stdint.h>

#define timestamp() Event_getTimestamp()//xTaskGetTickCount() * portTICK_PERIOD_MS
#define timestampFromISR() Event_getTimestampFromISR()//xTaskGetTickCountFromISR() * portTICK_PERIOD_MS

#define EVENT_LOAD_MANAGER_GRACE_EXPIRED 255
#define EVENT_FREQUENCY_ANALYZER_STABLE 254
#define EVENT_FREQUENCY_ANALYZER_UNSTABLE 253
#define EVENT_BUTTON_PRESSED 252
#define EVENT_SWITCH_ON(x) (247 + x)
#define EVENT_SWITCH_OFF(x) (242 + x)

typedef struct {
    uint64_t timestamp;
    uint8_t code;
} Event;

void Event_start();

uint64_t Event_getTimestamp();

uint64_t Event_getTimestampFromISR();

