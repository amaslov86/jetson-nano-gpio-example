//
// Simple GPIO memory-mapped example by YoungJin Suh (http://valentis.pe.kr / valentis@chollian.net)
//                           originally from Snarky (github.com/jwatte)
// build with:
//  g++ -O1 -g -o mem gpiomem.cpp -Wall -std=gnu++17
// run with:
//  sudo ./mem
//

#include <stdio.h>
#include <stdlib.h>                     // for exit()
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include "nanogpio.h"

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
    GPIO_mem volatile *pinLed = (GPIO_mem volatile *)((char *)base + (GPIO_16 & pagemask));
    GPIO_mem volatile *pinSwt = (GPIO_mem volatile *)((char *)base + (GPIO_77 & pagemask));

    // for LED : GPIO OUT 
    pinLed->CNF[0] = 0x00FF;
    pinLed->OE[0] = OUTPUT;
//  pinLed->OUT[0] = 0xFF;
//  pinLed->IN = 0x00; read only
    
    // for Switch : GPIO IN 
    pinSwt->CNF[0] = 0x00FF;
    pinSwt->OE[0] = INPUT;
    pinSwt->IN[0] = 0x00; 		// initial value
    
    //  disable interrupts
    pinLed->INT_ENB[0] = 0x00;
    pinSwt->INT_ENB[0] = 0x00;

    // parameter for Input
    pinSwt->INT_STA[0] = 0xFF;		// for Active_low
    pinSwt->INT_LVL[0] = GPIO_INT_LVL_EDGE_BOTH;
    pinSwt->INT_CLR[0] = 0xffffff;

    // turn led light with switch 
    printf("checkout : \"$ sudo cat /sys/kernel/debug/tegra_gpio\"\n");
    for(uint8_t cnt = 0; cnt < 100; cnt++) {
        //printf("[%d] %x\n", cnt, pinSwt->IN[0]>>5);		// for debug message
        printf((pinSwt->IN[0]>>5)?"x":"o");
        pinLed->OUT[0] = (pinSwt->IN[0]>>5)?0:0xff;
	fflush(stdout);
        usleep(50*1000);
    }

    /* turn off the LED */
    pinLed->OUT[0] = 0;

    /* unmap */
    munmap(base, pagesize);

    /* close the /dev/mem */
    close(fd);

    printf("\nGood Bye!!!\n");
    
    return 0;
}
