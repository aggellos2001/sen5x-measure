# Raspberry PI measurement driver for the Sensirion SEN5x *(tested on SEN55 only)*

This is a modified version of the original [Sensirion/raspberry-pi-i2c-sen5x](https://github.com/Sensirion/raspberry-pi-i2c-sen5x). 

We are using the provided software for our university project and for non-commercial purposes only.

## How it works

Currently, the script will run for 5 minutes, take the average of the measurements and then sleep for another 5 minutes. 

After each successfull run, the script will try to write the data to a file called "measurements.csv" in the same directory as the script's.

Here's a preview of the csv:

```txt
Time,PM1,PM2.5,PM4,PM10,Humidity,Temperature,VOC,NOx
1680033923,5.7,6.0,6.0,6.0,36.5,23.9,116.0,1.0
1680034530,5.2,5.5,5.5,5.5,32.3,25.2,99.0,1.0
1680035137,4.7,4.9,4.9,4.9,30.5,25.3,100.0,1.0
1680035744,4.7,5.0,5.1,5.1,30.0,25.1,105.0,1.0
```
A downloadable version of the file above can be found in the releases section.


