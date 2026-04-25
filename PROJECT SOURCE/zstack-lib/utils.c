//==============================================================================
// utils.c
//==============================================================================
// Utility functions implementation
//==============================================================================

#include "utils.h"
#include "hal_adc.h"

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Map a value from one range to another
 * @param a1 Lower bound of source range
 * @param a2 Upper bound of source range
 * @param b1 Lower bound of target range
 * @param b2 Upper bound of target range
 * @param s  Source value to map
 * @return Mapped value clamped to target range
 */
double mapRange(double a1, double a2, double b1, double b2, double s)
{
    double result = b1 + (s - a1) * (b2 - b1) / (a2 - a1);
    return MIN(b2, MAX(result, b1));
}

/**
 * @brief Read ADC multiple times and return average
 * @param channel      ADC channel to read
 * @param resolution   ADC resolution (8, 10, 12, 14 bits)
 * @param reference    ADC reference voltage
 * @param samplesCount Number of samples to average
 * @return Averaged ADC reading
 */
uint16 adcReadSampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount)
{
    HalAdcSetReference(reference);
    uint32 samplesSum = 0;

    for (uint8 i = 0; i < samplesCount; i++) {
        samplesSum += HalAdcRead(channel, resolution);
    }

    return samplesSum / samplesCount;
}