#include "freertos/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

uint32_t startTime = 0;
uint32_t stopTime = 0;
bool isRunning = false;

static QueueHandle_t xReactionTimeQueue;
static SemaphoreHandle_t xReactionTimeMutex;

static ReactionTimes reactionTimes = {
	.min = UINT_MAX,
	.max = 0,
	.average = 0,
	.averageSamples = 0
}

void Reactions_start() {
    xReactionTimeQueue = xQueueCreate(16, sizeof(uint32_t));
    // xReactionTimeMutex = xSemaphoreCreateMutex(xReactionTimeMutex);
}

void Reactions_reportStartTime() {
	uint32_t currTime = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
	xQueueSend(xReactionTimeQueue, &currTime, 0);
}

void Reactions_reportStartTimeFromISR() {
	uint32_t currTime = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
	xQueueSendFromISR(xReactionTimeQueue, &currTime, NULL);
}

void Reactions_discardNext() {
	xQueueReceive(xReactionTimeQueue, NULL, 0);
}

void Reactions_registerReaction() {
	uint32_t tStop = xTaskGetTickCount() * portTICK_PERIOD_MS;
	uint32_t tStart;
	if (xQueueReceive(xReactionTimeQueue, &t, 0) == pdTRUE) {
		uint32_t t = tStop - tStart;
		// xSemaphoreTake(xReactionTimeMutex, portMAX_DELAY);
		if (t < reactionTimes.min) {
			reactionTimes.min = t;
		}

		if (t > reactionTimes.max) {
			reactionTimes.max = t;
		}

		reactionTimes.average = (reactionTimes.average * reactionTimes.averageSamples + t) / (reactionTimes.averageSamples + 1)
		reactionTimes.averageSamples++;
		// xSemaphoreGive(xReactionTimeMutex);
	}
}

ReactionTimes Reactions_getReactionTimes() {
	// xSemaphoreTake(xReactionTimeMutex, portMAX_DELAY);
	ReactionTimes retVal = reactionTimes;
	// xSemaphoreGive(xReactionTimeMutex);
	return retVal;
}
