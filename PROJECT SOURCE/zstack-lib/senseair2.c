//==============================================================================
// senseair2.c
//==============================================================================
// Sensair CO2 sensor driver implementation
//==============================================================================

#include "hal_types.h"
#include "hal_uart.h"
#include "senseair2.h"
#include "OSAL.h"

//==============================================================================
// Defines
//==============================================================================

#ifndef CO2_UART_PORT
#define CO2_UART_PORT   HAL_UART_PORT_0
#endif

#define AIR_QUALITY_INVALID_RESPONSE    0xFFFF
#define SENSEAIR_RESPONSE_LENGTH        20

//==============================================================================
// Modbus Commands
//==============================================================================

// Read CO2 and temperature registers (function code 0x04)
uint8 readCO2[] = {0xFE, 0x04, 0x00, 0x00, 0x00, 0x04, 0xE5, 0xC6};

// Disable Automatic Baseline Correction (write to register 0x001F)
uint8 disableABC[] = {0xFE, 0x06, 0x00, 0x1F, 0x00, 0x00, 0xAC, 0x03};

// Enable Automatic Baseline Correction (write to register 0x001F)
uint8 enableABC[] = {0xFE, 0x06, 0x00, 0x1F, 0x00, 0xB4, 0xAC, 0x74};

// Switch command (Z8 series)
uint8 swith[] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00, 0x46};

// Read CH2O (formaldehyde) command (Z8 series)
uint8 readCH20[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize Sensair sensor UART at 9600 baud
 * @param pf Callback function for UART events
 */
void SenseAir_Init(halUARTCBack_t pf)
{
    halUARTCfg_t uartConfig;

    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_9600;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 48;
    uartConfig.rx.maxBufSize        = SENSEAIR_RESPONSE_LENGTH;
    uartConfig.tx.maxBufSize        = SENSEAIR_RESPONSE_LENGTH;
    uartConfig.idleTimeout          = 10;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = pf;

    HalUARTInit();
    HalUARTOpen(CO2_UART_PORT, &uartConfig);
}

/**
 * @brief Enable or disable Automatic Baseline Correction (ABC)
 * @param isEnabled TRUE to enable ABC, FALSE to disable
 */
void SenseAir_SetABC(bool isEnabled)
{
    if (isEnabled) {
        HalUARTWrite(CO2_UART_PORT, enableABC, sizeof(enableABC) / sizeof(enableABC[0]));
    } else {
        HalUARTWrite(CO2_UART_PORT, disableABC, sizeof(disableABC) / sizeof(disableABC[0]));
    }
}

/**
 * @brief Request CO2 measurement from sensor
 */
void SenseAir_RequestMeasure(void)
{
    HalUARTWrite(CO2_UART_PORT, readCO2, sizeof(readCO2) / sizeof(readCO2[0]));
}

/**
 * @brief Alias for SenseAir_RequestMeasure (Z8 series compatibility)
 */
void Z8_RequestMeasure(void)
{
    SenseAir_RequestMeasure();
}

/**
 * @brief Read and parse CO2 value from UART response
 * @return CO2 concentration in ppm, or 0xFFFF on invalid response
 */
uint16 SenseAir_Read(void)
{
    uint8 response[SENSEAIR_RESPONSE_LENGTH];
    HalUARTRead(CO2_UART_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));

    // Validate response header
    if (response[0] != 0xFE || response[1] != 0x04) {
        return AIR_QUALITY_INVALID_RESPONSE;
    }

    const uint8 length = response[2];
    // const uint16 status = (((uint16)response[3]) << 8) | response[4];  // Status word (unused)
    const uint16 ppm = (((uint16)response[length + 1]) << 8) | response[length + 2];

    return ppm;
}

/**
 * @brief Calculate Modbus checksum byte
 * @param packet Pointer to 8-byte packet buffer
 * @return Calculated checksum byte
 */
uint8 getCheckSum(uint8 *packet)
{
    uint8 i;
    uint8 checksum = 0;

    for (i = 1; i < 8; i++) {
        checksum += packet[i];
    }

    checksum = 0xFF - checksum;
    checksum += 1;

    return checksum;
}