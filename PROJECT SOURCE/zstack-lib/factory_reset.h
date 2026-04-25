//==============================================================================
// factory_reset.h
//==============================================================================
// Factory reset functionality with long press and boot counter detection
//==============================================================================

#ifndef FACTORY_RESET_H
#define FACTORY_RESET_H

//==============================================================================
// Event Masks
//==============================================================================
#define FACTORY_RESET_EVT                0x1000
#define FACTORY_BOOTCOUNTER_RESET_EVT    0x2000
#define FACTORY_LED_EVT                  0x4000
#define FACTORY_LEDOFF_EVT               0x8000

//==============================================================================
// Timing Configuration
//==============================================================================
#ifndef FACTORY_RESET_HOLD_TIME_LONG
    #define FACTORY_RESET_HOLD_TIME_LONG    ((uint32)9 * 1000)  // 9 seconds for factory reset
#endif

#ifndef FACTORY_RESET_HOLD_TIME_FAST
    #define FACTORY_RESET_HOLD_TIME_FAST    2000                // 2 seconds (unused in current logic)
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_MAX_VALUE
    #define FACTORY_RESET_BOOTCOUNTER_MAX_VALUE    5           // Reset after 5 boot cycles
#endif

#ifndef FACTORY_RESET_BOOTCOUNTER_RESET_TIME
    #define FACTORY_RESET_BOOTCOUNTER_RESET_TIME    (10 * 1000) // 10 seconds delay
#endif

//==============================================================================
// Feature Enables
//==============================================================================
#ifndef FACTORY_RESET_BY_LONG_PRESS
    #define FACTORY_RESET_BY_LONG_PRESS     TRUE
#endif

#ifndef FACTORY_RESET_BY_LONG_PRESS_PORT
    #define FACTORY_RESET_BY_LONG_PRESS_PORT 0x00    // Port mask for reset detection
#endif

#ifndef FACTORY_RESET_BY_BOOT_COUNTER
    #define FACTORY_RESET_BY_BOOT_COUNTER   TRUE
#endif

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize factory reset module
 * @param task_id Task ID for event handling
 */
extern void zclFactoryResetter_Init(uint8 task_id);

/**
 * @brief Main event loop handler
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
extern uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events);

/**
 * @brief Handle key press events for factory reset detection
 * @param portAndAction Port and action mask
 * @param keyCode       Key code (unused)
 */
extern void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode);

#endif // FACTORY_RESET_H