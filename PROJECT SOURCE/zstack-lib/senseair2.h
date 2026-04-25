//==============================================================================
// senseair2.h
//==============================================================================
// Sensair CO2 sensor driver
//==============================================================================

#ifndef SENSEAIR2_H
#define SENSEAIR2_H

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize Sensair sensor UART interface
 * @param pf Callback function for UART events (RX, TX, errors)
 */
void SenseAir_Init(halUARTCBack_t pf);

/**
 * @brief Request CO2 measurement from sensor
 */
void SenseAir_RequestMeasure(void);

/**
 * @brief Request measurement (alias for Z8 series compatibility)
 */
void Z8_RequestMeasure(void);

/**
 * @brief Read CO2 value from UART buffer
 * @return CO2 concentration in ppm, or 0xFFFF on invalid response
 */
uint16 SenseAir_Read(void);

/**
 * @brief Enable or disable Automatic Baseline Correction (ABC)
 * @param isEnabled TRUE to enable ABC, FALSE to disable
 */
void SenseAir_SetABC(bool isEnabled);

/**
 * @brief Calculate Modbus checksum (CRC) for a packet
 * @param packet Pointer to packet buffer (8 bytes)
 * @return Calculated checksum byte
 */
uint8 getCheckSum(uint8 *packet);

#endif // SENSEAIR2_H