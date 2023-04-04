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


## Compilation

To compile the file run
```bash
make
```
This will create a new binary called "measure" in the bin/ folder.

**After the compilation make sure to copy config.toml to bin/**

```bash
cp config.toml bin/
```

## Usage

You can now either give the name of the csv file as an argument or when the program asks for it.

**Do NOT include .csv**

### Arguments:

```bash
cd bin
./measure somename
```
Will create **somename.csv**.

### User input:

```bash
cd bin/
./measure ή make run

Input your measurements file name (without .csv): somename
```
Will also create **somename.csv**.


## Graph generation

Για την δημιουργία διαγραμμάτων μπορεί να χρησιμοποιηθεί το script `make_diagram.py`.

Μπορεί να τρέξει με την εντολή:

```bash
python script.py
```
το οποίο θα ζητήσει το όνομα του αρχείου CSV που περιέχει τις μετρήσεις πχ `test.csv`.

Επίσης μπορεί να τρέξει χρησιμοποιώντας τα file arguments:

```bash
python script.py -f /path/to/your/csv/test.csv
```
ώστε να τρέχει όπου και αν βρίσκεται το αρχείο.
