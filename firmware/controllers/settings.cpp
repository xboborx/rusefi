/**
 * @file settings.cpp
 * @brief This file is about configuring engine via the human-readable protocol
 *
 * @date Dec 30, 2012
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "pch.h"

#if ! EFI_UNIT_TEST

#include "eficonsole.h"
#include "trigger_decoder.h"
#include "console_io.h"
#include "idle_thread.h"
#include "alternator_controller.h"
#include "trigger_emulator_algo.h"
#include "value_lookup.h"
#if EFI_RTC
#include "rtc_helper.h"
#endif // EFI_RTC

#if EFI_PROD_CODE
#include "can_hw.h"
#include "rusefi.h"
#include "hardware.h"
#endif // EFI_PROD_CODE

#if EFI_ELECTRONIC_THROTTLE_BODY
#include "electronic_throttle.h"
#endif // EFI_ELECTRONIC_THROTTLE_BODY

#if EFI_INTERNAL_FLASH
#include "flash_main.h"
#endif // EFI_INTERNAL_FLASH

#if EFI_ENGINE_SNIFFER
#include "engine_sniffer.h"
extern int waveChartUsedSize;
extern WaveChart waveChart;
#endif // EFI_ENGINE_SNIFFER

void printSpiState(const engine_configuration_s *engineConfiguration) {
	efiPrintf("spi 1=%s/2=%s/3=%s/4=%s",
		boolToString(engineConfiguration->is_enabled_spi_1),
		boolToString(engineConfiguration->is_enabled_spi_2),
		boolToString(engineConfiguration->is_enabled_spi_3),
		boolToString(engineConfiguration->is_enabled_spi_4));
}

static void printOutputs(const engine_configuration_s *engineConfiguration) {
	efiPrintf("injectionPins: mode %s", getPin_output_mode_e(engineConfiguration->injectionPinMode));
	for (size_t i = 0; i < engineConfiguration->cylindersCount; i++) {
		brain_pin_e brainPin = engineConfiguration->injectionPins[i];
		efiPrintf("injection #%d @ %s", (1 + i), hwPortname(brainPin));
	}

	efiPrintf("ignitionPins: mode %s", getPin_output_mode_e(engineConfiguration->ignitionPinMode));
	for (size_t i = 0; i < engineConfiguration->cylindersCount; i++) {
		brain_pin_e brainPin = engineConfiguration->ignitionPins[i];
		efiPrintf("ignition #%d @ %s", (1 + i), hwPortname(brainPin));
	}

	efiPrintf("idlePin: mode %s @ %s freq=%d", getPin_output_mode_e(engineConfiguration->idle.solenoidPinMode),
			hwPortname(engineConfiguration->idle.solenoidPin), engineConfiguration->idle.solenoidFrequency);
	efiPrintf("malfunctionIndicator: %s mode=%s", hwPortname(engineConfiguration->malfunctionIndicatorPin),
			getPin_output_mode_e(engineConfiguration->malfunctionIndicatorPinMode));

	efiPrintf("fuelPumpPin: mode %s @ %s", getPin_output_mode_e(engineConfiguration->fuelPumpPinMode),
			hwPortname(engineConfiguration->fuelPumpPin));

	efiPrintf("fanPin: mode %s @ %s", getPin_output_mode_e(engineConfiguration->fanPinMode),
			hwPortname(engineConfiguration->fanPin));

	efiPrintf("mainRelay: mode %s @ %s", getPin_output_mode_e(engineConfiguration->mainRelayPinMode),
			hwPortname(engineConfiguration->mainRelayPin));

	efiPrintf("starterRelay: mode %s @ %s", getPin_output_mode_e(engineConfiguration->starterRelayDisablePinMode),
			hwPortname(engineConfiguration->starterRelayDisablePin));

	efiPrintf("alternator field: mode %s @ %s",
			getPin_output_mode_e(engineConfiguration->alternatorControlPinMode),
			hwPortname(engineConfiguration->alternatorControlPin));
}

/**
 * @brief	Prints current engine configuration to human-readable console.
 */
void printConfiguration(const engine_configuration_s *engineConfiguration) {

	efiPrintf("Template %s/%d trigger %s/%s/%d", getEngine_type_e(engineConfiguration->engineType),
			engineConfiguration->engineType, getTrigger_type_e(engineConfiguration->trigger.type),
			getEngine_load_mode_e(engineConfiguration->fuelAlgorithm), engineConfiguration->fuelAlgorithm);


	efiPrintf("configurationVersion=%d", engine->getGlobalConfigurationVersion());

	efiPrintf("rpmHardLimit: %d/operationMode=%d", engineConfiguration->rpmHardLimit,
			getEngineRotationState()->getOperationMode());

	efiPrintf("globalTriggerAngleOffset=%.2f", engineConfiguration->globalTriggerAngleOffset);

	efiPrintf("=== cranking ===");
	efiPrintf("crankingRpm: %d", engineConfiguration->cranking.rpm);
	efiPrintf("cranking injection %s", getInjection_mode_e(engineConfiguration->crankingInjectionMode));

	efiPrintf("cranking timing %.2f", engineConfiguration->crankingTimingAngle);

	efiPrintf("=== ignition ===");

	efiPrintf("ignitionMode: %s/enabled=%s", getIgnition_mode_e(engineConfiguration->ignitionMode),
			boolToString(engineConfiguration->isIgnitionEnabled));
	efiPrintf("timingMode: %s", getTiming_mode_e(engineConfiguration->timingMode));
	if (engineConfiguration->timingMode == TM_FIXED) {
		efiPrintf("fixedModeTiming: %d", (int) engineConfiguration->fixedModeTiming);
	}

	efiPrintf("=== injection ===");
	efiPrintf("injection %s enabled=%s", getInjection_mode_e(engineConfiguration->injectionMode),
			boolToString(engineConfiguration->isInjectionEnabled));

	printOutputs(engineConfiguration);

	efiPrintf("map_avg=%s/wa=%s",
			boolToString(engineConfiguration->isMapAveragingEnabled),
			boolToString(engineConfiguration->isWaveAnalyzerEnabled));

	efiPrintf("isManualSpinningMode=%s/isCylinderCleanupEnabled=%s",
			boolToString(engineConfiguration->isManualSpinningMode),
			boolToString(engineConfiguration->isCylinderCleanupEnabled));

	efiPrintf("clutchUp@%s: %s", hwPortname(engineConfiguration->clutchUpPin),
			boolToString(engine->engineState.clutchUpState));
	efiPrintf("clutchDown@%s: %s", hwPortname(engineConfiguration->clutchDownPin),
			boolToString(engine->engineState.clutchDownState));

	efiPrintf("digitalPotentiometerSpiDevice %d", engineConfiguration->digitalPotentiometerSpiDevice);

	for (int i = 0; i < DIGIPOT_COUNT; i++) {
		efiPrintf("digitalPotentiometer CS%d %s", i,
				hwPortname(engineConfiguration->digitalPotentiometerChipSelect[i]));
	}

#if EFI_PROD_CODE
	printSpiState(engineConfiguration);
#endif // EFI_PROD_CODE
}

static void doPrintConfiguration() {
	printConfiguration(engineConfiguration);
}

static void setFixedModeTiming(int value) {
	engineConfiguration->fixedModeTiming = value;
	doPrintConfiguration();
	incrementGlobalConfigurationVersion();
}

static void setTimingMode(int value) {
	engineConfiguration->timingMode = (timing_mode_e) value;
	doPrintConfiguration();
	incrementGlobalConfigurationVersion();
}

static void setIdleSolenoidFrequency(int value) {
	engineConfiguration->idle.solenoidFrequency = value;
	incrementGlobalConfigurationVersion();
}

static void setInjectionPinMode(int value) {
	engineConfiguration->injectionPinMode = (pin_output_mode_e) value;
	doPrintConfiguration();
}

static void setIgnitionPinMode(int value) {
	engineConfiguration->ignitionPinMode = (pin_output_mode_e) value;
	doPrintConfiguration();
}

static void setIdlePinMode(int value) {
	engineConfiguration->idle.solenoidPinMode = (pin_output_mode_e) value;
	doPrintConfiguration();
}

static void setFuelPumpPinMode(int value) {
	engineConfiguration->fuelPumpPinMode = (pin_output_mode_e) value;
	doPrintConfiguration();
}

static void setMalfunctionIndicatorPinMode(int value) {
	engineConfiguration->malfunctionIndicatorPinMode = (pin_output_mode_e) value;
	doPrintConfiguration();
}

static void setSensorChartMode(int value) {
	engineConfiguration->sensorChartMode = (sensor_chart_e) value;
	doPrintConfiguration();
}

static void printTpsSenser(const char *msg, SensorType sensor, int16_t min, int16_t max, adc_channel_e channel) {
	auto tps = Sensor::get(sensor);
	auto raw = Sensor::getRaw(sensor);

	if (!tps.Valid) {
		efiPrintf("TPS not valid");
	}

	char pinNameBuffer[16];

	efiPrintf("tps min (closed) %d/max (full) %d v=%.2f @%s", min, max,
			raw, getPinNameByAdcChannel(msg, channel, pinNameBuffer));


	efiPrintf("current 10bit=%d value=%.2f", convertVoltageTo10bitADC(raw), tps.value_or(0));
}

void printTPSInfo(void) {
	efiPrintf("pedal up %f / down %f",
			engineConfiguration->throttlePedalUpVoltage,
			engineConfiguration->throttlePedalWOTVoltage);

	auto pps = Sensor::get(SensorType::AcceleratorPedal);

	if (!pps.Valid) {
		efiPrintf("PPS not valid");
	}

	printTpsSenser("TPS", SensorType::Tps1, engineConfiguration->tpsMin, engineConfiguration->tpsMax, engineConfiguration->tps1_1AdcChannel);
	printTpsSenser("TPS2", SensorType::Tps2, engineConfiguration->tps2Min, engineConfiguration->tps2Max, engineConfiguration->tps2_1AdcChannel);
}

static void setCrankingRpm(int value) {
	engineConfiguration->cranking.rpm = value;
	doPrintConfiguration();
}

/**
 * this method is used in console - it also prints current configuration
 */
static void setAlgorithmInt(int value) {
	setAlgorithm((engine_load_mode_e) value);
	doPrintConfiguration();
}

static void setFiringOrder(int value) {
	engineConfiguration->firingOrder = (firing_order_e) value;
	doPrintConfiguration();
}

static void setRpmHardLimit(int value) {
	engineConfiguration->rpmHardLimit = value;
	doPrintConfiguration();
}

static void setCrankingIACExtra(float percent) {
	engineConfiguration->crankingIACposition = percent;
	efiPrintf("cranking_iac %.2f", percent);
}

static void setCrankingFuel(float timeMs) {
	engineConfiguration->cranking.baseFuel = timeMs;
	efiPrintf("cranking_fuel %.2f", timeMs);
}

static void setGlobalTriggerAngleOffset(float value) {
	if (cisnan(value)) {
		warning(ObdCode::CUSTOM_ERR_SGTP_ARGUMENT, "Invalid argument");
		return;
	}
	engineConfiguration->globalTriggerAngleOffset = value;
	incrementGlobalConfigurationVersion();
	doPrintConfiguration();
}

static void setCrankingTimingAngle(float value) {
	engineConfiguration->crankingTimingAngle = value;
	incrementGlobalConfigurationVersion();
	doPrintConfiguration();
}

static void setCrankingInjectionMode(int value) {
	engineConfiguration->crankingInjectionMode = (injection_mode_e) value;
	incrementGlobalConfigurationVersion();
	doPrintConfiguration();
}

static void setInjectionMode(int value) {
	engineConfiguration->injectionMode = (injection_mode_e) value;
	incrementGlobalConfigurationVersion();
	doPrintConfiguration();
}

static void setIgnitionMode(int value) {
#if EFI_ENGINE_CONTROL
	engineConfiguration->ignitionMode = (ignition_mode_e) value;
	incrementGlobalConfigurationVersion();
	prepareOutputSignals();
	doPrintConfiguration();
#endif // EFI_ENGINE_CONTROL
}

static void setIndividualCoilsIgnition() {
	setIgnitionMode((int)IM_INDIVIDUAL_COILS);
}

static void setTriggerType(int value) {
	engineConfiguration->trigger.type = (trigger_type_e) value;
	incrementGlobalConfigurationVersion();
	doPrintConfiguration();
	efiPrintf("Do you need to also invoke set operation_mode X?");
	engine->resetEngineSnifferIfInTestMode();
}

static void setDebugMode(int value) {
	engineConfiguration->debugMode = (debug_mode_e) value;
}

static void setWholeTimingMap(float value) {
	setTable(config->ignitionTable, value);
}

static void setWholePhaseMapCmd(float value) {
	efiPrintf("Setting whole injection phase map to %.2f", value);
	setTable(config->injectionPhase, value);
}

static void setWholeTimingMapCmd(float value) {
	efiPrintf("Setting whole timing advance map to %.2f", value);
	setWholeTimingMap(value);
	engine->resetEngineSnifferIfInTestMode();
}

static void setWholeVeCmd(float value) {
	efiPrintf("Setting whole VE map to %.2f", value);
	if (engineConfiguration->fuelAlgorithm != LM_SPEED_DENSITY) {
		efiPrintf("WARNING: setting VE map not in SD mode is pointless");
	}
	setTable(config->veTable, value);
	engine->resetEngineSnifferIfInTestMode();
}

#if EFI_PROD_CODE

static brain_pin_e parseBrainPinWithErrorMessage(const char *pinName) {
	brain_pin_e pin = parseBrainPin(pinName);
	if (pin == Gpio::Invalid) {
		efiPrintf("invalid pin name [%s]", pinName);
	}
	return pin;
}

/**
 * For example:
 *   set_ignition_pin 1 PD7
 * todo: this method counts index from 1 while at least 'set_trigger_input_pin' counts from 0.
 * todo: make things consistent
 */
static void setIgnitionPin(const char *indexStr, const char *pinName) {
	int index = atoi(indexStr) - 1; // convert from human index into software index
	if (index < 0 || index >= MAX_CYLINDER_COUNT)
		return;
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting ignition pin[%d] to %s please save&restart", index, hwPortname(pin));
	engineConfiguration->ignitionPins[index] = pin;
	incrementGlobalConfigurationVersion();
}

// this method is useful for desperate time debugging
// readpin PA0
void readPin(const char *pinName) {
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	int physicalValue = palReadPad(getHwPort("read", pin), getHwPin("read", pin));
	efiPrintf("pin %s value %d", hwPortname(pin), physicalValue);
}

// this method is useful for desperate time debugging or hardware validation
static void benchSetPinValue(const char *pinName, int bit) {
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	palWritePad(getHwPort("write", pin), getHwPin("write", pin), bit);
	efiPrintf("pin %s set value", hwPortname(pin));
	readPin(pinName);
}

static void benchClearPin(const char *pinName) {
	benchSetPinValue(pinName, 0);
}

static void benchSetPin(const char *pinName) {
	benchSetPinValue(pinName, 1);
}

static void setIndividualPin(const char *pinName, brain_pin_e *targetPin, const char *name) {
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting %s pin to %s please save&restart", name, hwPortname(pin));
	*targetPin = pin;
	incrementGlobalConfigurationVersion();
}

// set vss_pin
static void setVssPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->vehicleSpeedSensorInputPin, "VSS");
}

// set_idle_pin none
static void setIdlePin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->idle.solenoidPin, "idle");
}

static void setMainRelayPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->mainRelayPin, "main relay");
}

static void setTriggerSyncPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->debugTriggerSync, "trigger sync");
}

static void setStarterRelayPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->starterRelayDisablePin, "starter disable relay");
}

static void setAlternatorPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->alternatorControlPin, "alternator");
}

static void setACRelayPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->acRelayPin, "A/C");
}

static void setFuelPumpPin(const char *pinName) {
	setIndividualPin(pinName, &engineConfiguration->fuelPumpPin, "fuelPump");
}

static void setInjectionPin(const char *indexStr, const char *pinName) {
	int index = atoi(indexStr) - 1; // convert from human index into software index
	if (index < 0 || index >= MAX_CYLINDER_COUNT)
		return;
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting injection pin[%d] to %s please save&restart", index, hwPortname(pin));
	engineConfiguration->injectionPins[index] = pin;
	incrementGlobalConfigurationVersion();
}

/**
 * For example:
 *   set_trigger_input_pin 0 PA5
 * todo: this method counts index from 0 while at least 'set_ignition_pin' counts from 1.
 * todo: make things consistent
 */
static void setTriggerInputPin(const char *indexStr, const char *pinName) {
	int index = atoi(indexStr);
	if (index < 0 || index > 2)
		return;
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting trigger pin[%d] to %s please save&restart", index, hwPortname(pin));
	engineConfiguration->triggerInputPins[index] = pin;
	incrementGlobalConfigurationVersion();
}

static void setTriggerSimulatorPin(const char *indexStr, const char *pinName) {
	int index = atoi(indexStr);
	if (index < 0 || index >= TRIGGER_SIMULATOR_PIN_COUNT)
		return;
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting trigger simulator pin[%d] to %s please save&restart", index, hwPortname(pin));
	engineConfiguration->triggerSimulatorPins[index] = pin;
	incrementGlobalConfigurationVersion();
}

#if HAL_USE_ADC
// set_analog_input_pin pps pa4
// set_analog_input_pin afr none
static void setAnalogInputPin(const char *sensorStr, const char *pinName) {
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	adc_channel_e channel = getAdcChannel(pin);
	if (channel == EFI_ADC_ERROR) {
		efiPrintf("Error with [%s]", pinName);
		return;
	}
	if (strEqual("map", sensorStr)) {
		engineConfiguration->map.sensor.hwChannel = channel;
		efiPrintf("setting MAP to %s/%d", pinName, channel);
	} else if (strEqual("pps", sensorStr)) {
		engineConfiguration->throttlePedalPositionAdcChannel = channel;
		efiPrintf("setting PPS to %s/%d", pinName, channel);
	} else if (strEqual("afr", sensorStr)) {
		engineConfiguration->afr.hwChannel = channel;
		efiPrintf("setting AFR to %s/%d", pinName, channel);
	} else if (strEqual("clt", sensorStr)) {
		engineConfiguration->clt.adcChannel = channel;
		efiPrintf("setting CLT to %s/%d", pinName, channel);
	} else if (strEqual("iat", sensorStr)) {
		engineConfiguration->iat.adcChannel = channel;
		efiPrintf("setting IAT to %s/%d", pinName, channel);
	} else if (strEqual("tps", sensorStr)) {
		engineConfiguration->tps1_1AdcChannel = channel;
		efiPrintf("setting TPS1 to %s/%d", pinName, channel);
	} else if (strEqual("tps2", sensorStr)) {
		engineConfiguration->tps2_1AdcChannel = channel;
		efiPrintf("setting TPS2 to %s/%d", pinName, channel);
	}
	incrementGlobalConfigurationVersion();
}
#endif // HAL_USE_ADC

static void setLogicInputPin(const char *indexStr, const char *pinName) {
	int index = atoi(indexStr);
	if (index < 0 || index > 2) {
		return;
	}
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("setting logic input pin[%d] to %s please save&restart", index, hwPortname(pin));
	engineConfiguration->logicAnalyzerPins[index] = pin;
	incrementGlobalConfigurationVersion();
}

static void showPinFunction(const char *pinName) {
	brain_pin_e pin = parseBrainPinWithErrorMessage(pinName);
	if (pin == Gpio::Invalid) {
		return;
	}
	efiPrintf("Pin %s: [%s]", pinName, getPinFunction(pin));
}

#endif // EFI_PROD_CODE

static void setSpiMode(int index, bool mode) {
	switch (index) {
	case 1:
		engineConfiguration->is_enabled_spi_1 = mode;
		break;
	case 2:
		engineConfiguration->is_enabled_spi_2 = mode;
		break;
	case 3:
		engineConfiguration->is_enabled_spi_3 = mode;
		break;
	default:
		efiPrintf("invalid spi index %d", index);
		return;
	}
	printSpiState(engineConfiguration);
}

bool verboseRxCan = false;

static void enableOrDisable(const char *param, bool isEnabled) {
	if (strEqualCaseInsensitive(param, "useTLE8888_cranking_hack")) {
		engineConfiguration->useTLE8888_cranking_hack = isEnabled;
#if EFI_SHAFT_POSITION_INPUT
	} else if (strEqualCaseInsensitive(param, CMD_TRIGGER_HW_INPUT)) {
		getTriggerCentral()->hwTriggerInputEnabled = isEnabled;
#endif // EFI_SHAFT_POSITION_INPUT
	} else if (strEqualCaseInsensitive(param, "verboseTLE8888")) {
		engineConfiguration->verboseTLE8888 = isEnabled;
	} else if (strEqualCaseInsensitive(param, "verboseRxCan")) {
		verboseRxCan = isEnabled;
	} else if (strEqualCaseInsensitive(param, "verboseCan")) {
		engineConfiguration->verboseCan = isEnabled;
	} else if (strEqualCaseInsensitive(param, "verboseIsoTp")) {
		engineConfiguration->verboseIsoTp = isEnabled;
	} else if (strEqualCaseInsensitive(param, "artificialMisfire")) {
		engineConfiguration->artificialTestMisfire = isEnabled;
	} else if (strEqualCaseInsensitive(param, "logic_level_trigger")) {
		engineConfiguration->displayLogicLevelsInEngineSniffer = isEnabled;
	} else if (strEqualCaseInsensitive(param, "can_broadcast")) {
		engineConfiguration->enableVerboseCanTx = isEnabled;
	} else if (strEqualCaseInsensitive(param, "etb_auto")) {
		engine->etbAutoTune = isEnabled;
	} else if (strEqualCaseInsensitive(param, "verboseKLine")) {
		engineConfiguration->verboseKLine = isEnabled;
	} else if (strEqualCaseInsensitive(param, "stepperidle")) {
		engineConfiguration->useStepperIdle = isEnabled;
	} else if (strEqualCaseInsensitive(param, "boardUseTempPullUp")) {
		engineConfiguration->boardUseTempPullUp = isEnabled;
		incrementGlobalConfigurationVersion();
	} else if (strEqualCaseInsensitive(param, "boardUseTachPullUp")) {
		engineConfiguration->boardUseTachPullUp = isEnabled;
		incrementGlobalConfigurationVersion();
	} else if (strEqualCaseInsensitive(param, "two_wire_wasted_spark")) {
		engineConfiguration->twoWireBatchIgnition = isEnabled;
		incrementGlobalConfigurationVersion();
	} else if (strEqualCaseInsensitive(param, "altdebug")) {
		engineConfiguration->isVerboseAlternator = isEnabled;
	} else if (strEqualCaseInsensitive(param, "tpic_advanced_mode")) {
		engineConfiguration->useTpicAdvancedMode = isEnabled;
	} else if (strEqualCaseInsensitive(param, "altcontrol")) {
		engineConfiguration->isAlternatorControlEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, "sd")) {
		engineConfiguration->isSdCardEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, CMD_FUNCTIONAL_TEST_MODE)) {
		engine->isFunctionalTestMode = isEnabled;
	} else if (strEqualCaseInsensitive(param, "can_read")) {
		engineConfiguration->canReadEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, "can_write")) {
		engineConfiguration->canWriteEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, CMD_INJECTION)) {
		engineConfiguration->isInjectionEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, CMD_PWM)) {
		engine->isPwmEnabled = isEnabled;
	} else if (strEqualCaseInsensitive(param, "trigger_details")) {
		engineConfiguration->verboseTriggerSynchDetails = isEnabled;
	} else if (strEqualCaseInsensitive(param, "vvt_details")) {
		engineConfiguration->verboseVVTDecoding = isEnabled;
	} else if (strEqualCaseInsensitive(param, "invertCamVVTSignal")) {
		engineConfiguration->invertCamVVTSignal = isEnabled;
	} else if (strEqualCaseInsensitive(param, CMD_IGNITION)) {
		engineConfiguration->isIgnitionEnabled = isEnabled;
#if EFI_EMULATE_POSITION_SENSORS
	} else if (strEqualCaseInsensitive(param, CMD_SELF_STIMULATION)) {
		if (isEnabled) {
			enableTriggerStimulator();
		} else {
			disableTriggerStimulator();
		}
	} else if (strEqualCaseInsensitive(param, CMD_EXTERNAL_STIMULATION)) {
		if (isEnabled) {
			enableExternalTriggerStimulator();
		} else {
			disableTriggerStimulator();
		}
#endif // EFI_EMULATE_POSITION_SENSORS
	} else {
		efiPrintf("unexpected [%s]", param);
		return; // well, MISRA would not like this 'return' here :(
	}
	efiPrintf("[%s] %s", param, isEnabled ? "enabled" : "disabled");
}

static void enable(const char *param) {
	enableOrDisable(param, true);
}

static void disable(const char *param) {
	enableOrDisable(param, false);
}

static void enableSpi(int index) {
	setSpiMode(index, true);
}

static void disableSpi(int index) {
	setSpiMode(index, false);
}

/**
 * See 'LimpManager::isEngineStop' for code which actually stops engine
 */
void scheduleStopEngine() {
	doScheduleStopEngine();
}

static void getValue(const char *paramStr) {

	{
		float value = getConfigValueByName(paramStr);
		if (value != EFI_ERROR_CODE) {
			efiPrintf("%s value: %.2f", paramStr, value);
			return;
		}
	}

	if (strEqualCaseInsensitive(paramStr, "tps_min")) {
		efiPrintf("tps_min=%d", engineConfiguration->tpsMin);
	} else if (strEqualCaseInsensitive(paramStr, "tps_max")) {
		efiPrintf("tps_max=%d", engineConfiguration->tpsMax);
	} else if (strEqualCaseInsensitive(paramStr, "global_trigger_offset_angle")) {
		efiPrintf("global_trigger_offset=%.2f", engineConfiguration->globalTriggerAngleOffset);
#if EFI_SHAFT_POSITION_INPUT
	} else if (strEqualCaseInsensitive(paramStr, "trigger_hw_input")) {
		efiPrintf("trigger_hw_input=%s", boolToString(getTriggerCentral()->hwTriggerInputEnabled));
#endif // EFI_SHAFT_POSITION_INPUT
	} else if (strEqualCaseInsensitive(paramStr, "is_enabled_spi_1")) {
		efiPrintf("is_enabled_spi_1=%s", boolToString(engineConfiguration->is_enabled_spi_1));
	} else if (strEqualCaseInsensitive(paramStr, "is_enabled_spi_2")) {
		efiPrintf("is_enabled_spi_2=%s", boolToString(engineConfiguration->is_enabled_spi_2));
	} else if (strEqualCaseInsensitive(paramStr, "is_enabled_spi_3")) {
		efiPrintf("is_enabled_spi_3=%s", boolToString(engineConfiguration->is_enabled_spi_3));
	} else if (strEqualCaseInsensitive(paramStr, "invertCamVVTSignal")) {
		efiPrintf("invertCamVVTSignal=%s", boolToString(engineConfiguration->invertCamVVTSignal));
	} else if (strEqualCaseInsensitive(paramStr, "isHip9011Enabled")) {
		efiPrintf("isHip9011Enabled=%d", engineConfiguration->isHip9011Enabled);
	} else if (strEqualCaseInsensitive(paramStr, CMD_DATE)) {
		printDateTime();
	} else {
		efiPrintf("Invalid Parameter: %s", paramStr);
	}
}

static void setScriptCurve1Value(float value) {
	setLinearCurve(config->scriptCurve1, value, value, 1);
}

static void setScriptCurve2Value(float value) {
	setLinearCurve(config->scriptCurve2, value, value, 1);
}

struct command_i_s {
	const char *token;
	VoidInt callback;
};

struct command_f_s {
	const char *token;
	VoidFloat callback;
};

const command_f_s commandsF[] = {
#if EFI_ENGINE_CONTROL
		{"global_trigger_offset_angle", setGlobalTriggerAngleOffset},
		{"cranking_fuel", setCrankingFuel},
		{"cranking_iac", setCrankingIACExtra},
		{"cranking_timing_angle", setCrankingTimingAngle},
		{"tps_accel_threshold", setTpsAccelThr},
		{"tps_decel_threshold", setTpsDecelThr},
		{"tps_decel_multiplier", setTpsDecelMult},
		{"flat_injector_lag", setFlatInjectorLag},
#endif // EFI_ENGINE_CONTROL
		{"script_curve_1_value", setScriptCurve1Value},
		{"script_curve_2_value", setScriptCurve2Value},
#if EFI_PROD_CODE
#if EFI_IDLE_CONTROL
		{"idle_p", setIdlePFactor},
		{"idle_i", setIdleIFactor},
		{"idle_d", setIdleDFactor},
#endif // EFI_IDLE_CONTROL
#endif // EFI_PROD_CODE

#if EFI_ELECTRONIC_THROTTLE_BODY
		{"etb_p", setEtbPFactor},
		{"etb_i", setEtbIFactor},
		{"etb_d", setEtbDFactor},
		{"etb", setThrottleDutyCycle},
#endif // EFI_ELECTRONIC_THROTTLE_BODY

		//		{"", },
//		{"", },
//		{"", },
		//		{"", },
		//		{"", },
		//		{"", },
};

static void setTpsErrorDetectionTooLow(int v) {
	engineConfiguration->tpsErrorDetectionTooLow = v;
}

static void setTpsErrorDetectionTooHigh(int v) {
	engineConfiguration->tpsErrorDetectionTooHigh = v;
}

const command_i_s commandsI[] = {{"ignition_mode", setIgnitionMode},
#if EFI_ENGINE_CONTROL
        {"driveWheelRevPerKm", [](int value) {
            engineConfiguration->driveWheelRevPerKm = value;
        }},
		{"cranking_rpm", setCrankingRpm},
		{"cranking_injection_mode", setCrankingInjectionMode},
		{"injection_mode", setInjectionMode},
		{"sensor_chart_mode", setSensorChartMode},
		{"tpsErrorDetectionTooLow", setTpsErrorDetectionTooLow},
		{"tpsErrorDetectionTooHigh", setTpsErrorDetectionTooHigh},
		{"fixed_mode_timing", setFixedModeTiming},
		{"timing_mode", setTimingMode},
		{CMD_ENGINE_TYPE, setEngineTypeAndSave},
		{"rpm_hard_limit", setRpmHardLimit},
		{"firing_order", setFiringOrder},
		{"algorithm", setAlgorithmInt},
		{"injection_pin_mode", setInjectionPinMode},
		{"ignition_pin_mode", setIgnitionPinMode},
		{"idle_pin_mode", setIdlePinMode},
		{"fuel_pump_pin_mode", setFuelPumpPinMode},
		{"malfunction_indicator_pin_mode", setMalfunctionIndicatorPinMode},
		{"debug_mode", setDebugMode},
		{"trigger_type", setTriggerType},
		{"idle_solenoid_freq", setIdleSolenoidFrequency},
		{"tps_accel_len", setTpsAccelLen},
#endif // EFI_ENGINE_CONTROL
#if EFI_BOR_LEVEL
		{"bor", setBor},
#endif // EFI_BOR_LEVEL
#if EFI_PROD_CODE
#if EFI_CAN_SUPPORT
		{"can_mode", setCanType},
		{"can_vss", setCanVss},
#endif // EFI_CAN_SUPPORT
#if EFI_IDLE_CONTROL
		{"idle_position", setManualIdleValvePosition},
		{"idle_rpm", setTargetIdleRpm},
#endif // EFI_IDLE_CONTROL
#endif // EFI_PROD_CODE

#if EFI_ELECTRONIC_THROTTLE_BODY
		{"etb_o", setEtbOffset},
#endif // EFI_ELECTRONIC_THROTTLE_BODY

		//		{"", },
		//		{"", },
		//		{"", },
		//		{"", },
		//		{"", },
};

static void setValue(const char *paramStr, const char *valueStr) {
	float valueF = atoff(valueStr);
	int valueI = atoi(valueStr);

	const command_f_s *currentF = &commandsF[0];
	while (currentF < commandsF + sizeof(commandsF)/sizeof(commandsF[0])) {
		if (strEqualCaseInsensitive(paramStr, currentF->token)) {
			currentF->callback(valueF);
			return;
		}
		currentF++;
	}

	const command_i_s *currentI = &commandsI[0];
	while (currentI < commandsI + sizeof(commandsI)/sizeof(commandsI[0])) {
		if (strEqualCaseInsensitive(paramStr, currentI->token)) {
			currentI->callback(valueI);
			return;
		}
		currentI++;
	}

#if EFI_ALTERNATOR_CONTROL
	if (strEqualCaseInsensitive(paramStr, "alt_t")) {
		if (valueI > 10) {
			engineConfiguration->alternatorControl.periodMs = valueI;
		}
		showAltInfo();
	} else if (strEqualCaseInsensitive(paramStr, "alt_offset")) {
		engineConfiguration->alternatorControl.offset = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "alt_p")) {
		setAltPFactor(valueF);
	} else
#endif // EFI_ALTERNATOR_CONTROL
	if (strEqualCaseInsensitive(paramStr, "warning_period")) {
		engineConfiguration->warningPeriod = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "dwell")) {
		setConstantDwell(valueF);
	} else if (strEqualCaseInsensitive(paramStr, CMD_ENGINESNIFFERRPMTHRESHOLD)) {
		engineConfiguration->engineSnifferRpmThreshold = valueI;
// migrate to new laucnh fields?
		//	} else if (strEqualCaseInsensitive(paramStr, "step1rpm")) {
//		engineConfiguration->step1rpm = valueI;
		//	} else if (strEqualCaseInsensitive(paramStr, "step1timing")) {
		//		engineConfiguration->step1timing = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "tps_max")) {
		engineConfiguration->tpsMax = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "tps_min")) {
		engineConfiguration->tpsMin = valueI;
#if EFI_EMULATE_POSITION_SENSORS
	} else if (strEqualCaseInsensitive(paramStr, CMD_RPM)) {
		setTriggerEmulatorRPM(valueI);
#endif // EFI_EMULATE_POSITION_SENSORS
	} else if (strEqualCaseInsensitive(paramStr, "mc33_hvolt")) {
		engineConfiguration->mc33_hvolt = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "mc33_i_peak")) {
		engineConfiguration->mc33_i_peak = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "mc33_i_hold")) {
		engineConfiguration->mc33_i_hold = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "mc33_t_max_boost")) {
		engineConfiguration->mc33_t_max_boost = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "mc33_t_peak_off")) {
		engineConfiguration->mc33_t_peak_off = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "vvt_offset")) {
		engineConfiguration->vvtOffsets[0] = valueF;
	} else if (strEqualCaseInsensitive(paramStr, "vvt_mode")) {
		engineConfiguration->vvtMode[0] = (vvt_mode_e)valueI;
	} else if (strEqualCaseInsensitive(paramStr, "wwaeTau")) {
		engineConfiguration->wwaeTau = valueF;
	} else if (strEqualCaseInsensitive(paramStr, "wwaeBeta")) {
		engineConfiguration->wwaeBeta = valueF;
	} else if (strEqualCaseInsensitive(paramStr, "benchTestOffTime")) {
		engineConfiguration->benchTestOffTime = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "benchTestCount")) {
		engineConfiguration->benchTestCount = valueI;
	} else if (strEqualCaseInsensitive(paramStr, "cranking_dwell")) {
		engineConfiguration->ignitionDwellForCrankingMs = valueF;
#if EFI_PROD_CODE
	} else if (strEqualCaseInsensitive(paramStr, CMD_VSS_PIN)) {
		setVssPin(valueStr);
#endif // EFI_PROD_CODE
	} else if (strEqualCaseInsensitive(paramStr, "targetvbatt")) {
		engineConfiguration->targetVBatt = valueF;
	} else if (strEqualCaseInsensitive(paramStr, CMD_DATE)) {
		// rusEfi console invokes this method with timestamp in local timezone
		setDateTime(valueStr);
	}

	bool isGoodName = setConfigValueByName(paramStr, valueF);
    if (isGoodName) {
       efiPrintf("Settings: applying [%s][%f]", paramStr, valueF);
    }

	engine->resetEngineSnifferIfInTestMode();
}

void initSettings() {
#if EFI_SIMULATOR
	printf("initSettings\n");
#endif // EFI_SIMULATOR

	// todo: start saving values into flash right away?

	addConsoleAction("showconfig", doPrintConfiguration);
	addConsoleAction("tpsinfo", printTPSInfo);
	addConsoleAction("calibrate_tps_1_closed", grabTPSIsClosed);
	addConsoleAction("calibrate_tps_1_wot", grabTPSIsWideOpen);

    // used by HW CI
	addConsoleAction(CMD_INDIVIDUAL_INJECTION, setIndividualCoilsIgnition);

	addConsoleActionF("set_whole_phase_map", setWholePhaseMapCmd);
	addConsoleActionF("set_whole_timing_map", setWholeTimingMapCmd);
	addConsoleActionF("set_whole_ve_map", setWholeVeCmd);
	addConsoleActionF("set_whole_ign_corr_map", setWholeIgnitionIatCorr);

	addConsoleAction("stopengine", (Void) scheduleStopEngine);

	// todo: refactor this - looks like all boolean flags should be controlled with less code duplication
	addConsoleActionI("enable_spi", enableSpi);
	addConsoleActionI("disable_spi", disableSpi);

	addConsoleActionS(CMD_ENABLE, enable);
	addConsoleActionS(CMD_DISABLE, disable);

	addConsoleActionSS(CMD_SET, setValue);
	addConsoleActionS(CMD_GET, getValue);

#if EFI_PROD_CODE
	addConsoleActionS("showpin", showPinFunction);
	addConsoleActionSS(CMD_INJECTION_PIN, setInjectionPin);
	addConsoleActionSS(CMD_IGNITION_PIN, setIgnitionPin);
	addConsoleActionSS(CMD_TRIGGER_PIN, setTriggerInputPin);
	addConsoleActionSS(CMD_TRIGGER_SIMULATOR_PIN, setTriggerSimulatorPin);

	addConsoleActionI(CMD_ECU_UNLOCK, unlockEcu);

	addConsoleActionS("set_fuel_pump_pin", setFuelPumpPin);
	addConsoleActionS("set_acrelay_pin", setACRelayPin);
	addConsoleActionS(CMD_ALTERNATOR_PIN, setAlternatorPin);
	addConsoleActionS(CMD_IDLE_PIN, setIdlePin);
	addConsoleActionS("set_main_relay_pin", setMainRelayPin);
	addConsoleActionS("set_starter_relay_pin", setStarterRelayPin);
	addConsoleActionS("set_trigger_sync_pin", setTriggerSyncPin);

	addConsoleActionS("bench_clearpin", benchClearPin);
	addConsoleActionS("bench_setpin", benchSetPin);
	addConsoleActionS("readpin", readPin);

#if HAL_USE_ADC
	addConsoleAction("adc_report", printFullAdcReport);
	addConsoleActionSS("set_analog_input_pin", setAnalogInputPin);
#endif // HAL_USE_ADC
	addConsoleActionSS(CMD_LOGIC_PIN, setLogicInputPin);
#endif // EFI_PROD_CODE
}

void printDateTime() {
#if EFI_RTC
	printRtcDateTime();
#else // EFI_RTC
	efiPrintf("Cannot print time: RTC not supported");
#endif // EFI_RTC
}

void setDateTime(const char * const isoDateTime) {
#if EFI_RTC
	if (strlen(isoDateTime) >= 19 && isoDateTime[10] == 'T') {
		efidatetime_t dateTime;
		dateTime.year = atoi(isoDateTime);
		dateTime.month = atoi(isoDateTime + 5);
		dateTime.day = atoi(isoDateTime + 8);
		dateTime.hour = atoi(isoDateTime + 11);
		dateTime.minute = atoi(isoDateTime + 14);
		dateTime.second = atoi(isoDateTime + 17);
		if (dateTime.year != ATOI_ERROR_CODE &&
				dateTime.month >= 1 && dateTime.month <= 12 &&
				dateTime.day >= 1 && dateTime.day <= 31 &&
				dateTime.hour <= 23 &&
				dateTime.minute <= 59 &&
				dateTime.second <= 59) {
			// doesn't concern about leap years or seconds; ChibiOS doesn't support (added) leap seconds anyway
			setRtcDateTime(&dateTime);
			return;
		}
	}
	efiPrintf("date_set Date parameter %s is wrong", isoDateTime);
#else // EFI_RTC
	efiPrintf("Cannot set time: RTC not supported");
#endif // EFI_RTC
}

#endif // ! EFI_UNIT_TEST

void setEngineTypeAndSave(int value) {
	setEngineType(value, true);
}

void setEngineType(int value, bool isWriteToFlash) {
	{
#if EFI_PROD_CODE
		chibios_rt::CriticalSectionLocker csl;
#endif // EFI_PROD_CODE

		engineConfiguration->engineType = (engine_type_e)value;
		resetConfigurationExt((engine_type_e)value);
		engine->resetEngineSnifferIfInTestMode();

#if EFI_INTERNAL_FLASH
		if (isWriteToFlash) {
			writeToFlashNow();
		}
#endif // EFI_INTERNAL_FLASH
	}
	incrementGlobalConfigurationVersion("engineType");
#if ! EFI_UNIT_TEST
	doPrintConfiguration();
#endif // ! EFI_UNIT_TEST
}
