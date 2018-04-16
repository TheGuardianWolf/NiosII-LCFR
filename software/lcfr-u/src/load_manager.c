#include <stdio.h>
#include <altera_avalon_pio_regs.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "load_manager.h"
#include "switch.h"
#include "button.h"
#include "frequency_analyzer.h"
#include "system.h"
#include "event.h"

static const TickType_t xGraceTimerFrequency = LOAD_MANAGER_GRACE * portTICK_PERIOD_MS;
static QueueHandle_t xLoadManagerQueue;

static SemaphoreHandle_t xReactionTimesMutex;

static bool gracePeriod = false;

static TimerHandle_t graceTimer;

static bool managementMode = false;

static bool maintainanceMode = false;

static bool sheddingLoads = false;

static ManagedState state[LOAD_MANAGER_LOADS] = {ENABLED, ENABLED, ENABLED, ENABLED, ENABLED};

static int8_t enabledLoadsCount = LOAD_MANAGER_LOADS;

static int8_t managedLoadsCount = LOAD_MANAGER_LOADS;

static Event event;

static ReactionTimes reactionTimes = { 
    .min = UINT_MAX, 
    .max = 0, 
    .average = 0, 
    .averageSamples = 0 
};

static void registerReactionTime(float then) {
	float now = timestamp();
	float t = now - then;
	xSemaphoreTake(xReactionTimesMutex, portMAX_DELAY);
    if (t < reactionTimes.min) { 
        reactionTimes.min = t; 
    } 
 
    if (t > reactionTimes.max) { 
        reactionTimes.max = t;
    } 
 
    reactionTimes.average = (reactionTimes.average * reactionTimes.averageSamples + t) / (reactionTimes.averageSamples + 1);
    reactionTimes.averageSamples++;
    xSemaphoreGive(xReactionTimesMutex);
} 

static void graceTimerCallback(xTimerHandle t_timer) {
	gracePeriod = false;
	Event event = {
		.code = EVENT_LOAD_MANAGER_GRACE_EXPIRED,
		.timestamp = timestamp()
	};
	xQueueSend(xLoadManagerQueue, &event, 10);
}

static void graceTimerReset() {
	gracePeriod = true;
	xTimerReset(graceTimer, 10);
}

static void graceTimerStop() {
	gracePeriod = false;
	xTimerStop(graceTimer, 10);
}

static void shedLoad() {
	uint8_t i;
	for (i = 0; i < LOAD_MANAGER_LOADS; i++) {
		if (state[i] == ENABLED) {
			state[i] = SHED;
			break;
		}
	}
	enabledLoadsCount--;
}

static void enableLoad() {
	int8_t i;
	for (i = LOAD_MANAGER_LOADS - 1; i >= 0 ; i--) {
		if (state[i] == SHED) {
			state[i] = ENABLED;
			break;
		}
	}
	enabledLoadsCount++;
}

static void updateStateFromSwitch() {
	uint8_t i;
	int8_t enabledLoads = 0;

	bool switchStates[SWITCH_COUNT];
	Switch_getState(switchStates);
	printf("Current switch state LM is [%u, %u, %u, %u, %u]\n",
			switchStates[0], switchStates[1], switchStates[2], switchStates[3], switchStates[4]);

	for (i = 0; i < LOAD_MANAGER_LOADS && i < SWITCH_COUNT; i++) {
		if (switchStates[i]) {
			state[i] = ENABLED;
			enabledLoads++;
		}
		else {
			state[i] = DISABLED;
		}
	}

	enabledLoadsCount = enabledLoads;
	managedLoadsCount = enabledLoads;
}

static void Task_loadManager(void *pvParameters) {
	uint8_t i;
	
	while (1) {
		if (xQueueReceive(xLoadManagerQueue, &event, portMAX_DELAY) == pdTRUE) {
			if (event.code == EVENT_BUTTON_PRESSED) {
				maintainanceMode = !maintainanceMode;
				if (maintainanceMode) {
					managementMode = false;
					updateStateFromSwitch();
//					printf("Current switch state is [%u, %u, %u, %u, %u]\n",
//							state[0], state[1], state[2], state[3], state[4]);
				}
			}
			else if (event.code == EVENT_FREQUENCY_ANALYZER_STABLE) {
				sheddingLoads = false;
				if (!maintainanceMode) {
					if (gracePeriod) {
						if (enabledLoadsCount < managedLoadsCount) {
							graceTimerReset();
						}
						else {
							managementMode = false;
							graceTimerStop();
						}
					}
					else {
						if (enabledLoadsCount < managedLoadsCount) {
							graceTimerReset();
						}
						else {
							managementMode = false;
						}
					}
				}
			}
			else if (event.code == EVENT_FREQUENCY_ANALYZER_UNSTABLE) {
				sheddingLoads = true;
				managementMode = true;
				if (!maintainanceMode) {
					if (gracePeriod) {
						if (enabledLoadsCount > 0) {
							graceTimerReset();
						}
						else {
							graceTimerStop();
						}
					}
					else {
						if (enabledLoadsCount > 1) {
							shedLoad();
							graceTimerReset();
						}
						else if (enabledLoadsCount == 1) {
							shedLoad();
						}
						registerReactionTime(event.timestamp);
					}
				}
			}
			else if (event.code >= EVENT_SWITCH_ON(0) && event.code <= EVENT_SWITCH_ON(4)) {
				if (maintainanceMode) {
					updateStateFromSwitch();
//					printf("Current switch state is [%u, %u, %u, %u, %u]\n",
//							state[0], state[1], state[2], state[3], state[4]);
				}
				else {
//					printf("%d\n", managementMode);
					i = event.code - EVENT_SWITCH_ON(0);
					if (i < LOAD_MANAGER_LOADS) {
						if (state[i] == DISABLED) {
							if (managementMode) {
								state[i] = SHED;
								managedLoadsCount++;
							}
							else {
								state[i] = ENABLED;
								managedLoadsCount++;
								enabledLoadsCount++;
							}
						}
					}
				}
			}
			else if (event.code >= EVENT_SWITCH_OFF(0) && event.code <= EVENT_SWITCH_OFF(4)) {
				if (maintainanceMode) {
					updateStateFromSwitch();
//					printf("Current switch state is [%u, %u, %u, %u, %u]\n",
//							state[0], state[1], state[2], state[3], state[4]);
				}
				else {
					i = event.code - EVENT_SWITCH_OFF(0);
					if (i < LOAD_MANAGER_LOADS) {
						if (state[i] == ENABLED) {
							state[i] = DISABLED;
							managedLoadsCount--;
							enabledLoadsCount--;
							if (gracePeriod) {
								if (managedLoadsCount == 0) {
									graceTimerReset();
								}
								else {
									graceTimerStop();
								}
							}
						}
						else if (state[i] == SHED) {
							state[i] = DISABLED;
							managedLoadsCount--;
						}
					}
				}
			}
			else if (event.code == EVENT_LOAD_MANAGER_GRACE_EXPIRED) {
				if (!gracePeriod) {
					if (!maintainanceMode) {
						if (sheddingLoads) {
							if (managedLoadsCount > 1) {
								shedLoad();
								// graceTimerReset
							}
							else if (managedLoadsCount == 1) {
								shedLoad();
								graceTimerStop();
							}
							else {
								graceTimerStop();
							}
						}
						else {
							if (enabledLoadsCount < managedLoadsCount - 1) {
								enableLoad();
								// graceTimerReset();
							}
							else if (enabledLoadsCount == managedLoadsCount - 1) {
								managementMode = false;
								enableLoad();
								graceTimerStop();
							}
							else {
								managementMode = false;
								graceTimerStop();
							}
						}
					}
				}
			}
		}

		//printf("Event received: %u, sheddingLoads: %u\n", event, (unsigned int) sheddingLoads);
	}
}

void LoadManager_start() {
	uint8_t switchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
	uint8_t i;
	for (i = 0; i < SWITCH_COUNT; i++) {
		state[i] = (ManagedState) ((switchValue >> i) & 1);
	}

	xLoadManagerQueue = xQueueCreate(16, sizeof(Event));
	xReactionTimesMutex = xSemaphoreCreateMutex();
	graceTimer = xTimerCreate("graceTimer", xGraceTimerFrequency, pdTRUE, NULL, graceTimerCallback);
	xTaskCreate(Task_loadManager, "loadManager", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

QueueHandle_t LoadManager_getQueueHandle() {
	return xLoadManagerQueue;
}

ManagedState LoadManager_getState(uint8_t i) {
	return state[i];
}

ReactionTimes LoadManager_getReactionTimes() {
	xSemaphoreTake(xReactionTimesMutex, portMAX_DELAY);
	ReactionTimes retVal = reactionTimes;
	xSemaphoreGive(xReactionTimesMutex);
    return retVal;
} 

bool LoadManager_isMaintainanceMode() {
	return maintainanceMode;
}
