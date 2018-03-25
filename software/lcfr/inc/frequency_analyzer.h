#ifndef FREQUENCY_ANALYZER_H_
#define FREQUENCY_ANALYZER_H_

#define FREQUENCY_ANALYZER_SAMPLING_FREQUENCY 16000.0f

typedef struct {
	uint32_t adcSamples;
	float instant;
	float derivative;
} FrequencySample;

void FrequencyAnalyzer_start();

FrequencySample FrequencyAnalyzer_getFrequencySample();

#endif /* FREQUENCY_ANALYZER_H_ */
