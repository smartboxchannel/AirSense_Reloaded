//==============================================================================
// preinclude.h
//==============================================================================
// Project-wide pre-include configuration macros
//==============================================================================

#ifndef PREINCLUDE_H
#define PREINCLUDE_H

//==============================================================================
// ZigBee Network Settings
//==============================================================================
#define TC_LINKKEY_JOIN            // Use Trust Center Link Key for joining
#define NV_INIT                    // Initialize Non-Volatile memory
#define NV_RESTORE                 // Restore state from Non-Volatile memory

#define SENSAIR                    // Sensair CO2 sensor support

#define TP2_LEGACY_ZC              // TP2 Legacy Zigbee Coordinator compatibility mode

#define NWK_AUTO_POLL              // Automatic network polling
#define MULTICAST_ENABLED FALSE    // Multicast disabled

//==============================================================================
// ZCL (ZigBee Cluster Library)
//==============================================================================
#define ZCL_READ                   // Support attribute reading
#define ZCL_WRITE                  // Support attribute writing
#define ZCL_BASIC                  // Basic cluster
#define ZCL_IDENTIFY               // Identify cluster
#define ZCL_REPORTING_DEVICE       // Device supports attribute reporting


//==============================================================================
// GreenPower and BDB (Base Device Behavior)
//==============================================================================
#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING                           TRUE
#define BDB_MAX_CLUSTERENDPOINTS_REPORTING      10

//==============================================================================
// HAL (Hardware Abstraction Layer) Configuration
//==============================================================================
#define HAL_BUZZER FALSE            // Buzzer disabled
#define HAL_KEY TRUE                // Key/button enabled
#define ISR_KEYINTERRUPT            // Key interrupt handler
#define MT_ZDO_MGMT                 // MT ZDO management commands

#define HAL_ADC TRUE                // ADC enabled
#define HAL_LCD FALSE               // LCD disabled

//==============================================================================
// Pin and Port Assignments
//==============================================================================
#define HAL_KEY_P2_INPUT_PINS BV(0) // Button on port P2.0

#define FACTORY_RESET_BY_LONG_PRESS_PORT      0x04  // Port P2 for factory reset
#define APP_COMMISSIONING_BY_LONG_PRESS       TRUE
#define APP_COMMISSIONING_BY_LONG_PRESS_PORT  0x04  // Port P2 for commissioning mode

//==============================================================================
// I2C (BME280 Sensor)
//==============================================================================
#define BME280_32BIT_ENABLE          // 32-bit mode for BME280

#define OCM_CLK_PORT  1              // I2C SCL port
#define OCM_CLK_PIN   6              // I2C SCL pin

#define OCM_DATA_PORT 1              // I2C SDA port
#define OCM_DATA_PIN  7              // I2C SDA pin



//==============================================================================
// ADC
//==============================================================================
#define READ_ADC_PORT 0
#define READ_ADC_PIN  0

//==============================================================================
// UART and Memory
//==============================================================================
#define HAL_UART        TRUE
#define HAL_UART_ISR    TRUE
#define HAL_UART_DMA    FALSE

#define INT_HEAP_LEN    (2048)       // Heap size in bytes

//==============================================================================
// Board Configuration
//==============================================================================
#include "hal_board_cfg.h"

#endif // PREINCLUDE_H