#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef enum SensorMode { ALL, GAS, IDLE } SensorMode;
typedef enum RTHMode { LOW, MEDIUM, HIGH } RTHMode;

typedef struct Config {
    struct Sensor {
        struct OperationMode {
            SensorMode main;
            SensorMode secondary;
        } operation_mode;
        bool clean_fan;
        RTHMode rth_mode;
        float temp_offset;
    } sensor;
    struct Measurement {
        int wait_between_measurements_for;
        int take_measurements_for;
        int sleep_until_next_batch_of_measurements_for;
        int ignore_first_n_measurements;
    } measurement;
    struct Console {
        bool verbose;
    } console;
} Config;

Config parse_config();

#endif