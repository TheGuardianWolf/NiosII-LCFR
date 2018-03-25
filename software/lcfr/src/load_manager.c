#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "load_manager.h"
#include "switch.h"
#include "button.h"
#include "frequency_analyzer.h"

static const TickType_t xFrequency = LOAD_MANAGER_PERIOD * portTICK_PERIOD_MS;
static const TickType_t xGraceTimerFrequency = LOAD_MANAGER_GRACE * portTICK_PERIOD_MS;

static bool gracePeriod = false;

static TimerHandle_t graceTimer;

static bool active = true;

static float config[3] = {45.0f, 55.0f, 1.0f};

static bool lastShedLoad = false;

static bool lastSwitchState[LOAD_MANAGER_LOADS] = {true, true, true, true, true};

static ManagedState state[LOAD_MANAGER_LOADS] = {ENABLED, ENABLED, ENABLED, ENABLED, ENABLED};

//static bool loadShed[LOAD_MANAGER_LOADS] = {false, false, false, false, false};
//
//static uint8_t loadShedCount = 0;

static uint8_t activeLoads = LOAD_MANAGER_LOADS;

static void graceTimerCallback(xTimerHandle t_timer) {
	gracePeriod = false;
}

static void graceTimerReset() {
	gracePeriod = true;
	xTimerReset(graceTimer, 10);
}

static void Task_loadManager(void *pvParameters) {
	TickType_t xLastWakeTime;

	while (1) {
		xLastWakeTime = xTaskGetTickCount();

		uint8_t i;
		bool shedLoad = false;

		// Switch event detection and handling
		bool switchWasDisabled = false;

		for (i = 0; i < LOAD_MANAGER_LOADS; i++) {
			bool switchState = Switch_getState(i);
			if (lastSwitchState[i]) {
				if (!switchState) {
					switchWasDisabled = true;
					state[i] = DISABLED;
					activeLoads--;
				}
			}
			else {
				if (switchState) {
					if (active && shedLoad) {
						state[i] = SHED;
					}
					else {
						state[i] = ENABLED;
						activeLoads++;
					}
				}
			}
			lastSwitchState[i] = switchState;
		}

		if (active) {
			FrequencySample sample = FrequencyAnalyzer_getFrequencySample();
			shedLoad = (sample.instant < config[0] || sample.instant > config[1] || sample.derivative > config[2]);

			if (gracePeriod && lastShedLoad != shedLoad) {
				graceTimerReset();
			}
			else {
				if (shedLoad) {
					if (gracePeriod && switchWasDisabled) {
						graceTimerReset();
					}

					// Drop a load if we are not waiting
					if (!gracePeriod && activeLoads > 0) {
						for (i = 0; i < LOAD_MANAGER_LOADS; i++) {
							if (state[i] == ENABLED) {
								state[i] = SHED;
								activeLoads--;
							}
						}
						graceTimerReset();
					}
				}
				else {
					// Reset waiting period if switch was used to enable
					/*
					if (gracePeriod && switchWasEnabled) {
						graceTimerReset();
					}
					*/

					// Connect a load if we are not waiting
					if (!gracePeriod && activeLoads < LOAD_MANAGER_LOADS) {
						for (i = 0; i < LOAD_MANAGER_LOADS; i++) {
							if (state[i] == SHED) {
								state[i] = ENABLED;
								activeLoads++;
							}
						}
						graceTimerReset();
					}
				}
			}

			lastShedLoad = shedLoad;
		}
		else {
			if (gracePeriod) {
				gracePeriod = false;
				xTimerStop(graceTimer, 10);
			}
		}

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void LoadManager_start() {
	Button_ISRAttachHook(0, &LoadManager_toggleActive);
	graceTimer = xTimerCreate("graceTimer", xGraceTimerFrequency, pdFALSE, NULL, graceTimerCallback);
	xTaskCreate(Task_loadManager, "loadManager", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

bool LoadManager_getActive() {
	return active;
}

void LoadManager_toggleActive() {
	active ^= 1;
}

ManagedState LoadManager_getState(uint8_t i) {
	return state[i];
}
