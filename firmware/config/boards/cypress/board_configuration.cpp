/**
 * @file boards/cypress/board_configuration.h
 *
 * @brief In this file we can override engine_configuration.cpp.
 *
 * @date Jan 27, 2020
 * @author andreika <prometheus.pcb@gmail.com>
 */

#include "pch.h"
#include "trigger_input.h"

Gpio getWarningLedPin() {
	// this board has no warning led
	return Gpio::Unassigned;
}

Gpio getRunningLedPin() {
	return Gpio::Unassigned;
}

Gpio getCommsLedPin() {
	// this board has no comms led
	return Gpio::Unassigned;
}

void setBoardDefaultConfiguration() {
	setCrankOperationMode();

	engineConfiguration->useNoiselessTriggerDecoder = true;

	setAlgorithm(LM_SPEED_DENSITY);

	engineConfiguration->cylindersCount = 4;
	engineConfiguration->firingOrder = FO_1_3_4_2;

	engineConfiguration->ignitionMode = IM_WASTED_SPARK;
	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_SIMULTANEOUS;

	engineConfiguration->globalTriggerAngleOffset = 114;	// the end of 19th tooth?

	engineConfiguration->displacement = 1.645;
	engineConfiguration->injector.flow = 200;
	
	engineConfiguration->cranking.baseFuel = 25;		// ???
	engineConfiguration->cranking.rpm = 600;

	engineConfiguration->rpmHardLimit = 3000; // yes, 3k. let's play it safe for now
	
	engineConfiguration->map.sensor.type = MT_MPX4250A;

	engineConfiguration->idleStepperReactionTime = 10;
	engineConfiguration->stepperDirectionPinMode = OM_INVERTED;
	engineConfiguration->idle.stepperDirectionPin = Gpio::Unassigned;
	engineConfiguration->idle.stepperStepPin = Gpio::Unassigned;
	engineConfiguration->stepperEnablePin = Gpio::Unassigned;

	engineConfiguration->useLinearCltSensor = true;
	// todo:
	engineConfiguration->clt.config.resistance_1 = 0;
	engineConfiguration->clt.config.tempC_1 = -40.0f;
	engineConfiguration->clt.config.resistance_2 = 5.0f;
	engineConfiguration->clt.config.tempC_2 = 120.0f,
	engineConfiguration->clt.config.bias_resistor = 3300;

	engineConfiguration->tpsMin = convertVoltageTo10bitADC(0.250);
	engineConfiguration->tpsMax = convertVoltageTo10bitADC(4.538);
	engineConfiguration->tpsErrorDetectionTooLow = -10; // -10% open
	engineConfiguration->tpsErrorDetectionTooHigh = 110; // 110% open

	engineConfiguration->mapMinBufferLength = 4;

	engineConfiguration->map.sensor.hwChannel = EFI_ADC_13;
	engineConfiguration->clt.adcChannel = EFI_ADC_26;
	engineConfiguration->iat.adcChannel = EFI_ADC_27;
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_3;
	engineConfiguration->afr.hwChannel = EFI_ADC_4;
	engineConfiguration->vbattAdcChannel = EFI_ADC_2;

#if 0
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_NONE;
	engineConfiguration->vbattAdcChannel = EFI_ADC_NONE;
	engineConfiguration->clt.adcChannel = EFI_ADC_NONE;
	engineConfiguration->iat.adcChannel = EFI_ADC_NONE;
	engineConfiguration->afr.hwChannel = EFI_ADC_NONE;
#endif

	engineConfiguration->auxFastSensor1_adcChannel = EFI_ADC_NONE;
	engineConfiguration->tps1_2AdcChannel = EFI_ADC_NONE;
	engineConfiguration->tps2_2AdcChannel = EFI_ADC_NONE;
	engineConfiguration->throttlePedalPositionSecondAdcChannel = EFI_ADC_NONE;
	
	engineConfiguration->mafAdcChannel = EFI_ADC_NONE;
	engineConfiguration->hipOutputChannel = EFI_ADC_NONE;
	engineConfiguration->fuelLevelSensor = EFI_ADC_NONE;
	engineConfiguration->oilPressure.hwChannel = EFI_ADC_NONE;

	engineConfiguration->acSwitch = Gpio::Unassigned;
	engineConfiguration->triggerInputPins[0] = Gpio::B0;
	engineConfiguration->triggerInputPins[1] = Gpio::Unassigned;

#if 0	

	// todo:
	int i;
	for (i = 0; i < MAX_CYLINDER_COUNT; i++)
		engineConfiguration->injectionPins[i] = Gpio::Unassigned;
	for (i = 0; i < MAX_CYLINDER_COUNT; i++)
		engineConfiguration->ignitionPins[i] = Gpio::Unassigned;
	
	engineConfiguration->adcVcc = 5.0f;
	engineConfiguration->analogInputDividerCoefficient = 1;
#endif

	//!!!!!!!!!!!!!!!!!!!
	//engineConfiguration->isFastAdcEnabled = false;
}

void setAdcChannelOverrides() {
	addAdcChannelForTrigger();
}
