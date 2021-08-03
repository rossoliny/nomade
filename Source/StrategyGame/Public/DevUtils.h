#pragma once

/**
 * Call doNotOptimizeAway(var) against variables that you use for
 * benchmarking but otherwise are useless. The compiler tends to do a
 * good job at eliminating unused variables, and this function fools
 * it into thinking var is in fact needed.
 */
#ifdef _MSC_VER

#pragma optimize("", off)

template <class T>
void doNotOptimizeAway(T&& datum) {
	datum = datum;
}

#pragma optimize("", on)

#elif defined(__clang__)

template <class T>
__attribute__((__optnone__)) void doNotOptimizeAway(T&& /* datum */) {}

#else

template <class T>
void doNotOptimizeAway(T&& datum) {
	asm volatile("" : "+r" (datum));
}

#endif