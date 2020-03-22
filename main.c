
//-----------------------------------------------------------------------------
// Includes

#include <stdio.h>
#include "driverlib.h"
#include "DSPLib.h"

//-----------------------------------------------------------------------------
// Defines

#define TIMER_PERIOD_TICKS (1600) // 1600 ticks for a 16MHz DCO clock should give us a 100us tick
#define AES_ENCRYPTION_DATA_SIZE (16) // Size of data to be encrypted/decrypted (must be multiple of 16)
#define FFT_SAMPLES (16) // (512)
#define DMT_SIZE (512) // Size to transfer in bytes

//-----------------------------------------------------------------------------
// Globals

uint32_t systemTicks100Microseconds; // System uptime tick, 100us per tick
Calendar calendar; // Calendar used for RTC
// These are kept in global scope so the compiler doesn't optimize them
uint64_t startTimeSysTicks;
uint64_t endTimeSysTicks;
uint64_t totalAesTicks;
uint64_t totalFftTicks;
uint64_t totalDmaTicks;
uint8_t dataAESencrypted[AES_ENCRYPTION_DATA_SIZE]; // Encrypted data
uint8_t dataAESdecrypted[AES_ENCRYPTION_DATA_SIZE]; // Decrypted data
char message[AES_ENCRYPTION_DATA_SIZE] = {0};

// AES stuff
#pragma PERSISTENT(cipherKey)
uint8_t cipherKey[32] =
{
    0xDE, 0xAD, 0xBE, 0xEF,
    0xBA, 0xDC, 0x0F, 0xEE,
    0xFE, 0xED, 0xBE, 0xEF,
    0xBE, 0xEF, 0xBA, 0xBE,
    0xBA, 0xDF, 0x00, 0x0D,
    0xFE, 0xED, 0xC0, 0xDE,
    0xD0, 0xD0, 0xCA, 0xCA,
    0xCA, 0xFE, 0xBA, 0xBE,
};

// FFT stuff
DSPLIB_DATA(input, MSP_ALIGN_CMPLX_FFT_Q15(FFT_SAMPLES))
_q15 input[FFT_SAMPLES*2];
/* Input signal parameters */
#pragma PERSISTENT(inputVector)
int32_t inputVector[FFT_SAMPLES] = {
0x08f11159, 0x121ce61b, 0x173ff715, 0x163be513, 0x1f3307ca, 0xf5eb162e, 0x07be107c, 0xf852ea79,
0x18b0001f, 0x09fe0215, 0xfa7a08c6, 0x10e3f37a, 0x095603dd, 0x1c98fe4c, 0xf5e8f08d, 0x01e6f133,};
/**
0x174cfeda, 0xf5ee1403, 0xf916ee8d, 0xf6570c28, 0xe2581849, 0xe8c6ec42, 0x16baea16, 0xea35e118,
0xe32def6b, 0xf832f434, 0xe7ad1da6, 0xe1850915, 0x0760f104, 0xe191fe1d, 0xef50fd49, 0xfc22e377,
0xf77b069c, 0xf529f210, 0xfe39f091, 0x08d9eb7f, 0x17c0e090, 0x0aa5f1a0, 0x12b7ee7b, 0x1de5f4db,
0xe90ef5e4, 0x138a1617, 0x1f2c10bb, 0x01bef50f, 0xf32ce68d, 0xe3d5e34f, 0x06c6e27b, 0xe9171ff6,
0x1206fe41, 0xeed11e3f, 0x09be103e, 0xf0cff7ab, 0xe94be17f, 0x0ff9fb75, 0xf04ffc02, 0xf1e5eddf,
0x03f6195d, 0xea19056f, 0xfa7fe323, 0x09aeebd7, 0x0f26edaa, 0x1025ed84, 0x0d7a15ec, 0x142cf93d,
0x177b1f81, 0xefc002fd, 0xfaa9e13b, 0x02b9e08f, 0x1c02e3f3, 0x1ff412b2, 0x00910c52, 0xe5b011d9,
0x17480487, 0xe7aa0fc7, 0xfb9f11c7, 0xff72f159, 0x1edc0ac5, 0xe0b20f96, 0x08d2ec58, 0x0bd8f4df,
0xf7dce04f, 0x0189fb99, 0x1c27f285, 0x16770442, 0x16f5f82b, 0x047cf66b, 0x08441786, 0x1c0eea2c,
0xf9f41f8c, 0xf15403b8, 0xf512f592, 0x0057f0c6, 0x005b13ee, 0x0046e109, 0x15e6092e, 0xe97e0c1f,
0x07b80dc2, 0x0047eb07, 0xef49e3e0, 0x1c0a16be, 0x0d29063e, 0x1dc4e087, 0x0ab2156d, 0xf4faf9d4,
0xfd8d04a6, 0xfa38e64e, 0x1713f29f, 0x068dfa90, 0x1b97176f, 0xe09e06d3, 0x12f2f17f, 0xff420a1b,
0xf5221aab, 0xfe8bff89, 0x1646e46b, 0xeaa91a95, 0xfb1ce370, 0xf8dd086d, 0x024105ce, 0x0a74edd7,
0x1425ffce, 0xf26c04ac, 0xff3ceb39, 0x02a7f8f9, 0xffcd1ad3, 0x0c52e346, 0xed6112bf, 0xed6b0b93,
0x08f11159, 0x121ce61b, 0x173ff715, 0x163be513, 0x1f3307ca, 0xf5eb162e, 0x07be107c, 0xf852ea79,
0x18b0001f, 0x09fe0215, 0xfa7a08c6, 0x10e3f37a, 0x095603dd, 0x1c98fe4c, 0xf5e8f08d, 0x01e6f133,
0x174cfeda, 0xf5ee1403, 0xf916ee8d, 0xf6570c28, 0xe2581849, 0xe8c6ec42, 0x16baea16, 0xea35e118,
0xe32def6b, 0xf832f434, 0xe7ad1da6, 0xe1850915, 0x0760f104, 0xe191fe1d, 0xef50fd49, 0xfc22e377,
0xf77b069c, 0xf529f210, 0xfe39f091, 0x08d9eb7f, 0x17c0e090, 0x0aa5f1a0, 0x12b7ee7b, 0x1de5f4db,
0xe90ef5e4, 0x138a1617, 0x1f2c10bb, 0x01bef50f, 0xf32ce68d, 0xe3d5e34f, 0x06c6e27b, 0xe9171ff6,
0x1206fe41, 0xeed11e3f, 0x09be103e, 0xf0cff7ab, 0xe94be17f, 0x0ff9fb75, 0xf04ffc02, 0xf1e5eddf,
0x03f6195d, 0xea19056f, 0xfa7fe323, 0x09aeebd7, 0x0f26edaa, 0x1025ed84, 0x0d7a15ec, 0x142cf93d,
0x177b1f81, 0xefc002fd, 0xfaa9e13b, 0x02b9e08f, 0x1c02e3f3, 0x1ff412b2, 0x00910c52, 0xe5b011d9,
0x17480487, 0xe7aa0fc7, 0xfb9f11c7, 0xff72f159, 0x1edc0ac5, 0xe0b20f96, 0x08d2ec58, 0x0bd8f4df,
0xf7dce04f, 0x0189fb99, 0x1c27f285, 0x16770442, 0x16f5f82b, 0x047cf66b, 0x08441786, 0x1c0eea2c,
0xf9f41f8c, 0xf15403b8, 0xf512f592, 0x0057f0c6, 0x005b13ee, 0x0046e109, 0x15e6092e, 0xe97e0c1f,
0x07b80dc2, 0x0047eb07, 0xef49e3e0, 0x1c0a16be, 0x0d29063e, 0x1dc4e087, 0x0ab2156d, 0xf4faf9d4,
0xfd8d04a6, 0xfa38e64e, 0x1713f29f, 0x068dfa90, 0x1b97176f, 0xe09e06d3, 0x12f2f17f, 0xff420a1b,
0xf5221aab, 0xfe8bff89, 0x1646e46b, 0xeaa91a95, 0xfb1ce370, 0xf8dd086d, 0x024105ce, 0x0a74edd7,
0x1425ffce, 0xf26c04ac, 0xff3ceb39, 0x02a7f8f9, 0xffcd1ad3, 0x0c52e346, 0xed6112bf, 0xed6b0b93,
0x08f11159, 0x121ce61b, 0x173ff715, 0x163be513, 0x1f3307ca, 0xf5eb162e, 0x07be107c, 0xf852ea79,
0x18b0001f, 0x09fe0215, 0xfa7a08c6, 0x10e3f37a, 0x095603dd, 0x1c98fe4c, 0xf5e8f08d, 0x01e6f133,
0x174cfeda, 0xf5ee1403, 0xf916ee8d, 0xf6570c28, 0xe2581849, 0xe8c6ec42, 0x16baea16, 0xea35e118,
0xe32def6b, 0xf832f434, 0xe7ad1da6, 0xe1850915, 0x0760f104, 0xe191fe1d, 0xef50fd49, 0xfc22e377,
0xf77b069c, 0xf529f210, 0xfe39f091, 0x08d9eb7f, 0x17c0e090, 0x0aa5f1a0, 0x12b7ee7b, 0x1de5f4db,
0xe90ef5e4, 0x138a1617, 0x1f2c10bb, 0x01bef50f, 0xf32ce68d, 0xe3d5e34f, 0x06c6e27b, 0xe9171ff6,
0x1206fe41, 0xeed11e3f, 0x09be103e, 0xf0cff7ab, 0xe94be17f, 0x0ff9fb75, 0xf04ffc02, 0xf1e5eddf,
0x03f6195d, 0xea19056f, 0xfa7fe323, 0x09aeebd7, 0x0f26edaa, 0x1025ed84, 0x0d7a15ec, 0x142cf93d,
0x177b1f81, 0xefc002fd, 0xfaa9e13b, 0x02b9e08f, 0x1c02e3f3, 0x1ff412b2, 0x00910c52, 0xe5b011d9,
0x17480487, 0xe7aa0fc7, 0xfb9f11c7, 0xff72f159, 0x1edc0ac5, 0xe0b20f96, 0x08d2ec58, 0x0bd8f4df,
0xf7dce04f, 0x0189fb99, 0x1c27f285, 0x16770442, 0x16f5f82b, 0x047cf66b, 0x08441786, 0x1c0eea2c,
0xf9f41f8c, 0xf15403b8, 0xf512f592, 0x0057f0c6, 0x005b13ee, 0x0046e109, 0x15e6092e, 0xe97e0c1f,
0x07b80dc2, 0x0047eb07, 0xef49e3e0, 0x1c0a16be, 0x0d29063e, 0x1dc4e087, 0x0ab2156d, 0xf4faf9d4,
0xfd8d04a6, 0xfa38e64e, 0x1713f29f, 0x068dfa90, 0x1b97176f, 0xe09e06d3, 0x12f2f17f, 0xff420a1b,
0xf5221aab, 0xfe8bff89, 0x1646e46b, 0xeaa91a95, 0xfb1ce370, 0xf8dd086d, 0x024105ce, 0x0a74edd7,
0x1425ffce, 0xf26c04ac, 0xff3ceb39, 0x02a7f8f9, 0xffcd1ad3, 0x0c52e346, 0xed6112bf, 0xed6b0b93,
0x08f11159, 0x121ce61b, 0x173ff715, 0x163be513, 0x1f3307ca, 0xf5eb162e, 0x07be107c, 0xf852ea79,
0x18b0001f, 0x09fe0215, 0xfa7a08c6, 0x10e3f37a, 0x095603dd, 0x1c98fe4c, 0xf5e8f08d, 0x01e6f133,
0x174cfeda, 0xf5ee1403, 0xf916ee8d, 0xf6570c28, 0xe2581849, 0xe8c6ec42, 0x16baea16, 0xea35e118,
0xe32def6b, 0xf832f434, 0xe7ad1da6, 0xe1850915, 0x0760f104, 0xe191fe1d, 0xef50fd49, 0xfc22e377,
0xf77b069c, 0xf529f210, 0xfe39f091, 0x08d9eb7f, 0x17c0e090, 0x0aa5f1a0, 0x12b7ee7b, 0x1de5f4db,
0xe90ef5e4, 0x138a1617, 0x1f2c10bb, 0x01bef50f, 0xf32ce68d, 0xe3d5e34f, 0x06c6e27b, 0xe9171ff6,
0x1206fe41, 0xeed11e3f, 0x09be103e, 0xf0cff7ab, 0xe94be17f, 0x0ff9fb75, 0xf04ffc02, 0xf1e5eddf,
0x03f6195d, 0xea19056f, 0xfa7fe323, 0x09aeebd7, 0x0f26edaa, 0x1025ed84, 0x0d7a15ec, 0x142cf93d,
0x177b1f81, 0xefc002fd, 0xfaa9e13b, 0x02b9e08f, 0x1c02e3f3, 0x1ff412b2, 0x00910c52, 0xe5b011d9,
0x17480487, 0xe7aa0fc7, 0xfb9f11c7, 0xff72f159, 0x1edc0ac5, 0xe0b20f96, 0x08d2ec58, 0x0bd8f4df,
0xf7dce04f, 0x0189fb99, 0x1c27f285, 0x16770442, 0x16f5f82b, 0x047cf66b, 0x08441786, 0x1c0eea2c,
0xf9f41f8c, 0xf15403b8, 0xf512f592, 0x0057f0c6, 0x005b13ee, 0x0046e109, 0x15e6092e, 0xe97e0c1f,
0x07b80dc2, 0x0047eb07, 0xef49e3e0, 0x1c0a16be, 0x0d29063e, 0x1dc4e087, 0x0ab2156d, 0xf4faf9d4,
0xfd8d04a6, 0xfa38e64e, 0x1713f29f, 0x068dfa90, 0x1b97176f, 0xe09e06d3, 0x12f2f17f, 0xff420a1b,
0xf5221aab, 0xfe8bff89, 0x1646e46b, 0xeaa91a95, 0xfb1ce370, 0xf8dd086d, 0x024105ce, 0x0a74edd7,
0x1425ffce, 0xf26c04ac, 0xff3ceb39, 0x02a7f8f9, 0xffcd1ad3, 0x0c52e346, 0xed6112bf, 0xed6b0b93,
};
**/
msp_cmplx_fft_q15_params fftParams;

// DMT stuff
#pragma PERSISTENT(dmtNvsramBuff)
uint8_t dmtNvsramBuff[DMT_SIZE];
uint8_t dmtSramBuff[DMT_SIZE];

//-----------------------------------------------------------------------------
// Function prototypes

void Init_GPIO(void);
void Init_Clock(void);
void Init_UART(void);
void Init_RTC(void);
void Init_Timer(void);
void Init_AES(uint8_t * cypherKey);
void enterLPM35();

//-----------------------------------------------------------------------------
// Functions

int _system_pre_init(void)
{
    // Stop Watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);     // Stop WDT

    // Disable global interrupts
    __disable_interrupt();

    // Initialize our system ticks
    systemTicks100Microseconds = 0;

    // Choose if segment (BSS) initialization should be performed or not.
    // Return: 0 to omit initialization 1 to run initialization
    return 1;
}

#define AES_BENCHMARKS 1
#define FFT_BENCHMARKS 1
#define DMA_BENCHMARKS 1

void main(void)
{
    uint16_t i;

    // Zero out ticks
    startTimeSysTicks = 0;
    endTimeSysTicks = 0;
    totalAesTicks = 0;
    totalFftTicks = 0;
    totalDmaTicks = 0;

    // Peripheral initialization
    Init_GPIO();
    Init_Clock();
    Init_UART();
    Init_Timer();
    Init_AES(cipherKey);

    // Enable global interrupts
    __enable_interrupt();

#if AES_BENCHMARKS
    // Copy the string we want to encrypt to our buffer
    const char stringToEncrypt[] = "I am a meat popsicle.           ";
    memcpy(message, stringToEncrypt, sizeof(stringToEncrypt));
    // Get start time
    startTimeSysTicks = systemTicks100Microseconds;
    // Do stuff
    for (i = 0; i < AES_ENCRYPTION_DATA_SIZE; i += 16)
    {
        // Encrypt data with preloaded cipher key
         AES256_encryptData(AES256_BASE, (uint8_t*)(message) + i, dataAESencrypted + i);
    }
    // Get end time
    endTimeSysTicks = systemTicks100Microseconds;
    // Figure out how long it took to do our thing
    totalAesTicks = endTimeSysTicks - startTimeSysTicks;
    // printf("Total time for AES: %llu us\n", totalAesTicks*100);
#endif // AES_BENCHMARKS

#if FFT_BENCHMARKS
    // Copy input vector to peripheral mem
    memcpy(input, inputVector, sizeof(inputVector));
    /* Initialize the FFT parameter structure. */
    fftParams.length = FFT_SAMPLES;
    fftParams.bitReverse = 1;
    fftParams.twiddleTable = msp_cmplx_twiddle_table_512_q15;
    // Get start time
    startTimeSysTicks = systemTicks100Microseconds;
    // Do stuff
    msp_cmplx_fft_fixed_q15(&fftParams, input);
    // Get end time
    endTimeSysTicks = systemTicks100Microseconds;
    // Figure out how long it took to do our thing
    totalFftTicks = endTimeSysTicks - startTimeSysTicks;
    // printf("Total time for FFT: %llu us\n", totalFftTicks*100);
#endif // FFT_BENCHMARKS

#if FFT_BENCHMARKS
    DMA_initParam param = {0};
    param.channelSelect = DMA_CHANNEL_0;
    param.transferModeSelect = DMA_TRANSFER_REPEATED_BLOCK;
    param.transferSize = DMT_SIZE;
    param.triggerSourceSelect = DMA_TRIGGERSOURCE_0;
    param.transferUnitSelect = DMA_SIZE_SRCWORD_DSTWORD;
    param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
    DMA_init(&param);

    // Configure DMA channel 0
    // Use dmtNvsramBuff as source
    // Increment source address after every transfer
    DMA_setSrcAddress(DMA_CHANNEL_0, (uint32_t)&dmtNvsramBuff, DMA_DIRECTION_INCREMENT);
    
    // Configure DMA channel 0
    // Use dmtSramBuff as destination
    // Increment destination address after every transfer
    DMA_setDstAddress(DMA_CHANNEL_0, (uint32_t)&dmtSramBuff, DMA_DIRECTION_INCREMENT);
    //Enable transfers on DMA channel 0
    DMA_enableTransfers(DMA_CHANNEL_0);

    // Get start time
    startTimeSysTicks = systemTicks100Microseconds;
    // Do stuff
    //Start block tranfer on DMA channel 0
    DMA_startTransfer(DMA_CHANNEL_0);
    // Get end time
    endTimeSysTicks = systemTicks100Microseconds;
    // Figure out how long it took to do our thing
    totalDmaTicks = endTimeSysTicks - startTimeSysTicks;
    // printf("Total time for DMA: %llu us\n", totalDmaTicks*100);
#endif // FFT_BENCHMARKS

    // Main loop
    for (;;)
    {
        __no_operation();
    }
}

/*
 * GPIO Initialization
 */
void Init_GPIO()
{
    // Set all GPIO pins to output low for low power
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7|GPIO_PIN8|GPIO_PIN9|GPIO_PIN10|GPIO_PIN11|GPIO_PIN12|GPIO_PIN13|GPIO_PIN14|GPIO_PIN15);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0); // SD CS

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7|GPIO_PIN8|GPIO_PIN9|GPIO_PIN10|GPIO_PIN11|GPIO_PIN12|GPIO_PIN13|GPIO_PIN14|GPIO_PIN15);

    // Configure P2.0 - UCA0TXD and P2.1 - UCA0RXD
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2, GPIO_PIN1, GPIO_SECONDARY_MODULE_FUNCTION);

    // Set PJ.4 and PJ.5 as Primary Module Function Input, LFXT.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_PJ, GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);

    // P1.0 and P1.1 are the LEDs
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1);

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}

/*
 * Clock System Initialization
 */
void Init_Clock()
{
    // Set DCO frequency to 16 MHz
    CS_setDCOFreq(CS_DCORSEL_1, CS_DCOFSEL_4);
    //Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768, 0);
    //Set ACLK=LFXT
    CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    //Start XT1 with no time out
    CS_turnOnLFXT(CS_LFXT_DRIVE_3);
}

/*
 * UART Communication Initialization
 */
void Init_UART()
{
    // Configure UART
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 52;
    param.firstModReg = 1;
    param.secondModReg = 0x49;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);
    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    // Enable USCI_A0 RX interrupt
    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
}

/*
 * Real Time Clock Initialization
 */
void Init_RTC()
{
    //Setup Current Time for Calendar
    calendar.Seconds    = 0x55;
    calendar.Minutes    = 0x30;
    calendar.Hours      = 0x04;
    calendar.DayOfWeek  = 0x01;
    calendar.DayOfMonth = 0x30;
    calendar.Month      = 0x04;
    calendar.Year       = 0x2014;

    // Initialize RTC with the specified Calendar above
    RTC_C_initCalendar(RTC_C_BASE, &calendar, RTC_C_FORMAT_BCD);
    RTC_C_setCalendarEvent(RTC_C_BASE, RTC_C_CALENDAREVENT_MINUTECHANGE);
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);

    //Start RTC Clock
    RTC_C_startClock(RTC_C_BASE);
}

/*
 * Setup 100us tick timer
 */
void Init_Timer(void)
{
    // Start timer
    Timer_A_initUpModeParam param = {0};
    param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // Use SMCLK (=DCO @ 16 MHz)
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod = TIMER_PERIOD_TICKS;
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &param);

    __delay_cycles(10000); // Delay wait for clock to settle
}

/**
 * @brief      Setup the AES peripheral
 *
 * @param      cypherKey  The 32 byte cypher key to use for AES encryption/decrytion
 */
void Init_AES(uint8_t * cypherKey)
{
    // Load a cipher key to module
    AES256_setCipherKey(AES256_BASE, cypherKey, AES256_KEYLENGTH_256BIT);
}

//-----------------------------------------------------------------------------
// ISRs

/*
 * Timer0_A3 Interrupt Vector (TAIV) handler
 *
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    uint16_t currentTimerValue;

    // Add our desired period ticks
    currentTimerValue = Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0) + TIMER_PERIOD_TICKS;
    // Update compare value
    Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0, currentTimerValue);
    // Toggle LED
    GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1);
    // Update our system tick
    systemTicks100Microseconds++;

    __no_operation();
}

/*
 * RTC_C Interrupt Vector handler
 *
 */
#pragma vector = RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    switch(__even_in_range(RTCIV, 16))
    {
        case RTCIV_NONE:
            break;
        case RTCIV_RTCOFIFG:
            break;
        case RTCIV_RTCRDYIFG:
            break;
        case RTCIV_RTCTEVIFG:
            break;
        case RTCIV_RTCAIFG:
            break;
        case RTCIV_RT0PSIFG:
            break;
        case RTCIV_RT1PSIFG:
            break;
        default:
            break;
    }
}
