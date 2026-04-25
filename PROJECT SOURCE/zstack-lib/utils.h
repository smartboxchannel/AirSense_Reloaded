//==============================================================================
// utils.h
//==============================================================================
// Utility functions and I/O port macros
//==============================================================================

#ifndef UTILS_H
#define UTILS_H

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
extern double mapRange(double a1, double a2, double b1, double b2, double s);

/**
 * @brief Read ADC multiple times and return average
 * @param channel      ADC channel to read
 * @param resolution   ADC resolution (8, 10, 12, 14 bits)
 * @param reference    ADC reference voltage
 * @param samplesCount Number of samples to average
 * @return Averaged ADC reading
 */
extern uint16 adcReadSampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount);

//==============================================================================
// I/O Port Macros
//==============================================================================

#undef P
#undef INP
#define INP INP
#undef DIR
#define DIR DIR
#undef SEL
#define SEL SEL

// General I/O definitions
#define IO_GIO 0
#define IO_PER 1
#define IO_IN  0
#define IO_OUT 1
#define IO_PUD 0
#define IO_TRI 1
#define IO_PUP 0
#define IO_PDN 1

// String concatenation helpers
#define CAT1(x, y) x##y
#define CAT2(x, y) CAT1(x, y)

// Build I/O port register name
#define PNAME(y, z) CAT2(P, CAT2(y, z))

// Build I/O bit name
#define BNAME(port, pin) CAT2(CAT2(P, port), CAT2(_, pin))

// Set I/O pin direction
#define IO_DIR_PORT_PIN(port, pin, dir) \
    do { \
        if (dir == IO_OUT) \
            PNAME(port, DIR) |= (1 << (pin)); \
        else \
            PNAME(port, DIR) &= ~(1 << (pin)); \
    } while(0)

// Set I/O pin function
#define IO_FUNC_PORT_PIN(port, pin, func) \
    do { \
        if (port < 2) { \
            if (func == IO_PER) \
                PNAME(port, SEL) |= (1 << (pin)); \
            else \
                PNAME(port, SEL) &= ~(1 << (pin)); \
        } else { \
            if (func == IO_PER) \
                P2SEL |= (1 << (pin >> 1)); \
            else \
                P2SEL &= ~(1 << (pin >> 1)); \
        } \
    } while(0)

// Set I/O pin input mode
#define IO_IMODE_PORT_PIN(port, pin, mode) \
    do { \
        if (mode == IO_TRI) \
            PNAME(port, INP) |= (1 << (pin)); \
        else \
            PNAME(port, INP) &= ~(1 << (pin)); \
    } while(0)

// Set pull-up/pull-down direction
#define IO_PUD_PORT(port, dir) \
    do { \
        if (dir == IO_PDN) \
            P2INP |= (1 << (port + 5)); \
        else \
            P2INP &= ~(1 << (port + 5)); \
    } while(0)

#endif // UTILS_H