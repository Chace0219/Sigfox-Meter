// PinChangeIntExample
// This only works for ATMega328-compatibles; ie, Leonardo is not covered here.
// See the Arduino and the chip documentation for more details.
// See the Wiki at http://code.google.com/p/arduino-pinchangeint/wiki for more information.

// for vim editing: :set et ts=2 sts=2 sw=2

// This example demonstrates a configuration of 3 interrupting pins and 2 interrupt functions.
// The functions set the values of some global variables. All interrupts are serviced immediately,
// and the sketch can then query the values at our leisure. This makes loop timing non-critical.

// The interrupt functions are a simple count of the number of times the pin was brought high.
// For 2 of the pins, the values are stored and retrieved from an array and they are reset after
// every read. For one of the pins ("MYPIN3"), there is a monotonically increasing count; that is,
// until the 8-bit value reaches 255. Then it will go back to 0.

// For a more introductory sketch, see the SimpleExample328.ino sketch in the PinChangeInt
// library distribution.

#include <PinChangeInt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include <WISOL.h>
//#include <Tsensors.h>
#include <Wire.h>
#include <math.h>
//#include <SimpleTimer.h>

#include "Battery.h"

Battery battery(3400, 4600, A3);
Isigfox *Isigfox = new WISOL();

#define FLOW1 7
#define FLOW2 6
#define FLOW3 5
#define FLOW4 4
#define FLOW5 3

volatile uint32_t flow1Count = 0;
volatile uint32_t flow2Count = 0;
volatile uint32_t flow3Count = 0;
volatile uint32_t flow4Count = 0;
volatile uint32_t flow5Count = 0;

//
volatile uint16_t wdtCount = 0;
//
ISR(WDT_vect)
{
	wdtCount++;
	// Serial.println("WDT Overrun!!!");
	// This was debug code. now it does not need
}

/* Flow sensor interrupts */
void flow1ISR()
{
	flow1Count++;
}

void flow2ISR()
{
	flow2Count++;
}

void flow3ISR()
{
	flow3Count++;
}

void flow4ISR()
{
	flow4Count++;
}

void flow5ISR()
{
	flow5Count++;
}

void enterSleep(void)
{

	pinMode(FLOW1, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW1, flow1ISR, RISING);

	pinMode(FLOW2, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW2, flow2ISR, RISING);

	pinMode(FLOW3, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW3, flow3ISR, RISING);

	pinMode(FLOW4, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW4, flow4ISR, RISING);

	pinMode(FLOW5, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW5, flow5ISR, RISING);
	delay(50);

	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	sleep_mode();
	sleep_disable(); //*/
}

// Attach the interrupts in setup()
void setup() {
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);

	Serial.begin(9600);

	delay(5000);
	// WISOL test
	Isigfox->initSigfox();
	Isigfox->testComms();
	GetDeviceID();
	delay(2000);
	battery.begin();
	/*** Setup the WDT ***/

	/* Clear the reset flag. */
	MCUSR &= ~(1 << WDRF);
	/* In order to change WDE or the prescaler, we need to
	* set WDCE (This will allow updates for 4 clock cycles).
	*/
	WDTCSR |= (1 << WDCE) | (1 << WDE);

	/* set new watchdog timeout prescaler value */
	WDTCSR = 1 << WDP0 | 1 << WDP3; /* 8.0 seconds */

																	/* Enable the WD interrupt (note no reset). */
	WDTCSR |= _BV(WDIE);

	pinMode(FLOW1, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW1, flow1ISR, RISING);

	pinMode(FLOW2, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW2, flow2ISR, RISING);

	pinMode(FLOW3, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW3, flow3ISR, RISING);

	pinMode(FLOW4, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW4, flow4ISR, RISING);

	pinMode(FLOW5, INPUT_PULLUP);
	attachPinChangeInterrupt(FLOW5, flow5ISR, RISING);

}

void loop()
{
	uint8_t count;
	enterSleep();

	const uint8_t nInterval = 11; //minutes unit, you have to change time
	if (wdtCount >= (nInterval * 60) / 8)
	{
		uint8_t buf_str[12];

		uint16_t nVoltage = battery.voltage();
		uint8_t nLevel = 0;
		if (nVoltage >= 3800)
			nLevel = 6;
		else if (nVoltage >= 3700 && nVoltage < 3800)
			nLevel = 5;
		else if (nVoltage >= 3600 && nVoltage < 3700)
			nLevel = 4;
		else if (nVoltage >= 3500 && nVoltage < 3600)
			nLevel = 3;
		else if (nVoltage >= 3400 && nVoltage < 3500)
			nLevel = 2;
		else if (nVoltage >= 3000 && nVoltage < 3400)
			nLevel = 1;
		else
			nLevel = 0;

		buf_str[0] = nLevel;
		
		// flow1 high, flow1 low, etc, is that fit to your requirement
		buf_str[1] = flow1Count / 256;
		buf_str[2] = flow1Count % 256;
		buf_str[3] = flow2Count / 256;
		buf_str[4] = flow2Count % 256;
		buf_str[5] = flow3Count / 256;
		buf_str[6] = flow3Count % 256;
		buf_str[7] = flow4Count / 256;
		buf_str[8] = flow4Count % 256;
		buf_str[9] = flow5Count / 256;
		buf_str[10] = flow5Count % 256;

		wdtCount = 0;
		flow1Count = 0;
		flow2Count = 0;
		flow3Count = 0;
		flow4Count = 0;
		flow5Count = 0;

		Serial.print("Battery voltage is ");
		Serial.print(battery.voltage());
		Serial.println("mV.");
		Send_Pload(buf_str, 12);

	}
}

void Send_Pload(uint8_t *sendData, const uint8_t len) {
	// No downlink message require
	recvMsg *RecvMsg;
	Isigfox->resetMacroChannel(); // required to send on first macro channel
	delay(100);

	RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
	Isigfox->sendPayload(sendData, len, 0, RecvMsg);
	for (int i = 0; i < RecvMsg->len; i++) {
		Serial.print(RecvMsg->inData[i]);
	}
	Serial.println("");
	free(RecvMsg);
}

void GetDeviceID() {
	recvMsg *RecvMsg;
	const char msg[] = "AT$I=10";

	RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
	Isigfox->sendMessage(msg, 7, RecvMsg);

	Serial.print("Device ID: ");
	for (int i = 0; i<RecvMsg->len; i++) {
		Serial.print(RecvMsg->inData[i]);
	}
	Serial.println("");
	free(RecvMsg);
}