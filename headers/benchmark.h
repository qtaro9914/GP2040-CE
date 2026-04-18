#ifndef _BENCHMARK_H_
#define _BENCHMARK_H_

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

class Benchmark {
public:
#ifdef GP2040_BENCHMARK
    // USB polling is 1ms, so we target 950us to have safety margin
    static constexpr uint32_t USB_DEADLINE_US = 950;
    static constexpr uint32_t REPORT_INTERVAL = 10000;
    static constexpr size_t HISTOGRAM_SIZE = 2000;  // 0-1999us range

    static void start() {
        startTime = time_us_32();
    }

    static void end() {
        uint32_t endTime = time_us_32();
        uint32_t duration = endTime - startTime;

        // Update statistics
        if (duration > maxDuration) maxDuration = duration;
        if (duration < minDuration) minDuration = duration;
        totalDuration += duration;
        count++;

        // Track deadline misses
        if (duration > USB_DEADLINE_US) {
            missedDeadlines++;
        }

        // Update histogram
        if (duration < HISTOGRAM_SIZE) {
            histogram[duration]++;
        }

        // Report every REPORT_INTERVAL iterations
        if (count >= REPORT_INTERVAL) {
            printReport();
            reset();
        }
    }

    static void printReport() {
        uint32_t avg = (count > 0) ? (totalDuration / count) : 0;
        float missRate = (count > 0) ? (100.0f * missedDeadlines / count) : 0.0f;
        uint32_t jitter = maxDuration - minDuration;

        printf("\n=== BENCHMARK REPORT (n=%lu) ===\n", count);
        printf("Duration: Min=%lu us, Max=%lu us, Avg=%lu us, Jitter=%lu us\n",
               minDuration, maxDuration, avg, jitter);
        printf("Deadline: Target=%lu us, Misses=%lu (%.3f%%)\n",
               USB_DEADLINE_US, missedDeadlines, missRate);

        // Calculate percentiles
        uint32_t p50 = calculatePercentile(50);
        uint32_t p95 = calculatePercentile(95);
        uint32_t p99 = calculatePercentile(99);
        printf("Percentiles: P50=%lu us, P95=%lu us, P99=%lu us\n", p50, p95, p99);

        // Performance assessment
        if (missRate < 0.1f) {
            printf("Status: EXCELLENT - Jitter should be imperceptible\n");
        } else if (missRate < 1.0f) {
            printf("Status: GOOD - Minor jitter may occur\n");
        } else if (missRate < 5.0f) {
            printf("Status: FAIR - Noticeable jitter expected\n");
        } else {
            printf("Status: POOR - Significant optimization needed\n");
        }
        printf("================================\n\n");
    }

private:
    static uint32_t startTime;
    static uint32_t minDuration;
    static uint32_t maxDuration;
    static uint64_t totalDuration;
    static uint32_t count;
    static uint32_t missedDeadlines;
    static uint32_t histogram[HISTOGRAM_SIZE];

    static void reset() {
        count = 0;
        totalDuration = 0;
        maxDuration = 0;
        minDuration = UINT32_MAX;
        missedDeadlines = 0;
        memset(histogram, 0, sizeof(histogram));
    }

    static uint32_t calculatePercentile(uint32_t percentile) {
        if (count == 0) return 0;

        uint32_t target = (count * percentile) / 100;
        uint32_t accumulated = 0;

        for (size_t i = 0; i < HISTOGRAM_SIZE; i++) {
            accumulated += histogram[i];
            if (accumulated >= target) {
                return i;
            }
        }
        return HISTOGRAM_SIZE - 1;
    }
};

// Define static members
inline uint32_t Benchmark::startTime = 0;
inline uint32_t Benchmark::minDuration = UINT32_MAX;
inline uint32_t Benchmark::maxDuration = 0;
inline uint64_t Benchmark::totalDuration = 0;
inline uint32_t Benchmark::count = 0;
inline uint32_t Benchmark::missedDeadlines = 0;
inline uint32_t Benchmark::histogram[HISTOGRAM_SIZE] = {0};

#else
    static void start() {}
    static void end() {}
};
#endif

#endif
