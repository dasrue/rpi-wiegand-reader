/*
 * linked with -lpthread -lwiringPi -lrt
 */

#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>

#define PIN_0 15 // GPIO Pin 15 | Green cable | Data0
#define PIN_1 14 // GPIO Pin 14 | White cable | Data1
#define PIN_SOUND 25 // GPIO Pin 25 | Yellow cable | Sound

#define MAXWIEGANDBITS 64
#define READERTIMEOUT 3000000
#define LEN 256

static unsigned long long __wiegandDataInt;
static unsigned char __wiegandData[MAXWIEGANDBITS];
static unsigned long __wiegandBitCount;
static struct timespec __wiegandBitTime;

void getData0(int gpio, int level, uint32_t tick) {
    if(level == 0) {
        if (__wiegandBitCount / 8 < MAXWIEGANDBITS) {
            __wiegandData[__wiegandBitCount / 8] <<= 1;
            if((__wiegandBitCount / 8) < sizeof(__wiegandDataInt)) {
                __wiegandDataInt <<= 1;
            }
            __wiegandBitCount++;
        }
        clock_gettime(CLOCK_MONOTONIC, &__wiegandBitTime);
    }
}

void getData1(int gpio, int level, uint32_t tick) {
    if(level == 0) {
        if (__wiegandBitCount / 8 < MAXWIEGANDBITS) {
            __wiegandData[__wiegandBitCount / 8] <<= 1;
            __wiegandData[__wiegandBitCount / 8] |= 1;
            if((__wiegandBitCount / 8) < sizeof(__wiegandDataInt)) {
                __wiegandDataInt <<= 1;
                __wiegandDataInt |= 1;
            }
            __wiegandBitCount++;
        }
        clock_gettime(CLOCK_MONOTONIC, &__wiegandBitTime);
    }
}

int wiegandInit(int d0pin, int d1pin) {
    // Setup wiringPi
    gpioInitialise();
    gpioSetMode(d0pin, PI_INPUT);
    gpioSetMode(d1pin, PI_INPUT);
    gpioSetMode(PIN_SOUND, PI_OUTPUT);

    gpioSetAlertFunc(d0pin, getData0);
    gpioSetAlertFunc(d1pin, getData1);
}

void wiegandReset() {
    memset((void *)__wiegandData, 0, MAXWIEGANDBITS);
    __wiegandDataInt = 0;
    __wiegandBitCount = 0;
}

int wiegandGetPendingBitCount() {
    struct timespec now, delta;
    clock_gettime(CLOCK_MONOTONIC, &now);
    delta.tv_sec = now.tv_sec - __wiegandBitTime.tv_sec;
    delta.tv_nsec = now.tv_nsec - __wiegandBitTime.tv_nsec;

    if ((delta.tv_sec > 1) || (delta.tv_nsec > READERTIMEOUT))
        return __wiegandBitCount;

    return 0;
}

int wiegandReadData(void* data, int dataMaxLen) {
    if (wiegandGetPendingBitCount() > 0) {
        int bitCount = __wiegandBitCount;
        int byteCount = (__wiegandBitCount / 8) + 1;
        memcpy(data, (void *)__wiegandData, ((byteCount > dataMaxLen) ? dataMaxLen : byteCount));

        wiegandReset();
        return bitCount;
    }
    return 0;
}

int wiegandReadDataInt(unsigned long long * data) {
    if (wiegandGetPendingBitCount() > 0) {
        int bitCount = __wiegandBitCount;
        *data = __wiegandDataInt;

        wiegandReset();
        return bitCount;
    }
    return 0;
}

void printCharAsBinary(unsigned char ch) {
    int i;
    FILE * fp;
    fp = fopen("output","a");

    for (i = 0; i < 8; i++) {
        printf("%d", (ch & 0x80) ? 1 : 0);
        fprintf(fp, "%d", (ch & 0x80) ? 1 : 0);
        ch <<= 1;
    }

    fclose(fp);
}


void makeBeep(int millisecs, int times){
    int i;
    for (i = 0; i < times; i++) {
        gpioWrite (PIN_SOUND, 0);
        gpioDelay(millisecs*1000);
        gpioWrite (PIN_SOUND, 1);
        gpioDelay(millisecs*500);
    }
}



void main(void) {
    int i;

    wiegandInit(PIN_0, PIN_1);


    while(1) {
        int bitLen = wiegandGetPendingBitCount();
        if (bitLen == 0) {
            usleep(5000);
        } else {
            unsigned long long datai;
            char string1[100];
            bitLen = wiegandReadDataInt(&datai);
            int bytes = bitLen / 8 + 1;
            FILE *fp;
            fp = fopen("output","a");
            printf("%lu ", (unsigned long)time(NULL));
            fprintf(fp, "%lu ", (unsigned long)time(NULL));
            printf("Read %d bits (%d bytes): ", bitLen, bytes);
            fprintf(fp, "Read %d bits (%d bytes): ", bitLen, bytes);
            printf("%0*llX", bitLen / 4 + 1, datai);
            fprintf(fp, "%0*llX", bitLen / 4 + 1, datai);

            printf(" : ");
            fprintf(fp, " : ");
            fclose(fp);
            for (i = 0; i < bitLen; i++)
                  printf("%d", (datai >> (bitLen - i - 1)) & 1 ? 1 : 0);
            fp = fopen("output","a");
            printf("\n");
            fprintf(fp, "\n");
            fclose(fp);
            makeBeep(200, 1);
        }
    }
    gpioTerminate();
}
