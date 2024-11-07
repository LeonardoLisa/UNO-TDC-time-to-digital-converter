/*
 * UNO-TDC.ino
 * 
 * Version: 1.10
 * Date: 05/11/2024
 * Authors: Leonardo Lisa and Caterina Morgavi
 * leonardo.lisa@studenti.unimi.it
 * caterina.morgavi@studenti.unimi.it
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY
 * APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT
 * HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
 * IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
 * ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

/* Data format option */
#define EASYDATA
//#define RAWDATA

/* TSC7200 enable time */
#define TDC_ENABLE_TIME 2 // ms

/* TDC7200 register mapping */
#define CONFIG1 0x00
#define CONFIG2 0x01
#define INT_MASK 0x03
#define COARSE_CNTR_OVF_H 0x04
#define COARSE_CNTR_OVF_L 0x05
#define TIME1 0x10
#define CALIBRATION1 0x1b
#define CALIBRATION2 0x1c

/* TDC7200 command mask */
#define RW 0x40
#define AUTO_INC_MODE 0x80

/* ArduinoUNO pin mapping */
#define SCLK 0x20   // PB5 13 -> TDC7200 SCLK
#define CS 0x10     // PB4 12 -> TDC7200 CSB
#define DIN 0x08    // PB3 11 -> TDC7200 DOUT
// DON'T CHANGE DIN pin see getByte() for more info
// #define DIN 0x01 // PB2 8 -> TDC7200 DIN
#define DOUT 0x04   // PB2 10 -> TDC7200 DIN
#define ENABLE 0x02 // PB1 9  -> TDC7200 ENABLE
#define TRIG 0x04   // PD2 2 (INT0) -> TDC7200 TRIG

/* Serial TX buffer */
#define PENDING_DATA 1
#define DATA_SENT 0
#define BUF_SIZE 4
#define IS_LOST 1

/* Data struct */
class UNO_TDC
{
public:
  uint8_t time1_r[3]; // Why use uint24_t when you only need a 24bit container?
  uint8_t cal1_r[3];
  uint8_t cal2_r[3];
  uint8_t status = DATA_SENT;
};

/* Serial circular buffer */
volatile UNO_TDC buf[BUF_SIZE];
volatile uint8_t buf_index = 0;      // Buffer index
volatile uint8_t data_lost_flag = 0; // Buffer is full

/* Functions declarations */
void shiftOutMSB(uint8_t _data);
void shiftOutLSB(uint8_t _data);
void initializeTDC(void);
void startMeasurement(void);
uint8_t getByte(void);
void getMeasurement(void);
void checkRegisters(void);
void printData(uint8_t _index);

void setup()
{
  initializeTDC();
  Serial.begin(38400);
  delay(100);

  Serial.println(F("Press any key to start"));
  while (Serial.available() == 0)
  {
    __asm__("nop\n\t");
  }

  // Only for debug: it checks if the Arduino is correcly reading the tdc7200 registers.
  // checkRegisters();
  
  Serial.println(F("$$\\   $$\\$$\\   $$\\ $$$$$$\\   $$\\          $$\\"));
  Serial.println(F("$$ |  $$ $$$\\  $$ $$  __$$\\  $$ |         $$ |"));
  Serial.println(F("$$ |  $$ $$$$\\ $$ $$ /  $$ $$$$$$\\   $$$$$$$ |$$$$$$$\\"));
  Serial.println(F("$$ |  $$ $$ $$\\$$ $$ |  $$ \\_$$  _| $$  __$$ $$  _____|"));
  Serial.println(F("$$ |  $$ $$ \\$$$$ $$ |  $$ | $$ |   $$ /  $$ $$ /"));
  Serial.println(F("$$ |  $$ $$ |\\$$$ $$ |  $$ | $$ |$$\\$$ |  $$ $$ |"));
  Serial.println(F("\\$$$$$$  $$ | \\$$ |$$$$$$  | \\$$$$  \\$$$$$$$ \\$$$$$$$\\"));
  Serial.println(F(" \\______/\\__|  \\__|\\______/   \\____/ \\_______|\\_______|"));
  Serial.println();
  Serial.println(F("Università degli Studi di Milano, Milano 02/05/2024"));
  Serial.println();
  Serial.println(F("UNO-TDC Copyright (C) 2024 Leonardo Lisa and Caterina Morgavi"));
  Serial.println(F("This program comes with ABSOLUTELY NO WARRANTY."));
  Serial.println(F("This is free software, and you are welcome to redistribute it under the GNU General Public License v3 conditions."));
  Serial.println();
  startMeasurement();
}

void loop()
{
  /* Send measurements */
  if (buf[buf_index].status == PENDING_DATA)
  {
    printData(buf_index);
    buf[buf_index].status = DATA_SENT;
  }

  if (!!(data_lost_flag) == IS_LOST)
  {
    Serial.print(F("ERROR! "));
    Serial.print(data_lost_flag);
    Serial.println(F(" measurements lost :("));
    cli(); // Disable interrupts
    data_lost_flag = 0;
    sei(); // Enable interrupts
  }

  if (buf_index >= (BUF_SIZE - 1))
  {
    buf_index = 0;
  }
  else
  {
    buf_index++;
  }
}

void printData(uint8_t _index)
{
#ifdef EASYDATA
  double Difference = static_cast<uint32_t>(buf[_index].cal2_r[2] - buf[_index].cal1_r[2])*(uint32_t)65536 +
                        static_cast<uint32_t>(buf[_index].cal2_r[1] - buf[_index].cal1_r[1])*(uint32_t)256 +
                        static_cast<uint32_t>(buf[_index].cal2_r[0] - buf[_index].cal1_r[0]);
  double rawTime = static_cast<uint32_t>(buf[_index].time1_r[2])*(uint32_t)65536 +
                   static_cast<uint32_t>(buf[_index].time1_r[1])*(uint32_t)256 +
                   static_cast<uint32_t>(buf[_index].time1_r[0]);
  Serial.print(F("ToF is: "));
  Serial.println((rawTime * 562.5) / Difference);
#elif defined(RAWDATA)
  Serial.print(F("time1_r = "));
  Serial.print(buf[_index].time1_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].time1_r[1]);
  Serial.write(' ');
  Serial.println(buf[_index].time1_r[0]);

  Serial.print(F("cal1_r = "));
  Serial.print(buf[_index].cal1_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].cal1_r[1]);
  Serial.write(' ');
  Serial.println(buf[_index].cal1_r[0]);

  Serial.print(F("cal2_r = "));
  Serial.print(buf[_index].cal2_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].cal2_r[1]);
  Serial.write(' ');
  Serial.println(buf[_index].cal2_r[0]);
#else 
  Serial.print(buf[_index].time1_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].time1_r[1]);
  Serial.write(' ');
  Serial.print(buf[_index].time1_r[0]);
  Serial.write(' ');
  Serial.print(buf[_index].cal1_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].cal1_r[1]);
  Serial.write(' ');
  Serial.print(buf[_index].cal1_r[0]);
  Serial.write(' ');
  Serial.print(buf[_index].cal2_r[2]);
  Serial.write(' ');
  Serial.print(buf[_index].cal2_r[1]);
  Serial.write(' ');
  Serial.println(buf[_index].cal2_r[0]);
#endif
}

void shiftOutLSB(uint8_t _data)
{
  for (uint8_t j = 1; j < 0b10000000; j <<= 1)
  {
    if ((_data & j) == 0)
    {
      PORTB &= ~DOUT;
    }
    else
    {
      PORTB |= DOUT;
    }

    PORTB |= SCLK;
    PORTB &= ~SCLK & ~DOUT;
  }

  if ((_data & 0b10000000) == 0)
  {
    PORTB &= ~DOUT;
  }
  else
  {
    PORTB |= DOUT;
  }
  PORTB |= SCLK;
  PORTB &= ~SCLK & ~DOUT;
}

void shiftOutMSB(uint8_t _data)
{
  for (uint8_t j = 0b10000000; j > 0b00000001; j >>= 1)
  {
    if ((_data & j) == 0)
    {
      PORTB &= ~DOUT;
    }
    else
    {
      PORTB |= DOUT;
    }

    PORTB |= SCLK;
    __asm__("nop\n\t");
    PORTB &= ~SCLK & ~DOUT;
  }

  if ((_data & 0b00000001) == 0)
  {
    PORTB &= ~DOUT;
  }
  else
  {
    PORTB |= DOUT;
  }
  PORTB |= SCLK;
  __asm__("nop\n\t");
  PORTB &= ~SCLK & ~DOUT;
}

void initializeTDC(void)
{
  /* Pinmode settings */
  cli(); // Disable interrupts
  // Port D
  DDRD &= (~TRIG);
  PORTD &= (~TRIG);
  //PORTD |= TRIG; // Enable 50k internal pullup resistor.
  // Port B
  DDRB |= CS | SCLK | DOUT | ENABLE;
  DDRB &= ~DIN;
  PORTB |= ENABLE | CS;
  PORTB &= ~(SCLK | DOUT | DIN);
  sei(); // Enable interrupts

  delay(TDC_ENABLE_TIME);

  /* TDC7200 register setup */
  cli(); // Disable interrupts

  // CONFIG1 configuration
  // 0 Start New Measurement
  // 00 Measurement Mode 1
  // 0 Measurement is started on Rising edge of START signal
  // 0 Measurement is stopped on Rising edge of STOP signal
  // 0 TRIGG is output as a Rising edge signal
  // 0 Parity bit for Measurement Result Registers disabled (Parity Bit always 0)
  // 0 Calibration is not performed after interrupted measurement (for example, due to counter overflow or missing STOP signal)
  // 0b00000000 is the reset value.

  // CONFIG2 configuration
  // 01 calibration, measuring 10 CLOCK periods
  // 000 no Multi-Cycle Averaging Mode
  // 000 Single stop
  // 0b01000000
  PORTB &= ~CS;                // Start transaction
  shiftOutMSB((CONFIG2 | RW)); // Send write command
  shiftOutMSB(0b01000000);
  PORTB |= CS; // End transaction

  __asm__("nop\n\t"); // Minimum pause time (CS high) is 40ns

  // INT_MASK configuration
  // 00000 reserved
  // 0 CLOCK Counter Overflow Interrupt disabled
  // 0 Coarse Counter Overflow Interrupt disabled
  // 1 New Measurement Interrupt
  PORTB &= ~CS;                 // Start transaction
  shiftOutMSB((INT_MASK | RW)); // Send write command
  shiftOutMSB(0b00000001);
  PORTB |= CS; // End transaction

  /* Setting ISR */
  // 0bxxxxxx10 attach interrupt on TRIG to trigger on falling edge.
  EICRA = 0b00000010;
  // 0bxxxxxxx1 enable external interrupt on TRIG
  EIMSK = 0b00000001;

  sei(); // Enable interrupts
}

void startMeasurement(void)
{
  cli(); // Disable interrupts

  PORTB &= ~CS;                // Start transaction
  shiftOutMSB((CONFIG1 | RW)); // Send write command
  shiftOutMSB(0b00000001);
  // CONFIG1 configuration
  // 0 No FORCE_CAL
  // 0 No parity
  // 0
  // 0 Measurement is stopped on Rising edge of STOP signal
  // 0 Measurement is started on Rising edge of START signal
  // 00 Measurement Mode 1
  // 1 Start New Measurement
  // 0b00000001
  PORTB |= CS; // End transaction

  sei(); // Enable interrupts
}

uint8_t getByte(void)
{
  uint8_t _data = 0;
  // MSB first
  for (uint8_t i = 0x80; i > 0; i >>= 1)
  {
    PORTB |= SCLK; // Data on falling edge
    PORTB &= ~SCLK;
    if (!!(PINB & DIN))
    {
      _data |= i;
    }
  }
  PORTB |= SCLK;
  PORTB &= ~SCLK;
  if (!!(PINB & DIN))
  {
    _data |= 1;
  }

  return _data;
}

void getMeasurement(void)
{
  uint8_t _index = BUF_SIZE;

  cli(); // Disable interrupts

  /* Finding a spot for Data */
  for (uint8_t i = 0; i < BUF_SIZE; i++)
  {
    if (buf[i].status == DATA_SENT) // Add "(i != acquire_flag) &&" if you edit loop code
    {
      _index = i;
    }
  }
  // Fatal error: measure abort
  // No available space in serial buffer
  if (_index == BUF_SIZE)
  {
    // Take an action?
    sei();
    data_lost_flag++;
    return;
  }

  /* Harvesting data from TDC7200 */
  PORTB &= ~CS; // Start transaction
  // Send command
  // 0x80 for auto-increment, 0x40 set R or W mode (1 is W)
  shiftOutMSB(TIME1 | AUTO_INC_MODE); // Request TIME1 data
  // Most significant BYTE first
  for (uint8_t i = 0; i < 3; i++)
  {
    buf[_index].time1_r[2 - i] = getByte();
  }
  PORTB |= CS; // End transaction

  __asm__("nop\n\t"); // Minimum pause time (CS high) is 40ns

  PORTB &= ~CS;                              // Start transaction
  shiftOutMSB(CALIBRATION1 | AUTO_INC_MODE); // Request CALIBRATION1 data
  // Most significant BYTE first
  for (uint8_t i = 0; i < 3; i++)
  {
    buf[_index].cal1_r[2 - i] = getByte();
  }
  for (uint8_t i = 0; i < 3; i++)
  {
    buf[_index].cal2_r[2 - i] = getByte();
    buf[_index].status = PENDING_DATA;
  }
  PORTB |= CS; // End transaction

  __asm__("nop\n\t"); // Minimum pause time (CS high) is 40ns

  PORTB &= ~CS;                              // Start transaction
  shiftOutMSB(CALIBRATION2 | AUTO_INC_MODE); // Request CALIBRATION2 data
  // Most significant BYTE first
  for (uint8_t i = 0; i < 3; i++)
  {
    buf[_index].cal2_r[2 - i] = getByte();
    buf[_index].status = PENDING_DATA;
  }
  PORTB |= CS; // End transaction

  sei(); // Enable interrupts
}

void checkRegisters(void)
{
  uint8_t _temp;

  // Read CONFIG1 register
  cli();
  PORTB &= ~CS;         // Start transaction
  shiftOutMSB(CONFIG1); // Send read command
  _temp = getByte();
  PORTB |= CS; // End transaction
  sei();
  Serial.print(F("CONFIG1 = "));
  Serial.println(_temp);
  Serial.println(F("Should be 1 or 0"));

  // Read CONFIG2 register
  cli();
  PORTB &= ~CS;         // Start transaction
  shiftOutMSB(CONFIG2); // Send read command
  _temp = getByte();
  PORTB |= CS; // End transaction
  sei();
  Serial.print(F("CONFIG2 = "));
  Serial.println(_temp);
  Serial.println(F("Should be 64"));

  // Reads INT_MASK register
  cli();
  PORTB &= ~CS;          // Start transaction
  shiftOutMSB(INT_MASK); // Send read command
  _temp = getByte();
  PORTB |= CS; // End transaction
  sei();
  Serial.print(F("INT_MASK  = "));
  Serial.println(_temp);
  Serial.println(F("Should be 1"));
}

/* ISR */
// Newest TDC measurement ready for reading
ISR(INT0_vect)
{
  getMeasurement();
  startMeasurement();
}
