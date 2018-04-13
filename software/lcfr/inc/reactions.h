#ifndef Reactions_H
#define Reactions_H

#include <stdbool.h>

typedef struct {
	uint32_t min;
	uint32_t max;
	uint32_t average;
	uint32_t averageSamples;
} ReactionTimes;

void Reactions_registerReactionTime(uint32_t t) 
void Reactions_start(bool inISR);

bool Reactions_isRunning();

void Reactions_stop(bool inISR);

uint32_t Reactions_getTimeElapsed(bool inISR);

void Reactions_reset();

#endif /* Reactions_H */
