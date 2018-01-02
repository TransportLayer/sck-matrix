/***************************************************************************\
|*  SCKeyboard: Key matrix                                                 *|
|*  Copyright (C) 2017, 2018  TransportLayer                               *|
|*  Depends on Arduino libraries                                           *|
|*                                                                         *|
|*  This program is free firmware: you can redistribute it and/or modify   *|
|*  it under the terms of the GNU General Public License as published by   *|
|*  the Free Software Foundation, either version 3 of the License, or      *|
|*  (at your option) any later version.                                    *|
|*                                                                         *|
|*  This program is distributed in the hope that it will be useful,        *|
|*  but WITHOUT ANY WARRANTY; without even the implied warranty of         *|
|*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *|
|*  GNU General Public License for more details.                           *|
|*                                                                         *|
|*  You should have received a copy of the GNU General Public License      *|
|*  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *|
\***************************************************************************/

// We're using 2 decade counters to drive the columns
// Each matrix has 10 columns and 5 rows

#define MATRIX_RESET 14		// Also called A0
#define MATRIX_CLOCK 15		// Also called A1

// Testing will have to be done on higher-quality switches to check bounce characteristics
// 2500us seems to work for cheap tactile switches
#define DEBOUNCE_MIN 2500	// Minimum time in microseconds before confirming pin state
#define DEBOUNCE_MODE 0		// 0: Accurate / 1: Fast

const uint8_t matrixRows[5][2] = {
	{4, 13},	// Each array is one row
	{5, 12},	// Each number is the pin for that matrix
	{6, 11},
	{7, 10},
	{8, 9}
};

uint32_t matrixTimes[2][5][10] = {	// Time of last state change
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	}
};

// It would save a lot of memory to use a uint16_t or similar instead of many bools... might do this in the future.
bool matrixLastState[2][5][10] = {	// The most recently scanned state (used internally)
	{
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false}
	},
	{
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false}
	}
};

bool matrixState[2][5][10] = {	// The state sent out
	{
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false}
	},
	{
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false},
		{false, false, false, false, false, false, false, false}
	}
};

uint8_t column;

void setupPins(void) {
	digitalWrite(MATRIX_RESET, LOW);	// Ensure outputs are low before connecting them to the pins
	digitalWrite(MATRIX_CLOCK, LOW);	// (I think this might be done automatically though)
	pinMode(MATRIX_RESET, OUTPUT);
	pinMode(MATRIX_CLOCK, OUTPUT);
	for (uint8_t row = 0; row < sizeof(matrixRows) / sizeof(matrixRows[0]); ++row) {
		for (uint8_t matrixNumber = 0; matrixNumber < sizeof(matrixRows[row]) / sizeof(uint8_t); ++matrixNumber) {
			pinMode(matrixRows[row][matrixNumber], INPUT);
		}
	}
}

void resetColumns(void) {
	digitalWrite(MATRIX_RESET, HIGH);	// To be safe, we need to wait 260ns now. Our function calls take longer, so we don't need additional delay.
	digitalWrite(MATRIX_RESET, LOW);	// And 400ns here.
	column = 0;
}

void incrementColumns(void) {
	digitalWrite(MATRIX_CLOCK, HIGH);	// To be safe, we need to wait 200ns now. Our function calls take longer, so we don't need additional delay.
	digitalWrite(MATRIX_CLOCK, LOW);	// It doesn't matter how long we're low; the counters increment on the rising edge.
	++column;
}

void setup(void) {
	setupPins();
	resetColumns();
	Serial.begin(230400);
}

void loop(void) {
	for (uint8_t row = 0; row < sizeof(matrixRows) / sizeof(matrixRows[0]); ++row) {
		for (uint8_t matrixNumber = 0; matrixNumber < sizeof(matrixRows[row]) / sizeof(uint8_t); ++matrixNumber) {
			// 0b00000000	Key Down (0)
			// 0b10000000	Key Up   (128)
			// 00-49	Matrix 0
			// 50-99	Matrix 1
			// 00,10,20...	Rows 0, 1, 2... in Matrix 0
			// 50,60,70...	Rows 0, 1, 2... in Matrix 1
			// 0-9		Columns 0-9
			uint8_t keyCode = (matrixNumber * 50) + (row * 10) + column;		// Generate the keycode based on the information above
			bool pinState = digitalRead(matrixRows[row][matrixNumber]);	// Get the current state of the pin

			switch (DEBOUNCE_MODE) {
				case 0:		// Accurate (double-checks before sending out the state)
					if (pinState != matrixLastState[matrixNumber][row][column]) {	// If the current state doesn't match what it was last time...
						matrixTimes[matrixNumber][row][column] = micros();	// Update the last keypress timestamp
						matrixLastState[matrixNumber][row][column] = pinState;
					} else if (matrixState[matrixNumber][row][column] != pinState) {			// If the last state doesn't match what we sent out...
						if (micros() - matrixTimes[matrixNumber][row][column] >= DEBOUNCE_MIN) {	// And if we've waited long enough for debouncing...
							Serial.write(keyCode | !pinState << 7);			// Send out the new state
							matrixState[matrixNumber][row][column] = pinState;
						}
					}
					break;
				case 1:		// Fast (thanks Shel!) (immediately sends out the state and debounces after)
					if (pinState != matrixState[matrixNumber][row][column]) {	// If the current state doesn't match what we've sent out...
						if (micros() - matrixTimes[matrixNumber][row][column] >= DEBOUNCE_MIN) {	// And if we've waited long enough for debouncing...
							Serial.write(keyCode | !pinState << 7);	// Send out the new state
							matrixState[matrixNumber][row][column] = pinState;	// Store the new state
							matrixTimes[matrixNumber][row][column] = micros();	// And update the last keypress timestamp
						}
					}
					break;
			}
		}
	}

	// Increment or reset the columns based on our current position
	if (column >= 9) {
		// We don't need to reset the columns, but we're doing it to reduce the risk of desynchronization
		resetColumns();
	} else {
		incrementColumns();
	}
}
