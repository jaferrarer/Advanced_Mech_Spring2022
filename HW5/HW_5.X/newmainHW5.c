#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<math.h> //sin function

#include <stdio.h> // library that contains sprintf
// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use internal fast frc oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // primary osc disabled
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt value
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz fast rc internal oscillator
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

void initSPI();
unsigned char spi_io(unsigned char o);
unsigned short create_value(unsigned char ab, unsigned char v);

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4= 0;  //makes a4 output (initializes)
    TRISBbits.TRISB4= 1;  //makes b4 input (initializes)
    LATAbits.LATA4=0;     // turns a4 off
    __builtin_enable_interrupts();
       
    float t=0;
    int up=0; 
    int tri=0;
    
    
    
    while(1) {
        unsigned char volta=(sin(2*3.14*2*t)+1)*128; // voltage for sine wave
        unsigned short vala=create_value(0,volta); // assigns volta values created below
        LATAbits.LATA0=0; // sets pin high
        unsigned short a1=vala>>8; // leftmost 8 bits from 16 bit number shifted right
        unsigned short a2=vala; // remainder of 16 bit value
        spi_io(a1);
        spi_io(a2);
        LATAbits.LATA0=1;//sets pin low
        
        if (up==0){
            tri=tri + (255/50);
            if (tri>255){
                up=1;
            }
        }
        if (up==1){
            tri=tri-(255/50);
                    if (tri<0){
                        up = 0;
            }
         }
        if (tri<0){
            tri=0;
        }
       
        unsigned short valb=create_value(1,tri); // assigns volta values created below
        LATAbits.LATA0=0; // sets pin high
        unsigned short b1=valb>>8; // leftmost 8 bits from 16 bit number shifted right
        unsigned short b2=valb; // remainder of 16 bit value
        spi_io(b1);
        spi_io(b2);
        LATAbits.LATA0=1;//sets pin low
        
        _CP0_SET_COUNT(0); // delay value
        while (_CP0_GET_COUNT()<48000000/2/100){} // delay 100 times per second
        t=t+0.01; // increment value for resolution of sine and triangle waves
    }
    
}

// initialize SPI1
void initSPI() {
    // Pin B14 has to be SCK1
    // Turn of analog pins
    ANSELA=0; // 1 for analog
    // Make an output pin for CS
    TRISAbits.TRISA0=0; // makes output pin
    LATAbits.LATA0=1; // turns it on
        // Set SDO1
    RPA1Rbits.RPA1R= 0b0011;
    // Set SDI1
    SDI1Rbits.SDI1R=0b0001;
    // setup SPI1
    SPI1CON = 0; // turn off the spi module and reset it
    SPI1BUF; // clear the rx buffer by reading from it
    SPI1BRG = 1000; // 1000 for 24kHz, 1 for 12MHz; // baud rate to 10 MHz [SPI1BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.CKE = 1; // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1; // master operation
    SPI1CONbits.ON = 1; // turn on spi 
}


// send a byte via spi and return the response
unsigned char spi_io(unsigned char o) {
  SPI1BUF = o;
  while(!SPI1STATbits.SPIRBF) { // wait to receive the byte
    ;
  }
  return SPI1BUF;
}

unsigned short create_value(unsigned char ab, unsigned char v){
    unsigned short s=0; // 16 bit number
    s=ab<<15; // ab to place 1
            s= s | (0b111<<12); // 16 bit number with required values shifted
            s= s | (v<<4); // 8 bit number
            return s;
}