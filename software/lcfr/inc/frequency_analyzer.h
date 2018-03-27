#ifndef FREQUENCY_ANALYZER_H_
#define FREQUENCY_ANALYZER_H_

#define FREQUENCY_ANALYZER_SAMPLING_FREQUENCY 16000.0f

#define EVENT_FREQUENCY_ANALYZER_STABLE 254
#define EVENT_FREQUENCY_ANALYZER_UNSTABLE 253

typedef struct {
	uint32_t adcSamples;
	float instant;
	float derivative;
} FrequencySample;

void FrequencyAnalyzer_start();

#endif /* FREQUENCY_ANALYZER_H_ */
