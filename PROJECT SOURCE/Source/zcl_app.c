//==============================================================================
// zcl_app.c
//==============================================================================
// ZCL Application main implementation
//==============================================================================

#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"

#include "gp_interface.h"

#include "Debug.h"

#include "OnBoard.h"

#include "commissioning.h"
#include "factory_reset.h"

/* HAL */
#include "bme280.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_i2c.h"
#include "hal_key.h"
#include "senseair2.h"
#include "utils.h"


//==============================================================================
// Constants
//==============================================================================

#define STANDARD_PRESSURE_HPA 1013.25

//==============================================================================
// Macros
//==============================================================================

#define myround(x) ((int)((x) + (((x) >= 0) ? 0.5 : -0.5)))

//==============================================================================
// Global Variables
//==============================================================================

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

uint32 readPause = 200;
static uint8 currentSensorsSendingPhase = 0;
bool pushBut = false;
uint8 enableDisableABC_Old = 0;
uint16 CO2_t;
bool initOut = false;
int16 sendInitReportCount = 0;
bool allRep = false;
bool allRep2 = false;
uint8 zone = 3;
uint8 lastzone = 3;
bool identifyStart = 0;

float raw_co2 = 0;
float pressure = 0;


//==============================================================================
// User Delay Function
//==============================================================================
void user_delay_ms(uint32_t period);

void user_delay_ms(uint32_t period)
{
    MicroWait(period * 1000);
}

//==============================================================================
// Local Variables
//==============================================================================

// BME280 device structure
struct bme280_dev bme_dev = {
    .dev_id = BME280_I2C_ADDR_SEC,
    .intf = BME280_I2C_INTF,
    .read = I2C_ReadMultByte,
    .write = I2C_WriteMultByte,
    .delay_ms = user_delay_ms
};

uint8 SeqNum = 0x00;
afAddrType_t inderect_DstAddr = {
    .addrMode = (afAddrMode_t)AddrNotPresent,
    .endPoint = 0,
    .addr.shortAddr = 0
};


#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
//#define DEVICE_POLL_RATE                 7000   // Poll rate for end device
#endif

//==============================================================================
// Local Function Prototypes
//==============================================================================

static void zclApp_Report(void);
static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static void zclApp_ReadSensors(void);
static void zclApp_Req(void);
static void zclApp_CO2(void);
static void zclApp_Rep(void);
static void zclApp_PerformABC(void);
static void zclApp_HandleKeys(byte portAndAction, byte keyCode);
static uint8 zclApp_RequestBME280(struct bme280_dev *dev);
static uint8 zclApp_ReadBME280(struct bme280_dev *dev);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);
static void zclApp_Uart1RxCb(uint8 port, uint8 event);
void PWM5_initTimer1(void);
void PWM5(uint32 freq, uint16 tduty, uint8 channal);
static void zclApp_LedOn(void);
static void zclApp_LedOff(void);
void identifyLight(void);
static void zclApp_ProcessIdentifyTimeChange(uint8 endpoint);
void reportC(void);
void reportT(void);
void reportP(void);
void reportH(void);
void reportF(void);
float correct_co2_for_pressure(float raw_co2_ppm, float measured_pressure_hpa);


//==============================================================================
// ZCL General Profile Callback Table
//==============================================================================

static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB,        // Basic Cluster Reset command
    NULL,                       // Identify Trigger Effect command
    NULL,                       // On/Off cluster commands
    NULL,                       // On/Off cluster enhanced command Off with Effect
    NULL,                       // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                       // On/Off cluster enhanced command On with Timed Off
    NULL,                       // RSSI Location command
    NULL                        // RSSI Location Response command
};

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize ZCL application
 * @param task_id Task ID for event handling
 */
void zclApp_Init(byte task_id)
{
    PWM5_initTimer1();

    PWM5(125000, 0, 4);  // green
    PWM5(125000, 0, 2);  // blue
    PWM5(125000, 0, 3);  // red

    requestNewTrustCenterLinkKey = FALSE;
    zclApp_RestoreAttributesFromNV();
    zclApp_TaskID = task_id;

    zclGeneral_RegisterCmdCallbacks(zclApp_FirstEP.EndPoint, &zclApp_CmdCallbacks);

    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsCount, zclApp_AttrsFirstEP);

    zcl_registerReadWriteCB(zclApp_FirstEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    zcl_registerForMsg(zclApp_TaskID);

    RegisterForKeys(zclApp_TaskID);
    bdb_RegisterIdentifyTimeChangeCB(zclApp_ProcessIdentifyTimeChange);

    IO_IMODE_PORT_PIN(READ_ADC_PORT, READ_ADC_PIN, IO_TRI);

    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);

    HalI2CInit();

    SenseAir_Init(zclApp_Uart1RxCb);
    user_delay_ms(1000);

    if (zclApp_Config.EnableABC) {
        SenseAir_SetABC(TRUE);
    } else {
        SenseAir_SetABC(FALSE);
    }
    enableDisableABC_Old = zclApp_Config.EnableABC;

    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, 15000);
    osal_start_reload_timer(zclApp_TaskID, APP_CO2_EVT, 6567);

    // Startup LED animation - fade in
    for (uint8 j = 127; j > 1; j--) {
        PWM5(125000, j, 4);
        PWM5(125000, j, 2);
        PWM5(125000, j, 3);
        user_delay_ms(10);
    }

    // Startup LED animation - fade out
    for (uint8 j = 1; j < 127; j++) {
        PWM5(125000, j, 4);
        PWM5(125000, j, 2);
        PWM5(125000, j, 3);
        user_delay_ms(20);
    }

    PWM5(125000, 0, 4);
    PWM5(125000, 0, 2);
    PWM5(125000, 0, 3);

    user_delay_ms(500);
    HAL_TURN_ON_LED1();
    user_delay_ms(50);
    HAL_TURN_OFF_LED1();
}

/**
 * @brief Main event loop for ZCL application
 * @param task_id Task ID
 * @param events  Pending events
 * @return Remaining events
 */
uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;
    devStates_t zclApp_NwkState; //---
    
    (void)task_id; // Intentionally unreferenced parameter
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                
                break;
            case ZDO_STATE_CHANGE:
                zclApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                //LREP("NwkState=%d\r\n", zclApp_NwkState);
                if (zclApp_NwkState == DEV_END_DEVICE) {

                } 
                break;
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");

        if (initOut == FALSE) {
            sendInitReportCount++;
            if (sendInitReportCount == 10) {
                initOut = TRUE;
            }
            allRep = TRUE;
        }
        zclApp_Report();

        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_SAVE_ATTRS_EVT) {
        LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }

    if (events & APP_REQ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_Req();
        return (events ^ APP_REQ_SENSORS_EVT);
    }

    if (events & APP_CO2_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_CO2();
        return (events ^ APP_CO2_EVT);
    }

    if (events & APP_REP_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_Rep();
        return (events ^ APP_REP_SENSORS_EVT);
    }

    if (events & APP_LED_EVT) {
        zclApp_LedOff();
        return (events ^ APP_LED_EVT);
    }

    if (events & APP_IDENT_EVT) {
        identifyLight();
        return (events ^ APP_IDENT_EVT);
    }

    return 0;
}

//==============================================================================
// LED Feedback Function (Competitive Algorithm)
//==============================================================================

/**
 * @brief Update LED feedback based on CO2 and Formaldehyde levels
 *        Uses competitive algorithm: selects the worst zone
 */
static void zclApp_LedFeedback(void)
{
    uint8_t zoneCO2 = 0;
    uint8_t zoneCH2O = 0;
    uint8_t zone = 0;

    // Determine zone by CO2 (using configured thresholds)
    if (CO2_t <= zclApp_Config.Threshold1_PPM) {
        zoneCO2 = 0;  // Excellent
    } else if (CO2_t > zclApp_Config.Threshold1_PPM && CO2_t < zclApp_Config.Threshold2_PPM) {
        zoneCO2 = 1;  // Normal
    } else if (CO2_t >= zclApp_Config.Threshold2_PPM) {
        zoneCO2 = 2;  // Bad
    }

    // Determine zone by Formaldehyde (CH2O in ppb)
    if ((uint16)zclApp_Sensors.ch2o <= zclApp_Config.Threshold1_CH2O_ppb) {
        zoneCH2O = 0;  // Excellent (blue)
    } else if ((uint16)zclApp_Sensors.ch2o > zclApp_Config.Threshold1_CH2O_ppb &&
               (uint16)zclApp_Sensors.ch2o < zclApp_Config.Threshold2_CH2O_ppb) {
        zoneCH2O = 1;  // Normal (green)
    } else if ((uint16)zclApp_Sensors.ch2o >= zclApp_Config.Threshold2_CH2O_ppb) {
        zoneCH2O = 2;  // Bad (red)
    }

    // Competitive algorithm: select the worst zone (max value)
    zone = (zoneCO2 > zoneCH2O) ? zoneCO2 : zoneCH2O;

    // Control LED
    if (zclApp_Config.LedFeedback) {
        if (zone != lastzone) {
            // Fade out previous zone
            if (lastzone == 0) {
                for (uint8_t j = 1; j < 127; j++) {
                    PWM5(125000, j, 2);  // Green
                    user_delay_ms(6);
                }
                PWM5(125000, 0, 4);
            } else if (lastzone == 1) {
                for (uint8_t j = 1; j < 127; j++) {
                    PWM5(125000, j, 2);  // Green
                    //PWM5(125000, j, 4);  // Blue
                    PWM5(125000, j, 3);  // Red
                    user_delay_ms(6);
                }
                PWM5(125000, 0, 2);
            } else if (lastzone == 2) {
                for (uint8_t j = 1; j < 127; j++) {
                    PWM5(125000, j, 3);  // Red
                    user_delay_ms(6);
                }
                PWM5(125000, 0, 3);
            }

            lastzone = zone;

            // Fade in new zone
            if (zone == 0) {
                for (uint8_t j = 127; j > 1; j--) {
                    PWM5(125000, j, 2);  // Green
                    user_delay_ms(8);
                }
            } else if (zone == 1) {
                for (uint8_t j = 127; j > 1; j--) {
                    PWM5(125000, j, 2);  // Green
                    //PWM5(125000, j, 4);  // Blue
                    PWM5(125000, j, 3);  // Red
                    user_delay_ms(8);
                }
            } else if (zone == 2) {
                for (uint8_t j = 127; j > 1; j--) {
                    PWM5(125000, j, 3);  // Red
                    user_delay_ms(8);
                }
            }
        }
    } else {
        // Feedback disabled - turn off all LEDs
        PWM5(125000, 0, 4);
        PWM5(125000, 0, 2);
        PWM5(125000, 0, 3);
        lastzone = 3;
    }
}

//==============================================================================
// Local Functions
//==============================================================================

/**
 * @brief Handle key press events
 * @param portAndAction Port and action mask
 * @param keyCode       Key code
 */
static void zclApp_HandleKeys(byte portAndAction, byte keyCode)
{
#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE
    // Separate logic for commissioning vs factory reset
    if (devState == DEV_ROUTER ||
        devState == DEV_END_DEVICE ||
        devState == DEV_END_DEVICE_UNAUTH) {
        // On network: handle factory reset
        zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    } else {
        // Not on network: handle commissioning
        zclCommissioning_HandleKeys(portAndAction, keyCode);
    }
#else
    // Legacy mode: both handlers
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
#endif

    if (portAndAction & HAL_KEY_RELEASE) {
        allRep = TRUE;
        zclApp_Report();
    }
}

/**
 * @brief Perform ABC (Automatic Baseline Correction) if setting changed
 */
static void zclApp_PerformABC(void)
{
    if (zclApp_Config.EnableABC != enableDisableABC_Old) {
        enableDisableABC_Old = zclApp_Config.EnableABC;

        if (zclApp_Config.EnableABC) {
            SenseAir_SetABC(TRUE);
        } else {
            SenseAir_SetABC(FALSE);
        }
    }
}

/**
 * @brief Request measurements from all sensors
 */
static void zclApp_Req(void)
{
    zclApp_RequestBME280(&bme_dev);
    SenseAir_RequestMeasure();
    osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 400);
}

/**
 * @brief Read all sensor values
 */
static void zclApp_ReadSensors(void)
{
    zclApp_ReadBME280(&bme_dev);

    tempCH20 = adcReadSampled(READ_ADC_PIN, HAL_ADC_RESOLUTION_14, HAL_ADC_REF_AVDD, 5);
    zclApp_Sensors.ch2o = mapRange(990 + zclApp_Config.ch2oOffset, 5107 + zclApp_Config.ch2oOffset, 0, 5000, tempCH20);
    zclApp_Sensors.ch2o = (float)myround(zclApp_Sensors.ch2o);
    //zclApp_Sensors.ch2o = tempCH20;
}

/**
 * @brief Trigger CO2 measurement cycle
 */
static void zclApp_CO2(void)
{
    osal_start_timerEx(zclApp_TaskID, APP_REQ_SENSORS_EVT, 200);
}

/**
 * @brief Trigger sensor reporting
 */
static void zclApp_Report(void)
{
    if ((devState == DEV_ROUTER || devState == DEV_END_DEVICE) && allRep == TRUE) {
        zclApp_LedOn();
    }

    if (sendInitReportCount == 0) {
        initOut = FALSE;
    }

    osal_start_timerEx(zclApp_TaskID, APP_REP_SENSORS_EVT, 200);
}

/**
 * @brief Basic Cluster Reset callback
 */
static void zclApp_BasicResetCB(void)
{
    LREPMaster("BasicResetCB\r\n");
    zclApp_ResetAttributesToDefaultValues();
    zclApp_SaveAttributesToNV();
}

/**
 * @brief Read/Write authorization callback
 */
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper)
{
    LREPMaster("AUTH CB called\r\n");
    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}

/**
 * @brief Save configuration attributes to Non-Volatile memory
 */
static void zclApp_SaveAttributesToNV(void)
{
    uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    LREP("Saving attributes to NV write=%d\r\n", writeStatus);

    zclApp_LedFeedback();
}

/**
 * @brief Restore configuration attributes from Non-Volatile memory
 */
static void zclApp_RestoreAttributesFromNV(void)
{
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    LREP("Restoring attributes from NV status=%d\r\n", status);

    if (status == NV_ITEM_UNINIT) {
        uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
        LREP("NV was empty, writing %d\r\n", writeStatus);
    }

    if (status == ZSUCCESS) {
        LREPMaster("Reading from NV\r\n");
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}



/**
 * @brief Initialize and request measurement from BME280 sensor
 * @param dev BME280 device structure
 * @return 0 on success, 1 on error
 */
static uint8 zclApp_RequestBME280(struct bme280_dev *dev)
{
    int8_t rslt = bme280_init(dev);

    if (rslt == BME280_OK) {
        uint8_t settings_sel;
        dev->settings.osr_h = BME280_OVERSAMPLING_16X;
        dev->settings.osr_p = BME280_OVERSAMPLING_16X;
        dev->settings.osr_t = BME280_OVERSAMPLING_16X;
        dev->settings.filter = BME280_FILTER_COEFF_16;
        dev->settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

        settings_sel = BME280_OSR_PRESS_SEL;
        settings_sel |= BME280_OSR_TEMP_SEL;
        settings_sel |= BME280_OSR_HUM_SEL;
        settings_sel |= BME280_STANDBY_SEL;
        settings_sel |= BME280_FILTER_SEL;

        rslt = bme280_set_sensor_settings(settings_sel, dev);
        rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    } else {
        zclApp_LedOn();
        return 1;
    }

    return 0;
}

/**
 * @brief Read data from BME280 sensor
 * @param dev BME280 device structure
 * @return 0 on success, 1 on error
 */
static uint8 zclApp_ReadBME280(struct bme280_dev *dev)
{
    struct bme280_data bme_results;
    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &bme_results, dev);

    if (rslt == BME280_OK) {
        LREP("ReadBME280 t=%ld, p=%ld, h=%ld\r\n",
             bme_results.temperature, bme_results.pressure, bme_results.humidity);

        zclApp_Sensors.BME280_HumiditySensor_MeasuredValue =
            (uint16)(bme_results.humidity * 100 / 1024.0) + 700 + zclApp_Config.HumidityOffset*100;

        zclApp_Sensors.BME280_PressureSensor_ScaledValue =
            (int16)(pow(10.0, (double)zclApp_Sensors.BME280_PressureSensor_Scale) * (double)bme_results.pressure);

        zclApp_Sensors.BME280_PressureSensor_MeasuredValue =
            (int16)(zclApp_Sensors.BME280_PressureSensor_ScaledValue / 10.0)+zclApp_Config.PressureOffset;

        zclApp_Sensors.Temperature =
            (int16)bme_results.temperature - 220 + zclApp_Config.TemperatureOffset * 10;

        zclApp_Sensors.BME280_PressureSensor_raw = (int16)(bme_results.pressure / 100);
        pressure = (float)zclApp_Sensors.BME280_PressureSensor_raw;
    } else {
        zclApp_LedOn();
        return 1;
    }

    return 0;
}

/**
 * @brief Correct CO2 reading based on atmospheric pressure
 * @param raw_co2_ppm         Raw CO2 reading in ppm
 * @param measured_pressure_hpa Measured pressure in hPa
 * @return Pressure-corrected CO2 value
 */
float correct_co2_for_pressure(float raw_co2_ppm, float measured_pressure_hpa)
{
    if (measured_pressure_hpa < 500.0 || measured_pressure_hpa > 1100.0) {
        return raw_co2_ppm;
    }

    float corrected_co2 = raw_co2_ppm * (STANDARD_PRESSURE_HPA / measured_pressure_hpa);
    return corrected_co2;
}

/**
 * @brief Sequential sensor reporting (round-robin)
 */
static void zclApp_Rep(void)
{
    switch (currentSensorsSendingPhase++) {
    case 0:
        reportT();
        break;

    case 1:
        reportH();
        break;

    case 2:
        reportP();
        break;

    case 3:
        reportC();
        break;

    case 4:
        reportF();
        break;

    default:
        currentSensorsSendingPhase = 0;
        if (allRep == TRUE) {
            allRep = FALSE;
        }
        break;
    }

    if (currentSensorsSendingPhase != 0) {
        osal_start_timerEx(zclApp_TaskID, APP_REP_SENSORS_EVT, 50);
    }
}

/**
 * @brief UART RX callback for Sensair CO2 sensor
 * @param port UART port
 * @param event Event type
 */
static void zclApp_Uart1RxCb(uint8 port, uint8 event)
{
    uint8 i = 0;
    uint8 ch;

    while (Hal_UART_RxBufLen(port)) {
        HalUARTRead(port, &ch, 1);
        zclApp_StateText[i] = ch;
        i++;
    }

    const uint8 length = zclApp_StateText[2];

    if (zclApp_StateText[0] == 0xFE && zclApp_StateText[1] == 0x04) {
        CO2_t = ((zclApp_StateText[length + 1]) << 8) | zclApp_StateText[length + 2];
        zclApp_Sensors.CO2 = (float)CO2_t / 1000000.0;
        raw_co2 = (float)CO2_t;
    }

    if (zclApp_Config.accurate_co2) {
        zclApp_Sensors.CO2 = correct_co2_for_pressure((float)CO2_t, pressure) / 1000000.0;
    } else {
        zclApp_Sensors.CO2 = (float)CO2_t / 1000000.0;
    }

    zclApp_LedFeedback();
    zclApp_PerformABC();
}

//==============================================================================
// PWM Functions
//==============================================================================

/**
 * @brief Initialize Timer1 for PWM output
 */
void PWM5_initTimer1(void)
{
    // Configure P0.4, P0.5, P0.6 as outputs
    P0DIR |= 0x70;  // 0x70 = 0111 0000

    PERCFG &= ~0x40;    // Set timer 1 to alternative location
    PERCFG |= 0x03;     // Set USART0, USART1 to alternative location 2
    P2DIR &= ~0xC0;     // Configure timer 1 priority
    P0SEL |= 0x70;      // Configure P0.4, P0.5, P0.6 as peripheral function
}

/**
 * @brief Generate PWM signal on specified channel
 * @param freq   PWM frequency
 * @param tduty  Duty cycle value (0-65535)
 * @param channal PWM channel (1-4)
 */
void PWM5(uint32 freq, uint16 tduty, uint8 channal)
{
    T1CTL &= ~(BV(1) | BV(0));  // Suspend timer 1

    uint16 tfreq;

    if (freq <= 32768) {
        tfreq = (uint16)(2000000 / freq);
    } else {
        tfreq = (uint16)(16000000 / freq);
    }

    // PWM signal period
    uint8 tfreq_l = (uint8)tfreq;
    uint8 tfreq_h = (uint8)(tfreq >> 8);
    T1CC0L = tfreq_l;
    T1CC0H = tfreq_h;

    // PWM duty cycle
    uint8 tduty_l = (uint8)tduty;
    uint8 tduty_h = (uint8)(tduty >> 8);

    switch (channal) {
    case 1:
        T1CC1H = tduty_h;
        T1CC1L = tduty_l;
        T1CCTL1 = 0x1C;
        break;
    case 2:
        T1CC2H = tduty_h;
        T1CC2L = tduty_l;
        T1CCTL2 = 0x1C;
        break;
    case 3:
        T1CC3H = tduty_h;
        T1CC3L = tduty_l;
        T1CCTL3 = 0x1C;
        break;
    case 4:
        T1CC4H = tduty_h;
        T1CC4L = tduty_l;
        T1CCTL4 = 0x1C;
        break;
    default:
        break;
    }

    if (freq <= 32768) {
        T1CTL |= (BV(2) | 0x03);  // Up-down mode, tick frequency / 8
    } else {
        T1CTL = 0x03;              // Up-down mode, tick frequency / 1
    }
}

//==============================================================================
// LED Control Functions
//==============================================================================

/**
 * @brief Turn on LED1 with timer to auto-turn-off
 */
static void zclApp_LedOn(void)
{
    HAL_TURN_ON_LED1();
    osal_start_timerEx(zclApp_TaskID, APP_LED_EVT, 30);
}

/**
 * @brief Turn off LED1
 */
static void zclApp_LedOff(void)
{
    HAL_TURN_OFF_LED1();
}

//==============================================================================
// Identify Functions
//==============================================================================

/**
 * @brief Process identify time change callback
 * @param endpoint Endpoint number
 */
static void zclApp_ProcessIdentifyTimeChange(uint8 endpoint)
{
    (void)endpoint;

    if (zclApp_IdentifyTime > 0) {
        if (identifyStart == 0) {
            PWM5(125000, 0, 4);
            PWM5(125000, 0, 2);
            PWM5(125000, 0, 3);
            identifyStart = 1;
            identifyLight();
        }
    } else {
        if (identifyStart == 1) {
            osal_stop_timerEx(zclApp_TaskID, APP_IDENT_EVT);
            osal_clear_event(zclApp_TaskID, APP_IDENT_EVT);
            allRep = TRUE;
            lastzone = 3;
            identifyStart = 0;
            PWM5(125000, 0, 4);
            PWM5(125000, 0, 2);
            PWM5(125000, 0, 3);
            zclApp_Report();
        }
    }
}

/**
 * @brief Identify light effect (RGB cycle)
 */
void identifyLight(void)
{
    for (uint8 j = 127; j > 1; j--) {
        PWM5(125000, j, 4);
        PWM5(125000, j, 2);
        PWM5(125000, j, 3);
        user_delay_ms(3);
    }

    for (uint8 j = 1; j < 127; j++) {
        PWM5(125000, j, 4);
        PWM5(125000, j, 2);
        PWM5(125000, j, 3);
        user_delay_ms(12);
    }

    PWM5(125000, 0, 4);
    PWM5(125000, 0, 2);
    PWM5(125000, 0, 3);

    osal_start_timerEx(zclApp_TaskID, APP_IDENT_EVT, 200);
}

//==============================================================================
// Report Functions
//==============================================================================

/**
 * @brief Report Temperature cluster attributes
 */
void reportT(void)
{
    if (allRep == TRUE) {
        const uint8 NUM_ATTRIBUTES = 2;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));

        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;

            pReportCmd->attrList[0].attrID = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_INT16;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.Temperature);

            pReportCmd->attrList[1].attrID = ATTRID_TemperatureOffset;
            pReportCmd->attrList[1].dataType = ZCL_INT16;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.TemperatureOffset);

            zcl_SendReportCmd(1, &inderect_DstAddr, TEMP, pReportCmd,
                             ZCL_FRAME_SERVER_CLIENT_DIR, TRUE, SeqNum++);
        }
        osal_mem_free(pReportCmd);
    } else {
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    }
}

/**
 * @brief Report Humidity cluster attributes
 */
void reportH(void)
{
    if (allRep == TRUE) {
        const uint8 NUM_ATTRIBUTES = 2;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));

        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;

            pReportCmd->attrList[0].attrID = ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_UINT16;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.BME280_HumiditySensor_MeasuredValue);

            pReportCmd->attrList[1].attrID = ATTRID_HumidityOffset;
            pReportCmd->attrList[1].dataType = ZCL_INT16;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.HumidityOffset);

            zcl_SendReportCmd(1, &inderect_DstAddr, HUMIDITY, pReportCmd,
                             ZCL_FRAME_SERVER_CLIENT_DIR, TRUE, SeqNum++);
        }
        osal_mem_free(pReportCmd);
    } else {
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
    }
}

/**
 * @brief Report Pressure cluster attributes
 */
void reportP(void)
{
    if (allRep == TRUE) {
        const uint8 NUM_ATTRIBUTES = 4;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));

        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;

            pReportCmd->attrList[0].attrID = ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_INT16;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.BME280_PressureSensor_MeasuredValue);

            pReportCmd->attrList[1].attrID = ATTRID_MS_PRESSURE_MEASUREMENT_SCALED_VALUE;
            pReportCmd->attrList[1].dataType = ZCL_INT16;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Sensors.BME280_PressureSensor_ScaledValue);

            pReportCmd->attrList[2].attrID = ATTRID_MS_PRESSURE_MEASUREMENT_SCALE;
            pReportCmd->attrList[2].dataType = ZCL_INT8;
            pReportCmd->attrList[2].attrData = (void *)(&zclApp_Sensors.BME280_PressureSensor_Scale);

            pReportCmd->attrList[3].attrID = ATTRID_PressureOffset;
            pReportCmd->attrList[3].dataType = ZCL_INT32;
            pReportCmd->attrList[3].attrData = (void *)(&zclApp_Config.PressureOffset);

            zcl_SendReportCmd(1, &inderect_DstAddr, PRESSURE, pReportCmd,
                             ZCL_FRAME_SERVER_CLIENT_DIR, TRUE, SeqNum++);
        }
        osal_mem_free(pReportCmd);
    } else {
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, PRESSURE, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE);
    }
}

/**
 * @brief Report CO2 cluster attributes
 */
void reportC(void)
{
    if (allRep == TRUE) {
        const uint8 NUM_ATTRIBUTES = 6;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));

        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;

            pReportCmd->attrList[0].attrID = ATTRID_CO2_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.CO2);

            pReportCmd->attrList[1].attrID = ATTRID_ENABLE_ABC;
            pReportCmd->attrList[1].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.EnableABC);

            pReportCmd->attrList[2].attrID = ATTRID_LED_FEEDBACK;
            pReportCmd->attrList[2].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[2].attrData = (void *)(&zclApp_Config.LedFeedback);

            pReportCmd->attrList[3].attrID = ATTRID_THRESHOLD1;
            pReportCmd->attrList[3].dataType = ZCL_UINT16;
            pReportCmd->attrList[3].attrData = (void *)(&zclApp_Config.Threshold1_PPM);

            pReportCmd->attrList[4].attrID = ATTRID_THRESHOLD2;
            pReportCmd->attrList[4].dataType = ZCL_UINT16;
            pReportCmd->attrList[4].attrData = (void *)(&zclApp_Config.Threshold2_PPM);

            pReportCmd->attrList[5].attrID = ATTRID_CO2_ACCURATE_MEASURED_VALUE;
            pReportCmd->attrList[5].dataType = ZCL_BOOL;
            pReportCmd->attrList[5].attrData = (void *)(&zclApp_Config.accurate_co2);

            zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_CO2, pReportCmd,
                             ZCL_FRAME_SERVER_CLIENT_DIR, TRUE, SeqNum++);
        }
        osal_mem_free(pReportCmd);
    } else {
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ZCL_CO2, ATTRID_CO2_MEASURED_VALUE);
    }
}

/**
 * @brief Report Formaldehyde cluster attributes
 */
void reportF(void)
{
    if (allRep == TRUE) {
        const uint8 NUM_ATTRIBUTES = 4;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));

        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;

            pReportCmd->attrList[0].attrID = ATTRID_FORMALDEHYDE_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.ch2o);

            pReportCmd->attrList[1].attrID = ATTRID_FORMALDEHYDE_Offset;
            pReportCmd->attrList[1].dataType = ZCL_INT8;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Config.ch2oOffset);

            pReportCmd->attrList[2].attrID = ATTRID_THRESHOLD1;
            pReportCmd->attrList[2].dataType = ZCL_UINT16;
            pReportCmd->attrList[2].attrData = (void *)(&zclApp_Config.Threshold1_CH2O_ppb);

            pReportCmd->attrList[3].attrID = ATTRID_THRESHOLD2;
            pReportCmd->attrList[3].dataType = ZCL_UINT16;
            pReportCmd->attrList[3].attrData = (void *)(&zclApp_Config.Threshold2_CH2O_ppb);

            zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_FORMALDEHYDE, pReportCmd,
                             ZCL_FRAME_SERVER_CLIENT_DIR, TRUE, SeqNum++);
        }
        osal_mem_free(pReportCmd);
    } else {
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ZCL_FORMALDEHYDE, ATTRID_FORMALDEHYDE_MEASURED_VALUE);
    }
}