#include <modbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {

    modbus_t *ctx;
    uint16_t registers[6];
    int rc;
    char button_press;

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

    while (1) {
        printf("Press 'b' and Enter to request current time: ");
        scanf(" %c", &button_press);

        if (button_press == 'b') {
            // Read the time from the first 6 registers of the slave
            rc = modbus_read_registers(ctx, 0, 6, registers);
            if (rc == -1) {
                fprintf(stderr, "Failed to read registers\n");
                break;
            }

            // Print the received time
            printf("Received time: %04d-%02d-%02d %02d:%02d:%02d\n",
                   registers[0], registers[1], registers[2],
                   registers[3], registers[4], registers[5]);
        }
    }

    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}

