//==============================================================================
// factory_reset.c
//==============================================================================
// Factory reset functionality implementation
//==============================================================================

#include "factory_reset.h"
#include "AF.h"
#include "OnBoard.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "ZComDef.h"
#include "hal_key.h"
#include "zcl_app.h"
#include "ZDApp.h"

//==============================================================================
// External Variables
//==============================================================================
extern devStates_t devState;

//==============================================================================
// Static Function Prototypes
//==============================================================================
static void zclFactoryResetter_ResetToFN(void);
static void zclFactoryResetter_ProcessBootCounter(void);
static void zclFactoryResetter_ResetBootCounter(void);
static void zclApp_LedOnF(void);
static void zclApp_LedOffF(void);

//==============================================================================
// Static Variables
//==============================================================================
static uint8 zclFactoryResetter_TaskID;

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize factory reset module
 * @param task_id Task ID for event handling
 */
void zclFactoryResetter_Init(uint8 task_id)
{
    zclFactoryResetter_TaskID = task_id;

#if FACTORY_RESET_BY_BOOT_COUNTER
    zclFactoryResetter_ProcessBootCounter();
#endif
}

/**
 * @brief Main event loop handler
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events)
{
    if (events & FACTORY_RESET_EVT) {
        zclFactoryResetter_ResetToFN();
        return (events ^ FACTORY_RESET_EVT);
    }

    if (events & FACTORY_BOOTCOUNTER_RESET_EVT) {
        zclFactoryResetter_ResetBootCounter();
        return (events ^ FACTORY_BOOTCOUNTER_RESET_EVT);
    }

    if (events & FACTORY_LED_EVT) {
        zclApp_LedOnF();
        return (events ^ FACTORY_LED_EVT);
    }

    if (events & FACTORY_LEDOFF_EVT) {
        zclApp_LedOffF();
        return (events ^ FACTORY_LEDOFF_EVT);
    }

    return 0;
}

/**
 * @brief Handle key press events for factory reset detection
 * @param portAndAction Port and action mask
 * @param keyCode       Key code (unused)
 */
void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode)
{
#if FACTORY_RESET_BY_LONG_PRESS
    // Check if device is in a valid network state
    if (devState != DEV_ROUTER &&
        devState != DEV_END_DEVICE &&
        devState != DEV_END_DEVICE_UNAUTH)
    {
        // Device not on network - ignore factory reset
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
        HAL_TURN_OFF_LED1();
        return;
    }

    // Key release - cancel pending factory reset
    if (portAndAction & HAL_KEY_RELEASE) {
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
        HAL_TURN_OFF_LED1();
    } else {
        // Key press - start timer for factory reset
        bool isCorrectPort = true;

#if FACTORY_RESET_BY_LONG_PRESS_PORT
        isCorrectPort = (FACTORY_RESET_BY_LONG_PRESS_PORT & portAndAction) != 0;
#endif

        if (isCorrectPort) {
            uint32 timeout = FACTORY_RESET_HOLD_TIME_LONG;

            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, timeout);
            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 2000);
        }
    }
#endif
}

//==============================================================================
// Static Functions
//==============================================================================

/**
 * @brief Perform factory reset to factory new state
 */
static void zclFactoryResetter_ResetToFN(void)
{
    HAL_TURN_ON_LED1();

    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);

    bdb_resetLocalAction();
}

/**
 * @brief Reset boot counter to zero
 */
static void zclFactoryResetter_ResetBootCounter(void)
{
    uint16 bootCnt = 0;
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

/**
 * @brief Process boot counter and trigger factory reset if max value exceeded
 */
static void zclFactoryResetter_ProcessBootCounter(void)
{
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT, FACTORY_RESET_BOOTCOUNTER_RESET_TIME);

    uint16 bootCnt = 0;

    if (osal_nv_item_init(ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt) == ZSUCCESS) {
        osal_nv_read(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    }

    bootCnt += 1;

    if (bootCnt >= FACTORY_RESET_BOOTCOUNTER_MAX_VALUE) {
        bootCnt = 0;
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT);
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, 5000);
    }

    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

/**
 * @brief Turn LED on and schedule turn-off
 */
static void zclApp_LedOnF(void)
{
    HAL_TURN_ON_LED1();
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT, 50);
}

/**
 * @brief Turn LED off and schedule next blink
 */
static void zclApp_LedOffF(void)
{
    HAL_TURN_OFF_LED1();
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 1000);
}