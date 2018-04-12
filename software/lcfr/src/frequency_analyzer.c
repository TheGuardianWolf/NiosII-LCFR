#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <io.h>
#include <sys/alt_irq.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "system.h"

#include "frequency_analyzer.h"
#include "load_manager.h"
#include "display.h"
#include "config.h"
#include "VGA.h"
#include "keyboard.h"

static FrequencySample currentSample;
static bool stablity = true;
static float config_values[3] = {45.0f, 55.0f, 10.0f};
static struct config configReceive;
static struct display_info display;
static bool firstMeasurement = true;

static void ISR_frequencyAnalyzer() {
	//printf("enter freq ");
    FrequencySample newSample;
    bool newStablity;

    newSample.adcSamples = IORD(FREQUENCY_ANALYSER_BASE, 0);
    newSample.instant = FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / newSample.adcSamples;

	newSample.derivative = fabs(newSample.instant - currentSample.instant) *  FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / ((float)(currentSample.adcSamples + newSample.adcSamples) / 2);

	//get config semaphore and take the config data
	if (xSemaphoreTake(getConfigSemaphore(), portMAX_DELAY) == pdTRUE) {
		//configReceive = getConfig();
		xSemaphoreGive(getConfigSemaphore());

		if (configReceive.type == lower_freq) {
			config_values[0] = (float)configReceive.value;
		}
		else {
			config_values[2] = (float)configReceive.value;
		}
	}

	//check if it's the first measurement, if it is then ignore readings.
	if (!firstMeasurement) {
		newStablity = (newSample.instant > config_values[0] && newSample.instant < config_values[1] && newSample.derivative < config_values[2]);
	}
	else {
		newStablity = true;
	}

	if (newStablity != stablity) {
		uint8_t event = newStablity ? EVENT_FREQUENCY_ANALYZER_STABLE : EVENT_FREQUENCY_ANALYZER_UNSTABLE;
		xQueueSendFromISR(LoadManager_getQueueHandle(), &event, NULL);
	}

	stablity = newStablity;
	currentSample = newSample;
	firstMeasurement = false;

	display.stable = newStablity;
	display.freq = newSample.instant;

	xQueueSendFromISR(VGA_getQueueHandle(), &display, NULL);

#if DEBUG == 1
		printf("ISR Frequency Analyzer Executed\n");
		printf("samples: %u, instant: %f, derivative: %f\n", newSample.adcSamples, display.freq, newSample.derivative);
#endif
}

void FrequencyAnalyzer_start() {
    alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, ISR_frequencyAnalyzer);
	printf("finished FA init");
}
