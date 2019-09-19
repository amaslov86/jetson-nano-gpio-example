//
// Simple Ultrasonic example for NVIDIA Jetson Nano by YoungJin Suh (http://valentis.pe.kr / valentis@chollian.net)
//
//

#include <stdio.h>			
#include <stdlib.h>			// for exit()
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "gpionano.h"

static int64_t getMicrotime()
{
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);

  return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

int main(int argc, char** argv)
{
    //  read physical memory (needs root)
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "usage : $ sudo %s (with root privilege)\n", argv[0]);
        exit(1);
    }

    //  map a particular physical address into our address space
    int pagesize = getpagesize();
    int pagemask = pagesize-1;

    //  This page will actually contain all the GPIO controllers, because they are co-located
    void *base = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ((GPIO_16 | GPIO_77) & ~pagemask));
    if (base == NULL) {
        perror("mmap()");
        exit(1);
    }

    //  set up a pointer for convenient access -- this pointer is to the selected GPIO controller
    gpio_t volatile *pinTrigger = (gpio_t volatile *)((char *)base + (GPIO_16 & pagemask));
    gpio_t volatile *pinEcho = (gpio_t volatile *)((char *)base + (GPIO_77 & pagemask));

    // for Trigger : GPIO OUT 
    pinTrigger->CNF = 0x00FF;
    pinTrigger->OE = OUTPUT;
//  pinTrigger->OUT = 0xFF;
    
    // for Echo : GPIO IN 
    pinEcho->CNF = 0x00FF;
    pinEcho->OE = INPUT;
    pinEcho->IN = 0x00; 		// initial value
    
    //  disable interrupts
    pinTrigger->INT_ENB = 0x00;
    pinEcho->INT_ENB = 0x00;

    // parameter for Input
    pinEcho->INT_STA = 0xFF;		// for Active_low
    pinEcho->INT_LVL = GPIO_INT_LVL_EDGE_BOTH;
    pinEcho->INT_CLR = 0xffffff;

    printf("checkout : \"$ sudo cat /sys/kernel/debug/tegra_gpio\"\n");

    int8_t echo, prevEcho, lowHigh, highLow;
    int64_t startTime = 0, stopTime = 0, interval;
    float distanceCm;

    for(uint8_t cnt = 0; cnt < 10; cnt++) {
        // Trigger ultrasonic sensor 
        pinTrigger->OUT = 0xFF;		// HIGH
        usleep(10);
        pinTrigger->OUT = 0x00;		// LOW

        lowHigh = highLow = echo = prevEcho = 0;

	// receive echo & calculate distance
        while(0 == lowHigh || 0 == highLow) {
            prevEcho = echo;
            echo = pinEcho->IN>>5;
            if(0 == lowHigh && 0 == prevEcho && 1 == echo) {
                lowHigh = 1;
                startTime = getMicrotime();
            }
            if(1 == lowHigh && 1 == prevEcho && 0 == echo) {
                highLow = 1;
                stopTime = getMicrotime();
            }
        }

        interval = stopTime - startTime;
        distanceCm = interval / 58;
        printf("distance: %.2f cm\n", distanceCm);
	usleep(1000*1000);		// wait 1 sec
    }

    /* unmap */
    munmap(base, pagesize);

    /* close the /dev/mem */
    close(fd);

    printf("\nGood Bye!!!\n");
    
    return 0;
}
