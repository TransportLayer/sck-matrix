# SCKeyboard: Key matrix
This firmware drives, reads, and debounces 2 10x5 key matrices. It uses 12 I/O
pins total (5 inputs per matrix and 2 outputs for both decade counters), with
configurable key debouncing.

## What this firmware does
 - Drives 2 key matrices
 - Reads 2 key matrices
 - Debounces key inputs
 - Tracks key up and down state
 - Sends debounced matrix state over UART
 - Works well **in combination with another microcontroller** as a keyboard controller

## What this firmware doesn't do
 - Act as a keyboard
 - Have a keymap
 - Send standard keycodes
 - Have a PS/2 output

## Hardware required
 - ATmega328 or compatbile AVR microcontroller (16MHz or higher recommended)
 - 2x decade counter (such as CB4017B)

## How to use this firmware
Compile this firmware with the Arduino libraries. Flash it, and wire your rows
of Matrix 0 to Arduino pins 4, 5, 6, 7, and 8. Wire your rows of Matrix 1 to
Arduino pins 13, 12, 11, 10, and 9. Add pull-down resistors (10K ohms is a
reasonable value) to every row. Wire together the reset pins of both decade
counters, and wire together the clock pins of both decade counters. It doesn't
matter which decade counter drives the columns of which matrix, as they're
synced together. The first output of each decade counter is column 0, the
second is column 1, and so on. The output is over UART at 230400 baud.

## How to decode the output
The output is encoded by
`uint8_t output = ((matrixNumber * 50) + (row * 10) + column) | !keyDown << 7;`,
so a possible way to decode it is:
 - `uint8_t keyDown = output >> 7;`
 - `uint8_t matrixNumber = (output & ~(1<<7)) / 50;`
 - `uint8_t row = ((output & ~(1<<7)) % 50) / 10;`
 - `uint8_t column = (output & ~(1<<7)) % 10;`

## Debounce modes
### Mode 0
This mode double-checks an input after `DEBOUNCE_MIN` microseconds before
sending the key state. This produces accurate output, but adds latency between
pressing a key and sending it out. This mode uses more memory and takes longer
to process, but is the default mode due to its accuracy.
### Mode 1
This mode immediately sends the key state after an input is received, but waits
`DEBOUNCE_MIN` microseconds before accepting a new input. This can produce an
inaccurate output if noise causes an input to go high briefly, but reduces
latency between pressing a key and sending it out. This mode uses less memory
and is quicker to process.
