#ifndef FREQUENCY_ANALYZER_H_
#define FREQUENCY_ANALYZER_H_

#define FREQUENCY_ANALYZER_SAMPLING_FREQUENCY 16000.0f

#define EVENT_FREQUENCY_ANALYZER_STABLE 254
#define EVENT_FREQUENCY_ANALYZER_UNSTABLE 253

#include <system.h>

typedef struct {
	uint32_t adcSamples;
	float instant;
	float derivative;
} FrequencySample;

void FrequencyAnalyzer_start();

float FrequencyAnalyzer_getConfig(uint8_t configIndex);

void FrequencyAnalyzer_setConfig(uint8_t configIndex, float val);

#endif /* FREQUENCY_ANALYZER_H_ */
