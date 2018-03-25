#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <io.h>
#include <sys/alt_irq.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "frequency_analyzer.h"

static SemaphoreHandle_t sampleLock;

static FrequencySample currentSample;

static void ISR_frequencyAnalyzer() {
	FrequencySample newSample;
	newSample.adcSamples = IORD(FREQUENCY_ANALYSER_BASE, 0);
	newSample.instant = FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / newSample.adcSamples;
	newSample.derivative = fabs(newSample.instant - currentSample.instant) *  FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / ((currentSample.adcSamples + newSample.adcSamples) / 2);

	xSemaphoreTakeFromISR(sampleLock, NULL);
	currentSample = newSample;
	xSemaphoreGiveFromISR(sampleLock, NULL);
}

void FrequencyAnalyzer_start() {
	sampleLock = xSemaphoreCreateBinary();
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, ISR_frequencyAnalyzer);
}

FrequencySample FrequencyAnalyzer_getFrequencySample() {
	xSemaphoreTake(sampleLock, portMAX_DELAY);
	FrequencySample sample = currentSample;
	xSemaphoreGive(sampleLock);
	return sample;
}
