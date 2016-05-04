#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv.h"

#define F_CPU 12000000L
#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_DATA_TEST 2
#define USB_DATA_OUT 3
#include <util/delay.h>

static uint8_t testReplyBuf[10] = {'F', 'U', 'C', 'K', ' ', 'Y', 'O', 'U', '!', '\0'};
static uint8_t replyBuf[32];

int samplebits();

// this gets called when custom control message is received
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data; // cast data to correct type
        
    switch(rq->bRequest) { // custom command is in the bRequest field
    case USB_LED_ON:
        PORTD ^= (1 << PD5); // turn LED on
        return 0;
    case USB_LED_OFF: 
        PORTD ^= (1 << PD5); // turn LED off
        return 0;
    case USB_DATA_TEST:
        usbMsgPtr = testReplyBuf;
        return sizeof(testReplyBuf);
    case USB_DATA_OUT:
        sampleBits();
        usbMsgPtr = replyBuf;
        return sizeof(replyBuf);
    }

    return 0; // should not get here
}

int sampleBits(){
    // Keep track of number of bits and bytes done
    uint8_t numBits = 0;
    uint8_t numBytes = 0;

    // Samples
    uint8_t sample0 = 0;
    uint8_t sample1 = 0;

    // Data
    uint8_t dataByte = 0;

    while(numBytes < 32){

        while(numBits < 8){
    
            sample0 = (ACSR & (1 << ACO));
            _delay_ms(50);
    
            sample1 = (ACSR & (1 << ACO));
    
            // Von Neumann debias the samples
            // 0->1 is a 1
            if(sample0 < sample1){
                dataByte = (dataByte << 1) | 1;
                numBits += 1;
            }

            // 1-> 0 is a 0
            else if(sample0 > sample1){
                dataByte = (dataByte << 1);
                numBits += 1;
            }

            _delay_ms(50);
        }

        // Store the sampled data in the reply buffer and reset
        replyBuf[numBytes] = dataByte;
        dataByte = 0;
        numBytes += 1;
    }

    return 0;
}

int main() {
    uchar i;
    DDRD |= (1 << PD5);; // PD5 as output
    PORTB = 0;
    for(i=0; i < 6; i++){
        PORTD ^= (1 << PD5);
        _delay_ms(500);
    }

    wdt_enable(WDTO_1S); // enable 1s watchdog timer

    usbInit();
        
    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();
        
    sei(); // Enable interrupts after re-enumeration
        
    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }
        
    return 0;
}
