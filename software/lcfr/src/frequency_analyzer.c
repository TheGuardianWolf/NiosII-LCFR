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
#include "event.h"

static FrequencySample currentSample;
static uint32_t newAdcSamples;
static bool stablity = true;
static float configValues[3] = {49.0f, 55.0f, 100.0f};
static bool firstMeasurement = true;
static SemaphoreHandle_t xConfigSemaphore;
static QueueHandle_t xFrequencyAnalyzerQueue;

static void ISR_frequencyAnalyzer() {
    ADCSample adcSample {
		.adcSamples = IORD(FREQUENCY_ANALYSER_BASE, 0),
		.timestamp = timestampFromISR()
	};
	xQueueSendFromISR(xFrequencyAnalyzerQueue, &adcSample, NULL);
}

static void Task_frequencyAnalyzer(void *pvParameters) {
	while(1) {
		if (xQueueReceive(xFrequencyAnalyzerQueue, &newAdcSamples, portMAX_DELAY) == pdTRUE) {
			FrequencySample newSample;
			bool newStablity;

			newSample.adcSamples = newAdcSamples.adcSamples;
			newSample.instant = FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / newSample.adcSamples;

			//check if it's the first measurement, if it is then ignore readings.
			if (!firstMeasurement) {
				newStablity = (newSample.instant > configValues[0] && newSample.instant < configValues[1] && newSample.derivative < configValues[2]);
				newSample.derivative = fabs(newSample.instant - currentSample.instant) *  FREQUENCY_ANALYZER_SAMPLING_FREQUENCY / ((float)(currentSample.adcSamples + newSample.adcSamples) / 2);
			}
			else {
				newStablity = true;
				newSample.derivative = 0.0f;
			}

			if (newStablity != stablity) {
				Event event;
				if (newStablity) {
					event.code = EVENT_FREQUENCY_ANALYZER_STABLE;
					event.timestamp = newSample.timestamp;
				}
				else {
					event.code = EVENT_FREQUENCY_ANALYZER_UNSTABLE;
					event.timestamp = newSample.timestamp;
				}
				xQueueSend(LoadManager_getQueueHandle(), &event, portMAX_DELAY);
			}

			stablity = newStablity;
			currentSample = newSample;
			firstMeasurement = false;

			VGAFrequencyInfo vgaFreqInfo = {
				.stable = newStablity,
				.freq = newSample.instant,
				.derivative = newSample.derivative
			};

			xQueueSend(VGA_getQueueHandle(), &vgaFreqInfo, portMAX_DELAY);
	#if DEBUG == 1
			printf("ISR Frequency Analyzer Executed\n");
			printf("samples: %u, instant: %f, derivative: %f\n", newSample.adcSamples, display.freq, newSample.derivative);
	#endif
		}
	}
}

void FrequencyAnalyzer_start() {
    alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, ISR_frequencyAnalyzer);
	xFrequencyAnalyzerQueue = xQueueCreate(8, sizeof(ADCSample));
	xConfigSemaphore = xSemaphoreCreateMutex();
	xTaskCreate(Task_frequencyAnalyzer, "frequencyAnalyzer",  configMINIMAL_STACK_SIZE, NULL, 6, NULL);
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
