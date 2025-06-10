#include "i2c_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

static const char *device = "/dev/i2c-1"; // Raspberry Pi I2C bus 1
static int i2c_address = 0x1a;
static int i2c_file;

int WM8960_I2c_Init(){
    // Open the I2C device
    if ((i2c_file = open(device, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return 1;
    }

    // Specify the address of the I2C Slave to communicate with
    if (ioctl(i2c_file, I2C_SLAVE, i2c_address) < 0) {
        perror("Failed to acquire bus access or talk to slave");
        close(i2c_file);
        return 1;
    }

    return 0;
}

int WM8960_I2c_Send(uint8_t address_7bit, uint16_t data_9bit){
    // char buffer[2] = {0x00, 0x01}; // For example, register 0x00, data 0x01
    uint16_t buffer = ((address_7bit & 0x7f) << 9) & (data_9bit & 0x1ff);
    printf("add: %x, dat: %x, buf: %x/n");


    if (write(i2c_file, &buffer, 2) != 2) {
        perror("Failed to write to the i2c bus");
        close(i2c_file);
        return 1;
    }
    // Read example: read 2 bytes from the device
    // char read_buf[2];
    // if (read(file, read_buf, 2) != 2) {
    //     perror("Failed to read from the i2c bus");
    //     close(file);
    //     return 1;
    // } else {
    //     printf("Data read: %02x %02x\n", read_buf[0], read_buf[1]);
    // }

    close(i2c_file);
    return 0;
}

int WM8960_I2c_Deinit(){
    close(i2c_file);
    return 0;
}