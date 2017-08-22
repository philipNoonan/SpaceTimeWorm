#pragma once

#include <ctime>
#include <chrono>

class SimpleTimer {
private:
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
public:
	SimpleTimer() { start = end = std::chrono::high_resolution_clock::now(); }
	~SimpleTimer() {}

	void tick() {
		start = std::chrono::high_resolution_clock::now();
	}

	void tock() {
		end = std::chrono::high_resolution_clock::now();
	}

	double getDurationInSeconds() {
		return (double)std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
	}

	double getDurationInMilliseconds() {
		return (double)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	double getDurationInMicroseconds() {
		return (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	}

	double getDurationInNanoseconds() {
		return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	}
};

class FrequencyMonitor {
private:
	unsigned int count;
	std::chrono::high_resolution_clock::time_point begin;
	double hertz;
public:
	FrequencyMonitor() { reset(); }
	~FrequencyMonitor() {}

	// returns true iff a new counting cycle is completed.
	bool ding() {
		count++;

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		double duration = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count();
		if (duration > 1000.0) {
			hertz = count * 1000 / duration;
			count = 0;
			begin = now;

			return true;
		}
		else {
			return false;
		}
	}

	double getFrequency() {
		return hertz;
	}

	void reset() {
		count = 0; 
		hertz = 0; 
		begin = std::chrono::high_resolution_clock::now();
	}
};