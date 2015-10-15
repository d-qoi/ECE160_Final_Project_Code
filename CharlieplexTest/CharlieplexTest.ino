#include <Charlieplex.h>
#define LED_PIN_ONE   6
#define LED_PIN_TWO   7
#define LED_PIN_THREE 8

byte pins[] = {LED_PIN_ONE, LED_PIN_TWO, LED_PIN_THREE};
Charlieplex charlie = Charlieplex(pins, sizeof(pins));

byte LED_GREEN = {LED_PIN_ONE, LED_PIN_TWO};
byte LED_RED = {LED_PIN_TWO, LED_PIN_ONE};
byte LED_YELLOW = {LED_PIN_THREE, LED_PIN_TWO};
BYTE LED_BLUE = {LED_PIN_TWO, LED_PIN_THREE};

void setup() {

}

void loop() {
	charlie.charlieWrite(LED_BLUE, HIGH);
	delay(500);
	charlie.charlieWrite(LED_YELLOW, HIGH);
	delay(500);
	charlie.charlieWrite(LED_RED, HIGH);
	delay(500);
	charlie.charlieWrite(LED_GREEN, HIGH);
}