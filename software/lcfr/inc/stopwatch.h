#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <stdbool.h>

void Stopwatch_start(bool inISR);

bool Stopwatch_isRunning();

void Stopwatch_stop(bool inISR);

uint32_t Stopwatch_getTimeElapsed(bool inISR);

void Stopwatch_reset();

#endif /* STOPWATCH_H */
