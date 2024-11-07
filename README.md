<div align="center"><img src="\Hardware\Pictures\UNO-tdc_shield.jpg" alt="UNO-TDC Time to Digital Converter TDC Arduino shield" width="400"/></div>

<h4 align="center">Be sure to :star: my repo so you can keep up to date on any progress!</h4>

## About
The UNO-TDC is a Time to Digital Converter (TDC) that performs the function of a stopwatch and measures the elapsed time between a START pulse and up to five STOP pulses.
The UNO-TDC is a cheap effective acquisition system for nuclear physics experiments. It can be built using off the shelf component like an Arduino UNO and a Texas Instruments TDC7200.

## Specifications overview
The UNO-TDC has a measurement range of 14ns to 3926ns, and a dead time of 100us between two consecutive measurements. UNO-TDC must always receive a STOP signal because it does not feature a STOP timeout. The measurement resolution is 55ps and the error varies according to the measurement range as described in the "UNO-TDC datasheet". As a "rule of thumb" 1% error could be considered on all ranges. The data is not saved but continuously streamed to the computer on the serial port.

## License
This repository uses a complex multilicense system 
- All shared materials in the Hardware directory are shared under the [Creative Commons BY-NC-SA 4](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.en) license.  
- All software in the `\Software` directory is shared under [GPLv3](https://www.gnu.org/licenses/quick-guide-gplv3.html) the license.

## Installation
### Hardware
The hardware is custom made and you have to build it your self. However, it is an opensource project so all production files are available! There are two options: hand soldering all the components following the provided schematic or contact your favorite pcb manifacturer and order the necessary circuit board using the gerbers file under the `\Hardware\Production` directory. If you aim to have your UNO-tdc shield assembled as well, the assembly file and the bill of materials (BOM), are available under the `\Hardware` directory. In the future it may be possible to directly buy a fully tested and assembled board.
Warning: the current implementations use a 16 MHz frequency reference while the Texas Instruments TDC7200EVM evaluation board uses a 8 MHz voltage frequency reference.

![](https://github.com/LeonardoLisa/UNO-TDC/blob/main/Hardware/Pictures/UNO-tdc_pcb.png)

### Firmware
The firmware can be directly flashed on the Arduino using the Arduino IDE or with an external programmer.
At compilation time it is possible to choose between three data output formats:
1. Prints only the data cointained in the measurement registers (recommended). Comment line 37 and 38.
2. Prints data cointained in the measurement registers, and the registers' names. Only comment line 37.
3. Prints the time of flight computed by the Arduino UNO (not recommended). Only comment line 38.
These are the snippets of code that have to be changed in order to select the data ouput format:

```cpp
/* Data format option */
#define EASYDATA
//#define RAWDATA
```

Please note that the Arduino UNO is based on the Atmega328-PB which is an 8 bit AVR microcontroller that doesn't natively support floting point operations. Therefore calculating the time of flight at runtime uses a lot of resouces and may lead to data loss due to full measurement serial buffer. The Arduino will inform you using a message when a measurement is lost. This is the reason why using option 1 is recommended.

With regards to option 3, the measurement values are calculated according to a 16 MHz frequency reference. In order to use a 8 MHz frequency reference line 170 has to be changed to:

```cpp
Serial.println((rawTime * 1125) / Difference);
```

### Data Parser
The data parser is needed only if you have compiled the firmware using option 1 or 2. The data parser has be compiled so you must have already install a C++ compiler. 

```console
make
```

Right now it is only compatible with Linux machines. The measurement values are calculated according to a 16 MHz frequency reference. If you are using a 8 MHz frequency reference line 207 has to be changed to:

```cpp
tof = (rawMeasurement.time * 1125) / calcount;
```

## Usage
### Acquiring data
1. Before starting the data acquisition the serial port on your computer must be configured as follows:
- Parity = no parity bit
- Baud rate = 38400 

2. Connect the UNO-TDC to your computer and open the serial port with the console or any other software of your choice. For convenience you may want to use a data logging program like Rufus. On Arduino IDE simply open the serial monitor.

3. To start the data acquisition the UNO-TDC must receive a character or a string on the serial port from the computer.

4. When the acquisition begins a start up message will appear on the serial port and will start to continuously stream.
![](https://github.com/LeonardoLisa/UNO-TDC/blob/main/Hardware/output_log.svg)

5. To interrupt the measurement session close the serial port and disconnect the UNO-TDC from the computer. 

### Parsing Data
The data parser is needed only if you have compiled the firmware using option 1 or 2. The data parsing software is a console program, it can be run as follows.

```console
.\main data_filename 3700
```
It expects 2 parameters:
- data_filename: the raw log output from UNO-TDC.
- stop_limit: upper limit in nanoseconds, where any longer measurements will be ignored. 
