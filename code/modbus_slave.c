#include <modbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

int main() {

    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    uint8_t query[MODBUS_RTU_MAX_ADU_LENGTH];
    int rc;

    // Initialize Modbus RTU
    ctx = modbus_new_rtu("/dev/ttyS1", 9600, 'N', 8, 1);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return -1;
    }

/*
    // Enable debug mode for detailed logging
    modbus_set_debug(ctx, TRUE);
    
    // Set the RS-485 mode
    rc = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
    printf("modbus_rtu_set_serial_mode() returned: %d\n", rc);  // Print return value of set function
    
    if (rc == -1) {
    	fprintf(stderr, "Failed to read registers: %s\n", modbus_strerror(errno));
    	break;
    }
    
    // Check if the mode was set correctly
    int mode = modbus_rtu_get_serial_mode(ctx);
    printf("Current serial mode (should be RS-485): %d\n", mode);  // Print the current mode
*/
    
    modbus_set_slave(ctx, 1);

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed\n");
        modbus_free(ctx);
        return -1;
    }

    // Allocate and initialize the memory for the Modbus mapping
    mb_mapping = modbus_mapping_new(0, 0, 6, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    while (1) {
        // Wait for a query from the master
        rc = modbus_receive(ctx, query);
        if (rc > 0) {
            // Get the current time
            time_t rawtime;
            struct tm *timeinfo;

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            // Pack the time into registers (Year, Month, Day, Hour, Minute, Second)
            mb_mapping->tab_registers[0] = timeinfo->tm_year + 1900; // Year
            mb_mapping->tab_registers[1] = timeinfo->tm_mon + 1;     // Month
            mb_mapping->tab_registers[2] = timeinfo->tm_mday;        // Day
            mb_mapping->tab_registers[3] = timeinfo->tm_hour;        // Hour
            mb_mapping->tab_registers[4] = timeinfo->tm_min;         // Minute
            mb_mapping->tab_registers[5] = timeinfo->tm_sec;         // Second

            // Send the current time back to the master
            modbus_reply(ctx, query, rc, mb_mapping);
        }
    }

    // Free the Modbus mapping memory
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}

