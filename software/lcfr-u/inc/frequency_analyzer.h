#ifndef FREQUENCY_ANALYZER_H_
#define FREQUENCY_ANALYZER_H_

#include <stdint.h>

#define FREQUENCY_ANALYZER_SAMPLING_FREQUENCY 16000.0f
#define FREQUENCY_ANALYZER_CONFIG_TYPES 3

typedef struct {
	uint32_t adcSamples;
	float instant;
	float derivative;
} FrequencySample;

typedef struct {
	uint32_t timestamp;
	uint32_t adcSamples;
} ADCSample;

void FrequencyAnalyzer_start();

float FrequencyAnalyzer_getConfig(uint8_t configIndex);

void FrequencyAnalyzer_setConfig(uint8_t configIndex, float val);

#endif /* FREQUENCY_ANALYZER_H_ */
