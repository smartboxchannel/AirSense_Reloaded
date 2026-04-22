
from typing import Final

from zigpy.profiles import zha
from zigpy.quirks import CustomCluster
from zigpy.quirks.v2 import (
    QuirkBuilder,
    ReportingConfig,
    SensorDeviceClass,
    SensorStateClass,
)
from zigpy.quirks.v2.homeassistant.number import NumberDeviceClass
import zigpy.types as t
from zigpy.zcl import ClusterType
from zigpy.zcl.foundation import ZCLAttributeDef
from zigpy.zcl.clusters.general import Basic
from zigpy.zcl.clusters.measurement import (
    CarbonDioxideConcentration,
    FormaldehydeConcentration,
    RelativeHumidity,
    TemperatureMeasurement,
    PressureMeasurement,
)
from zigpy.quirks.v2.homeassistant import (
    UnitOfTemperature,
    UnitOfPressure,
    CONCENTRATION_PARTS_PER_MILLION,
    PERCENTAGE,
)

EFEKTA = "EfektaLab"


class CO2Measurement(CarbonDioxideConcentration, CustomCluster):
    class AttributeDefs(CarbonDioxideConcentration.AttributeDefs):
        co2_accurate_measurement: Final = ZCLAttributeDef(id=0xF111, type=t.Bool, access="rw")
        co2_automatic_calibration: Final = ZCLAttributeDef(id=0x0202, type=t.Bool, access="rw")
        led_indication: Final = ZCLAttributeDef(id=0x0203, type=t.Bool, access="rw")
        co2_moderate_threshold: Final = ZCLAttributeDef(id=0x0204, type=t.uint16_t, access="rw")
        co2_hazardous_threshold: Final = ZCLAttributeDef(id=0x0205, type=t.uint16_t, access="rw")

class CH2OMeasurement(FormaldehydeConcentration, CustomCluster):
    class AttributeDefs(FormaldehydeConcentration.AttributeDefs):
        formaldehyde_moderate_threshold: Final = ZCLAttributeDef(id=0x204, type=t.uint16_t, access="rw")
        formaldehyde_hazardous_threshold: Final = ZCLAttributeDef(id=0x0205, type=t.uint16_t, access="rw")
        formaldehyde_offset: Final = ZCLAttributeDef(id=0x0210, type=t.int8s, access="rw")


class TempMeasurement(TemperatureMeasurement, CustomCluster):
    class AttributeDefs(TemperatureMeasurement.AttributeDefs):
        temperature_offset: Final = ZCLAttributeDef(id=0x0210, type=t.int16s, access="rw")


class RHMeasurement(RelativeHumidity, CustomCluster):
    class AttributeDefs(RelativeHumidity.AttributeDefs):
        humidity_offset: Final = ZCLAttributeDef(id=0x0210, type=t.int16s, access="rw")
               

class PressMeasurement(PressureMeasurement, CustomCluster):
    class AttributeDefs(PressureMeasurement.AttributeDefs):
        pressure_offset: Final = ZCLAttributeDef(id=0x0210, type=t.int32s, access="rw")


(
    QuirkBuilder(EFEKTA, "DIYRuZ_AirSense_Reloaded")
    .replaces_endpoint(1, device_type=zha.DeviceType.SIMPLE_SENSOR)
    .replaces(Basic, endpoint_id=1)
    .replaces(CO2Measurement, endpoint_id=1)
    .replaces(CH2OMeasurement, endpoint_id=1)
    .replaces(TempMeasurement, endpoint_id=1)
    .replaces(RHMeasurement, endpoint_id=1)
    .replaces(PressMeasurement, endpoint_id=1)
    .switch(
        CO2Measurement.AttributeDefs.co2_accurate_measurement.name,
        CO2Measurement.cluster_id,
        endpoint_id=1,
        translation_key="co2_accurate_measurement",
        fallback_name="Enable/disable accurate CO2 measurement mode",
        unique_id_suffix="co2_accurate_measurement",
    )
    .switch(
        CO2Measurement.AttributeDefs.co2_automatic_calibration.name,
        CO2Measurement.cluster_id,
        endpoint_id=1,
        translation_key="co2_automatic_calibration",
        fallback_name="Enable/disable automatic calibration of CO2 sensor",
        unique_id_suffix="co2_automatic_calibration",
    )
    .switch(
        CO2Measurement.AttributeDefs.led_indication.name,
        CO2Measurement.cluster_id,
        endpoint_id=1,
        translation_key="led_indication",
        fallback_name="Enable/disable LED indication",
        unique_id_suffix="led_indication",
    )
    .number(
        CO2Measurement.AttributeDefs.co2_moderate_threshold.name,
        CO2Measurement.cluster_id,
        endpoint_id=1,
        translation_key="co2_moderate_threshold",
        fallback_name="Threshold for moderate CO2 level",
        unique_id_suffix="co2_moderate_threshold",
        min_value=0,
        max_value=5000,
        step=1,
        unit=CONCENTRATION_PARTS_PER_MILLION,
    )
    .number(
        CO2Measurement.AttributeDefs.co2_hazardous_threshold.name,
        CO2Measurement.cluster_id,
        endpoint_id=1,
        translation_key="co2_hazardous_threshold",
        fallback_name="Threshold for hazardous CO2 level",
        unique_id_suffix="co2_hazardous_threshold",
        min_value=0,
        max_value=5000,
        step=1,
        unit=CONCENTRATION_PARTS_PER_MILLION,
    )
    .number(
        CH2OMeasurement.AttributeDefs.formaldehyde_moderate_threshold.name,
        CH2OMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="formaldehyde_moderate_threshold",
        fallback_name="Threshold for moderate formaldehyde (CH2O) level",
        unique_id_suffix="formaldehyde_moderate_threshold",
        min_value=0,
        max_value=5000,
        step=1,
        unit="ppb",
    )
    .number(
        CH2OMeasurement.AttributeDefs.formaldehyde_hazardous_threshold.name,
        CH2OMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="formaldehyde_hazardous_threshold",
        fallback_name="Threshold for hazardous formaldehyde (CH2O) level",
        unique_id_suffix="formaldehyde_hazardous_threshold",
        min_value=0,
        max_value=5000,
        step=1,
        unit="ppb",
    )
    .number(
        TempMeasurement.AttributeDefs.temperature_offset.name,
        TempMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="temperature_offset",
        fallback_name="Adjust temperature",
        unique_id_suffix="temperature_offset",
        min_value=-50,
        max_value=50,
        step=0.1,
        multiplier=0.1,
        device_class=NumberDeviceClass.TEMPERATURE,
        unit=UnitOfTemperature.CELSIUS,
        mode="box",
    )
    .number(
        RHMeasurement.AttributeDefs.humidity_offset.name,
        RHMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="humidity_offset",
        fallback_name="Adjust humidity",
        unique_id_suffix="humidity_offset",
        min_value=-50,
        max_value=50,
        step=1,
        device_class=NumberDeviceClass.HUMIDITY,
        unit=PERCENTAGE,
        mode="box",
    )
    .number(
        PressMeasurement.AttributeDefs.pressure_offset.name,
        PressMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="pressure_offset",
        fallback_name="Adjust pressure",
        unique_id_suffix="pressure_offset",
        min_value=-1000,
        max_value=1000,
        step=1,
        device_class=NumberDeviceClass.PRESSURE,
        unit=UnitOfPressure.HPA,
        mode="box",
    )
    .number(
        CH2OMeasurement.AttributeDefs.formaldehyde_offset.name,
        CH2OMeasurement.cluster_id,
        endpoint_id=1,
        translation_key="formaldehyde_offset",
        fallback_name="Formaldehyde ADC offset",
        unique_id_suffix="formaldehyde_offset",
        min_value=-100,
        max_value=100,
        step=1,
        unit="adc",
    )
    .add_to_registry()
)