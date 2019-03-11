#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

float temperature;
float humidity;

uint8_t dhtport;

void initDht22(int port)
{
    dhtport = port;
    temperature = 0.0;
    humidity = 0.0;
}

float getDhtTemp()
{
    return temperature;
}

float getDhtHum()
{
    return humidity;
}

int8_t readSensorLevel(int timeout, int level)
{
    int8_t delay=0;

    while (gpio_get_level(dhtport) == level) {
        if(delay>timeout) {
            return -1;
        }

        delay++;
        ets_delay_us(1);
    }

    return delay;
}


uint8_t readSensorByte() {
    int8_t timeout;
    uint8_t i, val=0;

    for(i=0; i<8; i++) {
        val <<= 1;

        timeout = readSensorLevel( 56, 0 );
        if(timeout<0) {
            printf("Sensor timeout (%d)\n",i);
            return 0;
        }

        timeout = readSensorLevel( 56, 1 );
        if(timeout<0) {
            printf("Sensor timeout (%d)\n",i);
            return 0;
        }

        if(timeout > 40) {
            val |= 1;
        }
    }

    return val;
}

int readDht()
{
    int8_t i;
    uint8_t dhtData[5];

    //
	gpio_set_direction( dhtport, GPIO_MODE_OUTPUT );

	// Wake up sensor (3ms low, then 25us up)
	gpio_set_level( dhtport, 0 );
    ets_delay_us(3000);
    gpio_set_level( dhtport, 1 );

    // Sensor must respond, with 0 & 1
    gpio_set_direction(dhtport, GPIO_MODE_INPUT);

    i = readSensorLevel(85, 0);
    if(i<0) {
        printf("Sensor not responding (1,%d)\n",i);
        return -1;
    }

    i = readSensorLevel(85, 1);
    if(i<0) {
        printf("Sensor not responding (2,%d)\n",i);
        return -1;
    }

    printf("Sensor OK\n");
	gpio_set_level( 22, 0 );

    // Init ok
    for(i=0; i<5; i++) {
        dhtData[i]=readSensorByte();
        printf("%d: %x\n", i, dhtData[i]);
    }

	gpio_set_level( 22, 1 );

    //
    return(1);
}