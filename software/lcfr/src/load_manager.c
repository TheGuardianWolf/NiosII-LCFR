#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "load_manager.h"
#include "switch.h"
#include "button.h"
#include "frequency_analyzer.h"

static const TickType_t xGraceTimerFrequency = LOAD_MANAGER_GRACE * portTICK_PERIOD_MS;
static QueueHandle_t xLoadManagerQueue;

static bool gracePeriod = false;

static TimerHandle_t graceTimer;

static bool maintainanceMode = false;

static bool sheddingLoads = false;

static ManagedState state[LOAD_MANAGER_LOADS] = {ENABLED, ENABLED, ENABLED, ENABLED, ENABLED};

static int8_t enabledLoadsCount = LOAD_MANAGER_LOADS;

static int8_t managedLoadsCount = LOAD_MANAGER_LOADS;

static void graceTimerCallback(xTimerHandle t_timer) {
	gracePeriod = false;
	uint8_t event = EVENT_LOAD_MANAGER_GRACE_EXPIRED;
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
	uint8_t i;
	for (i = 0; i < LOAD_MANAGER_LOADS; i++) {
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

	for (i = 0; i < LOAD_MANAGER_LOADS && i < SWITCH_COUNT; i++) {
		if (Switch_getState(i)) {
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
	uint8_t event;
	uint8_t i;
	
	while (1) {
		if (xQueueReceive(xLoadManagerQueue, &event, portMAX_DELAY) == pdTRUE) {
			if (event == EVENT_BUTTON_PRESSED) {
				maintainanceMode = !maintainanceMode;
				if (maintainanceMode) {
					updateStateFromSwitch();
				}
			}
			else if (event == EVENT_FREQUENCY_ANALYZER_STABLE) {
				sheddingLoads = false;
				if (!maintainanceMode) {
					if (gracePeriod) {
						if (enabledLoadsCount < managedLoadsCount) {
							graceTimerReset();
						}
						else {
							graceTimerStop();
						}
					}
					else {
						if (enabledLoadsCount < managedLoadsCount) {
							graceTimerReset();
						}
					}
				}
			}
			else if (event == EVENT_FREQUENCY_ANALYZER_UNSTABLE) {
				sheddingLoads = true;
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
					}
				}
			}
			else if (event >= EVENT_SWITCH_ON(0) && event <= EVENT_SWITCH_ON(4)) {
				if (maintainanceMode) {
					updateStateFromSwitch();
				}
				else {
					i = event - EVENT_SWITCH_ON(0);
					if (i < LOAD_MANAGER_LOADS) {
						if (!sheddingLoads && state[i] == DISABLED) {
							state[i] = ENABLED;
							managedLoadsCount++;
							enabledLoadsCount++;
						}
					}
				}
			}
			else if (event >= EVENT_SWITCH_OFF(0) && event <= EVENT_SWITCH_OFF(4)) {
				if (maintainanceMode) {
					updateStateFromSwitch();
				}
				else {
					i = event - EVENT_SWITCH_OFF(0);
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
			else if (event == EVENT_LOAD_MANAGER_GRACE_EXPIRED) {
				// May seem redundant to check it, but accounts for when events
				// stored in queue gets outdated before they are processed
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
						}
						else {
							if (enabledLoadsCount < managedLoadsCount - 1) {
								enableLoad();
								// graceTimerReset();
							}
							else if (enabledLoadsCount == managedLoadsCount - 1) {
								enableLoad();
								graceTimerStop();
							}
						}
					}
				}
			}
		}
	}
}

void LoadManager_start() {
	xLoadManagerQueue = xQueueCreate(16, sizeof(uint8_t));
	graceTimer = xTimerCreate("graceTimer", xGraceTimerFrequency, pdTRUE, NULL, graceTimerCallback);
	xTaskCreate(Task_loadManager, "loadManager", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

QueueHandle_t LoadManager_getQueueHandle() {
	return xLoadManagerQueue;
}

ManagedState LoadManager_getState(uint8_t i) {
	return state[i];
}
