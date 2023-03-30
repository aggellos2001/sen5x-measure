/*
 * I2C-Generator: 0.3.0
 * Yaml Version: 2.1.3
 * Template Version: 0.7.0-109-gb259776
 */
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>   // NAN
#include <stdio.h>  // printf

#include "sen5x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char filename[100];

// μseconds! 1 second = 1000000 μseconds
static const int SLEEP_BETWEEN_MEASUREMENT_USEC = 1000000;

// other constants below are in seconds
static const int IGNORE_SEC_OF_MEASUREMENTS = 60;
static const int MEASURE_FOR_SEC = 300;
static const int SLEEP_UNTIL_NEXT_MEASUREMENT_SEC = 300;

int main(int argc, char** argv) {

    if (argc >= 2) {
        printf("Using file: %s.csv\n", argv[1]);
        strncpy(filename, argv[1], 95);
    } else {
        printf("Input your measurements file name (without .csv): ");
        scanf("%95s", filename);
    }
    if (strlen(filename) > 95) {
        printf("File name too long");
        exit(EXIT_FAILURE);
    }
    // 95 + 4 (".csv") = 99 + 1 (null terminator) = 100
    strcat(filename, ".csv");

    // open the csv file
    FILE* fp;

    fp = fopen(filename, "a");
    if (fp == NULL) {
        // if the file does not exist, create it
        fp = fopen(filename, "w+");
        if (fp == NULL) {
            printf("Error creating file");
            exit(EXIT_FAILURE);
        }
    }

    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 0) {
        // write header
        printf("Creating new file!\n");
        fprintf(fp, "Time,PM1,PM2.5,PM4,PM10,Humidity,Temperature,VOC,NOx\n");
        rewind(fp);
    }

    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = sen5x_device_reset();
    if (error) {
        printf("Error executing sen5x_device_reset(): %i\n", error);
    }

    unsigned char serial_number[32];
    uint8_t serial_number_size = 32;
    error = sen5x_get_serial_number(serial_number, serial_number_size);
    if (error) {
        printf("Error executing sen5x_get_serial_number(): %i\n", error);
    } else {
        printf("Serial number: %s\n", serial_number);
    }

    unsigned char product_name[32];
    uint8_t product_name_size = 32;
    error = sen5x_get_product_name(product_name, product_name_size);
    if (error) {
        printf("Error executing sen5x_get_product_name(): %i\n", error);
    } else {
        printf("Product name: %s\n", product_name);
    }

    uint8_t firmware_major;
    uint8_t firmware_minor;
    bool firmware_debug;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    error = sen5x_get_version(&firmware_major, &firmware_minor, &firmware_debug,
                              &hardware_major, &hardware_minor, &protocol_major,
                              &protocol_minor);
    if (error) {
        printf("Error executing sen5x_get_version(): %i\n", error);
    } else {
        printf("Firmware: %u.%u, Hardware: %u.%u\n", firmware_major,
               firmware_minor, hardware_major, hardware_minor);
    }

    float temp_offset = 0.0f;
    error = sen5x_set_temperature_offset_simple(temp_offset);
    if (error) {
        printf("Error executing sen5x_set_temperature_offset_simple(): %i\n",
               error);
    } else {
        printf("Temperature Offset set to %.2f °C (SEN54/SEN55 only)\n",
               temp_offset);
    }

loop:
    if (fp == NULL) {
        fp = fopen(filename, "a");
        if (fp == NULL) {
            printf("Error opening file");
            exit(EXIT_FAILURE);
        }
    }
    printf("Starting measurement...\n");
    // we use the counter to keep track
    // of how many successful measurements we have
    // to calculate the average later
    float mass_concentration_pm1p0 = 0.0;
    float mass_concentration_pm1p0_total = 0.0;
    int32_t mass_concentration_pm1p0_counter = 0;

    float mass_concentration_pm2p5 = 0.0;
    float mass_concentration_pm2p5_total = 0.0;
    int32_t mass_concentration_pm2p5_counter = 0;

    float mass_concentration_pm4p0 = 0.0;
    float mass_concentration_pm4p0_total = 0.0;
    int32_t mass_concentration_pm4p0_counter = 0.0;

    float mass_concentration_pm10p0 = 0.0;
    float mass_concentration_pm10p0_total = 0.0;
    int32_t mass_concentration_pm10p0_counter = 0;

    float ambient_humidity = 0.0;
    float ambient_humidity_total = 0.0;
    int32_t ambient_humidity_counter = 0;

    float ambient_temperature = 0.0;
    float ambient_temperature_total = 0.0;
    int32_t ambient_temperature_counter = 0;

    float voc_index = 0.0;
    float voc_index_total = 0.0;
    int32_t voc_index_counter = 0;

    float nox_index = 0.0;
    float nox_index_total = 0.0;
    int32_t nox_index_counter = 0.0;

    /* each loop has sleep time of 1 second + some processing time
    This loop will run for 2 minutes approximately
    */

    // Start Measurement
    error = sen5x_start_measurement();

    if (error) {
        printf("Error executing sen5x_start_measurement(): %i\n", error);
        // wait 60 seconds before trying again
        sleep(60);
        goto loop;
    }

    for (int c = 0; c < MEASURE_FOR_SEC; c++) {
        // Read Measurement
        sensirion_i2c_hal_sleep_usec(SLEEP_BETWEEN_MEASUREMENT_USEC);

        error = sen5x_read_measured_values(
            &mass_concentration_pm1p0, &mass_concentration_pm2p5,
            &mass_concentration_pm4p0, &mass_concentration_pm10p0,
            &ambient_humidity, &ambient_temperature, &voc_index, &nox_index);
        if (error) {
            printf("Error executing sen5x_read_measured_values(): %i\n", error);
            continue;
        }

        if (c < IGNORE_SEC_OF_MEASUREMENTS) {
            continue;
        }

        if (!isnan(mass_concentration_pm1p0)) {
            mass_concentration_pm1p0_total += mass_concentration_pm1p0;
            mass_concentration_pm1p0_counter++;
        }
        if (!isnan(mass_concentration_pm2p5)) {
            mass_concentration_pm2p5_total += mass_concentration_pm2p5;
            mass_concentration_pm2p5_counter++;
        }
        if (!isnan(mass_concentration_pm4p0) &&
            mass_concentration_pm4p0 > 0.0f) {
            mass_concentration_pm4p0_total += mass_concentration_pm4p0;
            mass_concentration_pm4p0_counter++;
        }
        if (!isnan(mass_concentration_pm10p0) &&
            mass_concentration_pm10p0 > 0.0f) {
            mass_concentration_pm10p0_total += mass_concentration_pm10p0;
            mass_concentration_pm10p0_counter++;
        }
        if (!isnan(ambient_humidity) && ambient_humidity > 0.0f) {
            ambient_humidity_total += ambient_humidity;
            ambient_humidity_counter++;
        }
        if (!isnan(ambient_temperature) && ambient_temperature > -40.0f) {
            ambient_temperature_total += ambient_temperature;
            ambient_temperature_counter++;
        }
        if (!isnan(voc_index) && voc_index > 0) {
            voc_index_total += voc_index;
            voc_index_counter++;
        }
        if (!isnan(nox_index) && nox_index > 0) {
            nox_index_total += nox_index;
            nox_index_counter++;
        }
    }
    error = sen5x_stop_measurement();

    if (error) {
        printf("Error executing sen5x_stop_measurement(): %i\n", error);
        // wait 60 seconds before trying again
        sleep(60);
        goto loop;
    }

    /*
    At this point, we have collected 2 minutes of data and can calculate the
    average values. (we ignore any NaN values)
    */

    // calculate the average values
    mass_concentration_pm1p0_total /= mass_concentration_pm1p0_counter != 0
                                          ? mass_concentration_pm1p0_counter
                                          : 1;
    mass_concentration_pm2p5_total /= mass_concentration_pm2p5_counter != 0
                                          ? mass_concentration_pm2p5_counter
                                          : 1;
    mass_concentration_pm4p0_total /= mass_concentration_pm4p0_counter != 0
                                          ? mass_concentration_pm4p0_counter
                                          : 1;
    mass_concentration_pm10p0_total /= mass_concentration_pm10p0_counter != 0
                                           ? mass_concentration_pm10p0_counter
                                           : 1;
    ambient_humidity_total /=
        ambient_humidity_counter != 0 ? ambient_humidity_counter : 1;
    ambient_temperature_total /=
        ambient_temperature_counter != 0 ? ambient_temperature_counter : 1;
    voc_index_total /= voc_index_counter != 0 ? voc_index_counter : 1;
    nox_index_total /= nox_index_counter != 0 ? nox_index_counter : 1;

    // print the average values one line concisely
    printf("PM1.0: %.1f μg/m3, PM2.5: %.1f μg/m3, PM4.0: %.1f μg/m3, PM10.0: "
           "%.1f μg/m3, Humidity: %.1f %%RH, Temperature: %.1f °C, VOC: %.1f, "
           "NOx: %.1f\n",
           mass_concentration_pm1p0, mass_concentration_pm2p5,
           mass_concentration_pm4p0, mass_concentration_pm10p0,
           ambient_humidity, ambient_temperature, voc_index, nox_index);

    // print the average values to the csv file + timestamp
    // fprintf(fp, "%.2f,%2.f,%2.f,%2.f,%2.f,%2.f,%2.f,%2.f");
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    fprintf(fp, "%ld,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", ts.tv_sec,
            mass_concentration_pm1p0, mass_concentration_pm2p5,
            mass_concentration_pm4p0, mass_concentration_pm10p0,
            ambient_humidity, ambient_temperature, voc_index, nox_index);
    fflush(fp);

    fclose(fp);
    // if we don't assign null to fp, the next time we try to open the file, it
    // will fail because fclose doesn't always set the pointer to null in time
    // fixes: double free error
    fp = NULL;

    // the loop
    printf("Sleeping and waiting 5 minutes...\n");
    sleep(SLEEP_UNTIL_NEXT_MEASUREMENT_SEC);
    goto loop;

    return 0;
}
