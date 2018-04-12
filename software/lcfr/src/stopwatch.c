#include "freertos/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

uint32_t startTime = 0;
uint32_t stopTime = 0;
bool isRunning = false;


void Stopwatch_start(bool inISR) {
    isRunning = true;
    startTime = ((inISR) ? xTaskGetTickCountFromISR() : xTaskGetTickCount()) * portTICK_PERIOD_MS;
}

bool Stopwatch_isRunning() {
    return isRunning;
}

void Stopwatch_stop(bool inISR) {
    if (isRunning) {
        stopTime = ((inISR) ? xTaskGetTickCountFromISR() : xTaskGetTickCount()) * portTICK_PERIOD_MS;
        isRunning = false;
    }
}

uint32_t Stopwatch_getTimeElapsed(bool inISR) {
    if (isRunning) {
        return ((inISR) ? xTaskGetTickCountFromISR() : xTaskGetTickCount()) * portTICK_PERIOD_MS - startTime;
    }
    return stopTime - startTime;
}

void Stopwatch_reset(bool inISR) {
    startTime = ((inISR) ? xTaskGetTickCountFromISR() : xTaskGetTickCount());
    stopTime = startTime;
}
