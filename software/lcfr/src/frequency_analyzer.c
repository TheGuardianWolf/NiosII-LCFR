#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <io.h>
#include <sys/alt_irq.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "frequency_analyzer.h"
#include "load_manager.h"

static FrequencySample currentSample;
static bool stablity = true;
static float config[3] = {45.0f, 55.0f, 10.0f};
static bool firstMeasurement = true;

static void ISR_frequencyAnalyzer() {
    FrequencySample newSample;
    bool newStablity;

    newSample.adcSamples = IORD(FREQUENCY_ANALYSER_BASE, 0);
    newSample.instant = FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / newSample.adcSamples;

	newSample.derivative = fabs(newSample.instant - currentSample.instant) *  FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / ((float)(currentSample.adcSamples + newSample.adcSamples) / 2);

	if (!firstMeasurement) {
		newStablity = (newSample.instant > config[0] && newSample.instant < config[1] && newSample.derivative < config[2]);
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

#if DEBUG == 1
		printf("ISR Frequency Analyzer Executed\n");
		printf("samples: %u, instant: %f, derivative: %f\n", newSample.adcSamples, newSample.instant, newSample.derivative);
#endif
}

void FrequencyAnalyzer_start() {
    alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, ISR_frequencyAnalyzer);
}
