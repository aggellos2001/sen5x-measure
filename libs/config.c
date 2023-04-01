#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "toml.h"

Config parse_config() {
    FILE* fp;
    char errbuff[200];
    Config config_struct;

    fp = fopen("config.toml", "r");
    if (!fp) {
        printf("Error opening config file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    toml_table_t* config = toml_parse_file(fp, errbuff, sizeof(errbuff));
    fclose(fp);

    if (!config) {
        printf("Error parsing config file: %s\n", errbuff);
        exit(EXIT_FAILURE);
    }

    toml_table_t* sensor = toml_table_in(config, "sensor");
    toml_table_t* measurement = toml_table_in(config, "measurement");

    if (!sensor || !measurement) {
        printf("Error parsing config file: missing sensor or measurement "
               "section\n");
        exit(EXIT_FAILURE);
    }

    toml_table_t* op_mode = toml_table_in(sensor, "operation-mode");
    if (!op_mode) {
        printf("Error parsing config file: missing operation-mode array\n");
        exit(EXIT_FAILURE);
    }

    toml_datum_t main_op_mode = toml_string_in(op_mode, "main");
    toml_datum_t secondary_op_mode = toml_string_in(op_mode, "secondary");
    toml_datum_t clean_fan = toml_bool_in(sensor, "clean-fan");
    toml_datum_t rht_accel_mode = toml_int_in(sensor, "rht-accel-mode");
    toml_datum_t temp_offset = toml_double_in(sensor, "temp-offset");

    if (!main_op_mode.ok || !secondary_op_mode.ok) {
        printf("Error parsing config file: missing operation-mode array\n");
        exit(EXIT_FAILURE);
    }
    if (!clean_fan.ok) {
        printf("Error parsing config file: missing clean-fan boolean\n");
        exit(EXIT_FAILURE);
    }
    if (!rht_accel_mode.ok) {
        printf("Error parsing config file: missing rht-accel-mode integer\n");
        exit(EXIT_FAILURE);
    }
    if (!temp_offset.ok) {
        printf("Error parsing config file: missing temp-offset double\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(main_op_mode.u.s, "all") == 0) {
        config_struct.sensor.operation_mode.main = ALL;
    } else if (strcmp(main_op_mode.u.s, "gas") == 0) {
        config_struct.sensor.operation_mode.main = GAS;
    } else {
        printf("Error parsing config file: invalid operation mode\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(secondary_op_mode.u.s, "gas") == 0) {
        config_struct.sensor.operation_mode.secondary = GAS;
    } else if (strcmp(secondary_op_mode.u.s, "idle") == 0) {
        config_struct.sensor.operation_mode.secondary = IDLE;
    } else {
        printf("Error parsing config file: invalid operation mode\n");
        exit(EXIT_FAILURE);
    }

    config_struct.sensor.clean_fan = clean_fan.u.b;
    config_struct.sensor.rth_mode = rht_accel_mode.u.i;
    config_struct.sensor.temp_offset = temp_offset.u.d;
    free(main_op_mode.u.s);
    free(secondary_op_mode.u.s);

    toml_datum_t wait_between_measurements_for =
        toml_int_in(measurement, "wait-between-measurements-for");
    toml_datum_t take_measurements_for =
        toml_int_in(measurement, "take-measurements-for");
    toml_datum_t sleep_until_next_batch_of_measurements_for =
        toml_int_in(measurement, "sleep-until-next-batch-of-measurements-for");
    toml_datum_t ignore_first_n_measurements =
        toml_int_in(measurement, "ignore-first-n-measurements");

    if (!wait_between_measurements_for.ok) {
        printf(
            "Error parsing config file: missing wait-between-measurements-for "
            "integer\n");
        exit(EXIT_FAILURE);
    }
    if (!take_measurements_for.ok) {
        printf("Error parsing config file: missing take-measurements-for "
               "integer\n");
        exit(EXIT_FAILURE);
    }
    if (!sleep_until_next_batch_of_measurements_for.ok) {
        printf("Error parsing config file: missing "
               "sleep-until-next-batch-of-measurements-for integer\n");
        exit(EXIT_FAILURE);
    }
    if (!ignore_first_n_measurements.ok) {
        printf("Error parsing config file: missing ignore-first-n-measurements "
               "integer\n");
        exit(EXIT_FAILURE);
    }

    config_struct.measurement.wait_between_measurements_for =
        wait_between_measurements_for.u.i;
    config_struct.measurement.take_measurements_for = take_measurements_for.u.i;
    config_struct.measurement.sleep_until_next_batch_of_measurements_for =
        sleep_until_next_batch_of_measurements_for.u.i;
    config_struct.measurement.ignore_first_n_measurements =
        ignore_first_n_measurements.u.i;

    toml_table_t* console = toml_table_in(config, "console");
    if (!console) {
        printf("Error parsing config file: missing console section\n");
    }

    toml_datum_t verbose = toml_bool_in(console, "verbose");
    if (!verbose.ok) {
        printf("Error parsing config file: missing verbose boolean\n");
        exit(EXIT_FAILURE);
    }

    config_struct.console.verbose = verbose.u.b;

    printf("Config file parsed successfully!\n");
    printf("Operation mode: Main:%d,  Secondary:%d\n",
           config_struct.sensor.operation_mode.main,
           config_struct.sensor.operation_mode.secondary);
    printf("Clean fan: %d\n", config_struct.sensor.clean_fan);
    printf("RHT mode: %d\n", config_struct.sensor.rth_mode);
    printf("Temp offset: %.1f\n", config_struct.sensor.temp_offset);
    printf("Wait between measurements for: %d seconds\n",
           config_struct.measurement.wait_between_measurements_for);
    printf("Take measurements for: %d seconds\n",
           config_struct.measurement.take_measurements_for);
    printf(
        "Sleep until next batch of measurements for: %d seconds\n",
        config_struct.measurement.sleep_until_next_batch_of_measurements_for);
    printf("Ignore first n measurements: %d\n",
           config_struct.measurement.ignore_first_n_measurements);
    printf("Verbose: %d\n", config_struct.console.verbose);

    toml_free(config);
    return config_struct;
}