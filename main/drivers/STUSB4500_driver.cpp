/**
 * @file STUSB4500.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  Driver for the STUSB4500 USB PD controller
 * @version 0.1
 * @date 2024-12-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "STUSB4500_driver.h"
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "stusb4500.h"
#include <cstring>

#include <rom/ets_sys.h>
#include "../pinout.h"
#include "esp_timer.h"




static i2c_master_dev_handle_t i2c_dev_primary_handle;

static void brute_force_write();
static void brute_force_read();


bool my_stusb4500_write(uint16_t addr, uint8_t reg, void const* buf, size_t len, void* context)
{   
    uint8_t reg_buffer = reg;

    uint8_t *write_buffer = (uint8_t*) malloc(len + 1);
    write_buffer[0] = reg;
    memcpy(write_buffer + 1, buf, len);

    // printf("Writing to 0x%02x with data: ", reg);
    // for (int i = 0; i < len+1; i++)
    // {
    //     printf("0x%02x ", ((uint8_t*)write_buffer)[i]);
    // }
    // printf("\n");
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_primary_handle, write_buffer, len+1, 1000));
    // ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_primary_handle, (uint8_t*)buf, len, 1000));
    // cfree(write_buffer);
    return true;
}

bool my_stusb4500_read(uint16_t addr, uint8_t reg, void* buf, size_t len, void* context)
{
    uint8_t reg_buffer = reg;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_primary_handle, &reg_buffer, 1, (uint8_t*)buf, len, 1000));
    // printf("Reading %d bytes from 0x%02x with data %x\n",len, reg, *(uint8_t*)buf);
    return true;
}

uint32_t get_milllis()
{
    printf("getting millis\n");
    return esp_timer_get_time()/1000;
}

void STUSB4500_init()
{
    // Initialize the STUSB4500
    i2c_master_bus_config_t i2c_primary_config = 
    {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_2,
        .scl_io_num = GPIO_NUM_1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0, // Set to zero to prevent async transactions
        .flags = {.enable_internal_pullup = true},
    };

    i2c_master_bus_handle_t i2c_primary_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_primary_config, &i2c_primary_bus));

    i2c_device_config_t i2c_primary_device_config = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x28,
        .scl_speed_hz = 100000,
        .scl_wait_us = 10000,
        .flags = {.disable_ack_check = false},
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_primary_bus, &i2c_primary_device_config, &i2c_dev_primary_handle));

    gpio_config_t io_conf =
    {
        .pin_bit_mask = (1ULL << GPIO_NUM_5 ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_config_t io_conf2 =
    {
        .pin_bit_mask = (1ULL << ST_USB_RST ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf2));

    // gpio_set_level(ST_USB_RST, 1);
    // ets_delay_us(10000);
    // gpio_set_level(ST_USB_RST, 0);
    // ets_delay_us(2000000);
    // return;
    // brute_force_write();
    brute_force_read();
    return;



    uint8_t buf2[16];
    
    uint8_t pdo_2_buf[] = {0xC8, 0xD0, 0x42, 0x00};
    my_stusb4500_write(0x28, 0x89, pdo_2_buf, 4, NULL);
    ets_delay_us(1000);

    buf2[0] = 0x02;
    my_stusb4500_write(0x28, 0x70, buf2, 1, NULL);
    ets_delay_us(1000);

    buf2[0] = 0x0D;
    my_stusb4500_write(0x28, 0x51, buf2, 1, NULL);
    ets_delay_us(1000);

    buf2[0] = 0x26;
    my_stusb4500_write(0x28, 0x1A, buf2, 1, NULL);
    ets_delay_us(1000);
    
    my_stusb4500_read(0x28, 0x70, buf2, 1, NULL);
    printf("PDO Channel: 0x%02x\n", buf2[0]);
    
    my_stusb4500_read(0x28, 0x2f, buf2, 1, NULL);
    printf("Device ID: 0x%02x\n", buf2[0]);


    // PORT_STATUS_1 (@0x0E)
    my_stusb4500_read(0x28, 0x0E, buf2, 1, NULL);
    printf("PORT_STATUS_1: 0x%02x\n", buf2[0]);

    // CC_STATUS (@0x11)
    my_stusb4500_read(0x28, 0x11, buf2, 1, NULL);
    printf("CC_STATUS: 0x%02x\n", buf2[0]);


    //  RDO_REG_STATUS_0 (@0x91 - 0x94)
    my_stusb4500_read(0x28, 0x91, buf2, 4, NULL);
    printf("RDO_REG_STATUS_0: 0x%02x %02x %02x %02x\n", buf2[3], buf2[2], buf2[1], buf2[0]);


    // ALERT_STATUS_1 (@0x0B)
    my_stusb4500_read(0x28, 0x0B, buf2, 1, NULL);
    printf("ALERT_STATUS_1: 0x%02x\n", buf2[0]);

    // PORT_STATUS_0 register (0x0D)
    my_stusb4500_read(0x28, 0x0D, buf2, 1, NULL);
    printf("    PORT_STATUS_0: 0x%02x\n", buf2[0]);


    // TYPEC_MONITORING_STATUS_0 (@0x0F)
    my_stusb4500_read(0x28, 0x0F, buf2, 1, NULL);
    printf("    TYPEC_MONITORING_STATUS_0: 0x%02x\n", buf2[0]);

    // CC_HW_FAULT_STATUS_0 register (0x12)
    my_stusb4500_read(0x28, 0x12, buf2, 1, NULL);
    printf("    CC_HW_FAULT_STATUS_0: 0x%02x\n", buf2[0]);

    // PRT_STATUS register (0x16)
    my_stusb4500_read(0x28, 0x16, buf2, 1, NULL);
    printf("    PRT_STATUS: 0x%02x\n", buf2[0]);

    // TYPEC_MONITORING_STATUS_1 (@0x10)
    my_stusb4500_read(0x28, 0x10, buf2, 1, NULL);
    printf("TYPEC_MONITORING_STATUS_1: 0x%02x\n", buf2[0]);

    // PE_FSM register (@0x29)
    my_stusb4500_read(0x28, 0x29, buf2, 1, NULL);
    printf("PE_FSM: 0x%02x\n", buf2[0]);
    
    // my_stusb4500_read(0x28, 0x8D, buf2, 4, NULL);
    // printf("PDO1: 0x%02x %02x %02x %02x\n", buf2[3], buf2[2], buf2[1], buf2[0]);

    my_stusb4500_read(0x28, 0x89, buf2, 4, NULL);
    printf("PDO2 DATA: 0x%02x %02x %02x %02x\n", buf2[3], buf2[2], buf2[1], buf2[0]);


    buf2[0] = 0x02;
    my_stusb4500_read(0x28, 0x70, buf2, 1, NULL);
    ets_delay_us(1000);

    

    my_stusb4500_read(0x28, 0x89, buf2, 4, NULL);
    printf("PDO2 DATA: 0x%02x %02x %02x %02x\n", buf2[3], buf2[2], buf2[1], buf2[0]);

    return;
    

    stusb4500_t my_stusb4500 = {
        .addr = 0x28,
        .write = my_stusb4500_write,
        .read = my_stusb4500_read,
        .context = NULL
    };

    stusb4500_config_t my_stusb4500_config = {
        .min_current_ma = 500,
        .min_voltage_mv = 5000,
        .max_voltage_mv = 9000,
        .get_ms = get_milllis,
    };
    

    // bool res = stusb4500_negotiate(&my_stusb4500, &my_stusb4500_config, false);
    // printf("Negotiation result: %d\n", res);
    // printf("updated pdo\n");
    // uint8_t buf[64];
    // stusb4500_nvm_read(&my_stusb4500, buf);
    // for (int i = 0; i < 64; i++)
    // {
    //     printf("%02x ", buf[i]);
    //     if (i % 8 == 7)
    //     {
    //         printf("\n");
    //     }
    // }
}


//     uint8_t reg = 0x2F;
//     uint8_t data = 0x00;



//     ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_primary_handle, &reg, 1, &data, 1, 1000));

//     ESP_LOGI("STUSB4500", "Device ID: 0x%02x", data);
// }


static void brute_force_write()
{
// i2c_master_transmit(i2c_dev_primary_handle, (uint8_t[]){0x00}, 1, 1000);
    uint8_t buf[16];
    buf[0] = 0x47;
    my_stusb4500_write(0x28, 0x95, buf,1,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x53, buf,1,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x40;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    buf[0] = 0xFA;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x07;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 5ms\n");
    ets_delay_us(5000);

    buf[0] = 0x05;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);


    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 5ms\n");
    ets_delay_us(5000);

    uint8_t nvm_sector_0[] = {0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00};
    my_stusb4500_write(0x28, 0x96, nvm_sector_0, 8, NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x01;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x06;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 2ms\n");
    ets_delay_us(2000);

    uint8_t nvm_sector_1[] = {0x10, 0x40, 0x9C, 0x1C, 0xFF, 0x01, 0x3C, 0xDF};
    my_stusb4500_write(0x28, 0x96, nvm_sector_1, 8, NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x01;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x06;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x51;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 2ms\n");
    ets_delay_us(2000);

    uint8_t nvm_sector_2[] = {0x02, 0x40, 0x0F, 0x00, 0x32,0x00, 0xFC, 0xF1};
    my_stusb4500_write(0x28, 0x96, nvm_sector_2, 8, NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x01;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x06;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x52;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 2ms\n");
    ets_delay_us(2000);

    uint8_t nvm_sector_3[] = {0x00,0x19, 0x56, 0xAF, 0xF5, 0x35, 0x5F, 0x00};
    my_stusb4500_write(0x28, 0x96, nvm_sector_3, 8, NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x01;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x06;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x53;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 2ms\n");
    ets_delay_us(2000);

    uint8_t nvm_sector_4[] = {0x00, 0x4B, 0x90, 0x21, 0x43, 0x00, 0x40,0xFB};
    my_stusb4500_write(0x28, 0x96, nvm_sector_4, 8, NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x01;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x06;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x54;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 2ms\n");
    ets_delay_us(2000);

    buf[0] = 0x040;
    buf[1] = 0x00;
    my_stusb4500_write(0x28, 0x96, buf,2,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x95, buf,1,NULL);


}


static void brute_force_read()
{
    uint8_t buf[16];

    buf[0] = 0x47;
    my_stusb4500_write(0x28, 0x95, buf,1,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    buf[0] = 0x40;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x97, buf,1,NULL);

    buf[0] = 0x50;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    my_stusb4500_read(0x28, 0x53, buf, 8, NULL);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }

    buf[0] = 0x51;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    my_stusb4500_read(0x28, 0x53, buf, 8, NULL);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }

    buf[0] = 0x52;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    my_stusb4500_read(0x28, 0x53, buf, 8, NULL);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }

    buf[0] = 0x53;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    my_stusb4500_read(0x28, 0x53, buf, 8, NULL);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }

    buf[0] = 0x54;
    my_stusb4500_write(0x28, 0x96, buf,1,NULL);

    printf("delaying 1ms\n");
    ets_delay_us(1000);

    my_stusb4500_read(0x28, 0x53, buf, 8, NULL);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }

    buf[0] = 0x040;
    buf[1] = 0x00;
    my_stusb4500_write(0x28, 0x96, buf,2,NULL);

    buf[0] = 0x00;
    my_stusb4500_write(0x28, 0x95, buf,1,NULL);







}