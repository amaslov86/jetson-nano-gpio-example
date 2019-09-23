//
// Simple GPIO memory-mapped example by YoungJin Suh (http://valentis.pe.kr / valentis@chollian.net)
//                           originally from Snarky (github.com/jwatte)
// build with:
//  g++ -O1 -g -o led led.cpp -Wall -std=gnu++17
// run with:
//  sudo ./led 
//

#include <stdio.h>
#include <stdlib.h>                     // for exit()
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include "gpionano.h"

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
    void *base = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (GPIO_38 & ~pagemask));
    if (base == NULL) {
        perror("mmap()");
        exit(1);
    }

    //  set up a pointer for convenient access -- this pointer is to the selected GPIO controller
    gpio_t volatile *pinLed = (gpio_t volatile *)((char *)base + (GPIO_38 & pagemask));

    // for LED : GPIO OUT 
    pinLed->CNF = 0x00FF;
    pinLed->OE = OUTPUT;
    pinLed->OUT = 0xFF;
    
    //  disable interrupts
    pinLed->INT_ENB = 0x00;

    // blank led light  
    for(uint8_t cnt = 0; cnt < 10; cnt++) {
        pinLed->OUT ^= 0xff;
        usleep(1000*1000);		// wait 1 sec
    }

    /* turn off the LED */
    pinLed->OUT = 0;

    /* unmap */
    munmap(base, pagesize);

    /* close the /dev/mem */
    close(fd);

    printf("\nGood Bye!!!\n");
    
    return 0;
}
