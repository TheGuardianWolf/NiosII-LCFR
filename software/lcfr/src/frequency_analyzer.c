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
#include "VGA.h"

static FrequencySample currentSample;
static bool stablity = true;
static float configValues[3] = {45.0f, 55.0f, 10.0f};
static bool firstMeasurement = true;
static SemaphoreHandle_t xConfigSemaphore;

static void ISR_frequencyAnalyzer() {
	//printf("enter freq ");
    FrequencySample newSample;
    bool newStablity;

    newSample.adcSamples = IORD(FREQUENCY_ANALYSER_BASE, 0);
    newSample.instant = FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / newSample.adcSamples;

	newSample.derivative = fabs(newSample.instant - currentSample.instant) *  FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / ((float)(currentSample.adcSamples + newSample.adcSamples) / 2);

	//check if it's the first measurement, if it is then ignore readings.
	if (!firstMeasurement) {
		newStablity = (newSample.instant > configValues[0] && newSample.instant < configValues[1] && newSample.derivative < configValues[2]);
	}
	else {
		newStablity = true;
	}

	if (newStablity != stablity) {
		uint8_t event = newStablity ? EVENT_FREQUENCY_ANALYZER_STABLE : EVENT_FREQUENCY_ANALYZER_UNSTABLE;
		//xQueueSendFromISR(LoadManager_getQueueHandle(), &event, NULL);
	}

	stablity = newStablity;
	currentSample = newSample;
	firstMeasurement = false;

	VGAFrequencyInfo vgaFreqInfo = {
		.stable = newStablity,
		.freq = newSample.instant,
		.derivative = newSample.derivative
	};

	//xQueueSendFromISR(VGA_getQueueHandle(), &vgaFreqInfo, NULL);
#if DEBUG == 1
		printf("ISR Frequency Analyzer Executed\n");
		printf("samples: %u, instant: %f, derivative: %f\n", newSample.adcSamples, vgaFreqInfo.freq, vgaFreqInfo.derivative);
#endif
}

void FrequencyAnalyzer_start() {
    alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, ISR_frequencyAnalyzer);
	xConfigSemaphore = xSemaphoreCreateBinary();
	printf("finished FA init");
}

float FrequencyAnalyzer_getConfig(uint8_t configIndex) {
	xSemaphoreTake(xConfigSemaphore, portMAX_DELAY);
	float retConfig = configValues[configIndex];
	xSemaphoreGive(xConfigSemaphore);
	return retConfig;
}

void FrequencyAnalyzer_setConfig(uint8_t configIndex, float val) {
	xSemaphoreTake(xConfigSemaphore, portMAX_DELAY);
	configValues[configIndex] = val;
	xSemaphoreGive(xConfigSemaphore);
}
