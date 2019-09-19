//
// Simple GPIO memory-mapped example by YoungJin Suh (http://valentis.pe.kr / valentis@chollian.net)
//                           originally from Snarky (github.com/jwatte)
// build with:
//  g++ -O1 -g -o mem gpiomem.cpp -Wall -std=gnu++17
// run with:
//  sudo ./mem
//
#include <stdio.h>			
#include <stdlib.h>			// for exit()
#include <stdint.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

/* Tegra X1 SoC Technical Reference Manual, version 1.3
 *
 * See Chapter 9 "Multi-Purpose I/O Pins", section 9.13 "GPIO Registers"
 * (table 32: GPIO Register Address Map)
 *
 * The GPIO hardware shares PinMux with up to 4 Special Function I/O per
 * pin, and only one of those five functions (SFIO plus GPIO) can be routed to
 * a pin at a time, using the PixMux.
 *
 * In turn, the PinMux outputs signals to Pads using Pad Control Groups. Pad
 * control groups control things like "drive strength" and "slew rate," and
 * need to be reset after deep sleep. Also, different pads have different
 * voltage tolerance. Pads marked "CZ" can be configured to be 3.3V tolerant
 * and driving; and pads marked "DD" can be 3.3V tolerant when in open-drain
 * mode (only.)
 *
 * The CNF register selects GPIO or SFIO, so setting it to 1 forces the GPIO
 * function. This is convenient for those who have a different pinmux at boot.
 */

//  The only address we really need
#define GPIO_216      0x6000d60C    // Jetson Nano  7[AUDIO_MCLK]
#define GPIO_50       0x6000d108    // Jetson Nano 11[UART2_RTS]
#define GPIO_194      0x6000d600    // Jetson Nano 15[LCD_TE]
#define GPIO_16       0x6000d008    // Jetson Nano 19[SPI1_MOSI]
#define GPIO_38       0x6000d100    // Jetson Nano 33[GPIO_PE6]
#define GPIO_77       0x6000d204    // Jetson Nano 38[I2S4_SDIN] // J

// From https://github.com/leahneukirchen/linux-jetson-tk1/blob/master/drivers/gpio/gpio-tegra.c
#define GPIO_INT_LVL_MASK		0x010101
#define GPIO_INT_LVL_EDGE_RISING	0x000101
#define GPIO_INT_LVL_EDGE_FALLING	0x000100
#define GPIO_INT_LVL_EDGE_BOTH		0x010100
#define GPIO_INT_LVL_LEVEL_HIGH		0x000001
#define GPIO_INT_LVL_LEVEL_LOW		0x000000

enum INOUT { 
    INPUT, OUTPUT
};

//  layout based on the definitions above
//  Each GPIO controller has four ports, each port controls 8 pins, each
//  register is interleaved for the four ports, so
//  REGX: port0, port1, port2, port3
//  REGY: port0, port1, port2, port3
struct GPIO_mem {
    uint32_t CNF[4];
    uint32_t OE[4];
    uint32_t OUT[4];
    uint32_t IN[4];
    uint32_t INT_STA[4];
    uint32_t INT_ENB[4];
    uint32_t INT_LVL[4];
    uint32_t INT_CLR[4];
};

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
