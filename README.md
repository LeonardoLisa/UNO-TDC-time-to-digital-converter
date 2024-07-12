## About
The UNO-TDC is a Time to Digital Converter (TDC) that performs the function of a stopwatch and measures the elapsed time between a START pulse and up to five STOP pulses.
The UNO-TDC is a cheap effective acquisition system for nuclear physics experiments. It can be build using off the shelf components: only an Arduino UNO and a Texas Instruments TDC7200 are required.

## Specifications overview
The UNO-TDC has a measurement range of 14ns to 3926ns, and a dead time of 100us between two consecutive measurements. UNO-TDC must always receive a STOP signal because it does not feature a STOP timeout. The measurement resolution is 55ps and the error varies according to the measurement range as described in the "UNO-TDC datasheet". As a "rule of thumb" 1% error could be considered on all ranges. The data is not saved but continuously streamed to the computer on the serial port.

## Installation
### Hardware
The hardware is custom made and has to be built following the provided schematic. In future revisions Gerber files for creating an Arduino UNO shield may be added.

![](https://github.com/LeonardoLisa/UNO-TDC/blob/main/Hardware/tdc_img.jpg)

### Firmware
The firmware can be directly flashed on the Arduino using the Arduino IDE or with an external programmer.

### Data Parser
The data parser has be compiled so you must have already install a C++ compiler, if you are using Windows. 

```console
make
```

Right now it is only compatible with Linux machines. 

## Usage
### Acquiring data
1. Before starting the data acquisition the serial port on your computer must be configured as follows:
- Parity = no parity bit
- Baud rate = 38400 

2. Connect the UNO-TDC to your computer and open the serial port with the console or any other software of your choice. For convenience you may want to use a data logging program like Rufus. On Arduino IDE simply open the serial monitor.

3. To start the data acquisition the UNO-TDC must receive a random character on the serial port from the computer.

4. When the acquisitions begins a start up message will appear on the serial port ad will start to continuously stream.
![](https://github.com/LeonardoLisa/UNO-TDC/blob/main/Hardware/output_log.svg)

5. To interrupt the measurement session close the serial port and disconnect the UNO-TDC from the computer. 

### Parsing Data
The data parsing software is a console program, it can be run as follows.

```console
.\main data_filename 3700
```
It expects 2 parameters:
- data_filename: the raw log output from UNO-TDC.
- stop_limit: upper limit in nanoseconds, where any longer measurements will be ignored. 
