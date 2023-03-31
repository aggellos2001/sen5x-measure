#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "libs/sen5x_i2c.h"
#include "libs/sensirion_common.h"
#include "libs/sensirion_i2c_hal.h"

// ===CONFIG FROM TOML FILE DO NOT CHANGE DIRECTLY ===
#include "libs/config.h"
#include "libs/toml.h"
Config config;
// ================

static char filename[100];

void stop_sensor_when_terminating() {
    sen5x_stop_measurement();
    printf("Stopping the sensor before terminating...\n");
    exit(EXIT_SUCCESS);
};

int main(int argc, char** argv) {

    // handle CTRL+C
    signal(SIGINT, stop_sensor_when_terminating);

    // parse config file
    config = parse_config();

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
        exit(EXIT_FAILURE);
    }

    unsigned char serial_number[32];
    uint8_t serial_number_size = 32;
    error = sen5x_get_serial_number(serial_number, serial_number_size);
    if (error) {
        printf("Error executing sen5x_get_serial_number(): %i\n", error);
        exit(EXIT_FAILURE);
    } else {
        printf("Serial number: %s\n", serial_number);
    }

    unsigned char product_name[32];
    uint8_t product_name_size = 32;
    error = sen5x_get_product_name(product_name, product_name_size);
    if (error) {
        printf("Error executing sen5x_get_product_name(): %i\n", error);
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    } else {
        printf("Firmware: %u.%u, Hardware: %u.%u\n", firmware_major,
               firmware_minor, hardware_major, hardware_minor);
    }

    error = sen5x_set_rht_acceleration_mode(config.sensor.rth_mode);
    if (error) {
        printf("Error executing sen5x_set_rht_acceleration_mode(): %i\n",
               error);
        exit(EXIT_FAILURE);
    }

    float temp_offset = 0.0f;
    error = sen5x_set_temperature_offset_simple(temp_offset);
    if (error) {
        printf("Error executing sen5x_set_temperature_offset_simple(): %i\n",
               error);
        exit(EXIT_FAILURE);
    } else {
        printf("Temperature Offset set to %.2f °C (SEN54/SEN55 only)\n",
               temp_offset);
    }

    if (config.console.verbose) {
        printf("Time\t PM1\t PM2.5\t PM4\t PM10\t RH\t Temp\t VOC\t NOx\n");
        printf("[s]\t [μg/m3]\t [μg/m3]\t [μg/m3]\t [μg/m3]\t [%]\t [degC]\t "
               "[-]\t [-]\n");
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
    int32_t mass_concentration_pm4p0_counter = 0;

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
    int32_t nox_index_counter = 0;

    int measures_for = config.measurement.take_measurements_for;
    int wait_between_measurements =
        config.measurement.wait_between_measurements_for;

    if (config.sensor.operation_mode.main == ALL) {
        // start Measurement with all sensors
        error = sen5x_start_measurement();
    } else {
        // start Measurement without PM sensors
        // only RHT and VOC/NOx
        error = sen5x_start_measurement_without_pm();
    }

    if (error) {
        printf("Error executing sen5x_start_measurement(): %i\n", error);
        // wait 60 seconds before trying again
        sleep(60);
        goto loop;
    }

    if (config.sensor.clean_fan) {
        sen5x_start_fan_cleaning();
        config.sensor.clean_fan = false;
    }

    for (int c = 0; c < measures_for; c++) {
        // Sleep between measurements for a given time
        // or wait until data is ready if configured
        if (wait_between_measurements > 0) {
            sensirion_i2c_hal_sleep_usec(wait_between_measurements * 1000000);
        } else {
            bool data_ready = false;
            do {
                sen5x_read_data_ready(&data_ready);
            } while (!data_ready);
        }

        // Read Measurement
        error = sen5x_read_measured_values(
            &mass_concentration_pm1p0, &mass_concentration_pm2p5,
            &mass_concentration_pm4p0, &mass_concentration_pm10p0,
            &ambient_humidity, &ambient_temperature, &voc_index, &nox_index);
        if (error) {
            printf("Error executing sen5x_read_measured_values(): %i\n", error);
            continue;
        }

        if (c < config.measurement.ignore_first_n_measurements) {
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

        if (config.console.verbose) {
            printf("%1.f\t %1.f\t %1.f\t %1.f\t %1.f\t %1.f\t %1.f\t %1.f",
                   mass_concentration_pm1p0, mass_concentration_pm2p5,
                   mass_concentration_pm4p0, mass_concentration_pm10p0,
                   ambient_humidity, ambient_temperature, voc_index, nox_index);
        }
    }

    if (config.sensor.operation_mode.secondary == GAS) {
        error = sen5x_start_measurement_without_pm();
    } else if (config.sensor.operation_mode.secondary == IDLE) {
        error = sen5x_stop_measurement();
    }

    if (error) {
        printf("Error executing sen5x_stop_measurement(): %i\n", error);
        // wait 60 seconds before trying again
        sleep(60);
        goto loop;
    }

    /*
    At this point, we have collected X minutes of data and can calculate the
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
    sleep(config.measurement.wait_between_measurements_for);

    goto loop;

    return 0;
}