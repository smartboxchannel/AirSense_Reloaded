//==============================================================================
// commissioning.h
//==============================================================================
// ZigBee device commissioning and network recovery module
//==============================================================================

#ifndef COMMISSIONING_H
#define COMMISSIONING_H

//==============================================================================
// Event Masks
//==============================================================================
#define APP_COMMISSIONING_CLOCK_DOWN_POLING_RATE_EVT    0x0001
#define APP_COMMISSIONING_END_DEVICE_REJOIN_EVT         0x0002
#define APP_COMMISSIONING_BY_LONG_PRESS_EVT             0x0004
#define APP_COMMISSIONING_OFF_EVT                       0x0008

//==============================================================================
// Rejoin Configuration
//==============================================================================
#define APP_COMMISSIONING_END_DEVICE_REJOIN_MAX_DELAY    ((uint32)600000)  // 10 minutes max
#define APP_COMMISSIONING_END_DEVICE_REJOIN_START_DELAY  (10 * 1000)       // 10 seconds initial
#define APP_COMMISSIONING_END_DEVICE_REJOIN_BACKOFF      ((float)1.2)      // Exponential backoff factor
#define APP_COMMISSIONING_END_DEVICE_REJOIN_TRIES        22               // Maximum rejoin attempts

//==============================================================================
// Long Press Commissioning
//==============================================================================
#ifndef APP_COMMISSIONING_BY_LONG_PRESS
    #define APP_COMMISSIONING_BY_LONG_PRESS     FALSE
#endif

#ifndef APP_COMMISSIONING_BY_LONG_PRESS_PORT
    #define APP_COMMISSIONING_BY_LONG_PRESS_PORT 0x00    // Port mask for commissioning
#endif

#ifndef APP_COMMISSIONING_HOLD_TIME_FAST
    #define APP_COMMISSIONING_HOLD_TIME_FAST     1000    // 1 second hold time
#endif

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize commissioning module
 * @param task_id Task ID for event handling
 */
extern void zclCommissioning_Init(uint8 task_id);

/**
 * @brief Main event loop handler
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
extern uint16 zclCommissioning_event_loop(uint8 task_id, uint16 events);

/**
 * @brief Control sleep/polling behavior for power saving
 * @param allow TRUE to enable power saving, FALSE to disable
 */
extern void zclCommissioning_Sleep(uint8 allow);

/**
 * @brief Handle key press events for commissioning
 * @param portAndAction Port and action mask
 * @param keyCode       Key code (unused)
 */
extern void zclCommissioning_HandleKeys(uint8 portAndAction, uint8 keyCode);

#endif // COMMISSIONING_H