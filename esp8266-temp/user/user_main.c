#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

int g_temperature = 0;
int g_thresholdTemperature = 30;

os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

static int gpioPin;

void ICACHE_FLASH_ATTR ds_init( int gpio )
{
  //set gpio2 as gpio pin
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  //disable pulldown
  PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO2_U);
  //enable pull up R
  PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);
  // Configure the GPIO with internal pull-up
  // PIN_PULLUP_EN( gpio );
  GPIO_DIS_OUTPUT( gpio );
  gpioPin = gpio;
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn’t then it is broken or shorted
// and we return;

void ICACHE_FLASH_ATTR ds_reset(void)
{
  //    IO_REG_TYPE mask = bitmask;
  //    volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
  uint8_t retries = 125;
  // noInterrupts();
  // DIRECT_MODE_INPUT(reg, mask);
  GPIO_DIS_OUTPUT( gpioPin );
  // interrupts();
  // wait until the wire is high… just in case
  do {
    //if (–retries == 0) return;
    os_delay_us(2);
  } while ( !GPIO_INPUT_GET( gpioPin ));
  // noInterrupts();
  GPIO_OUTPUT_SET( gpioPin, 0 );
  // DIRECT_WRITE_LOW(reg, mask);
  // DIRECT_MODE_OUTPUT(reg, mask);    // drive output low
  // interrupts();
  os_delay_us(480);
  // noInterrupts();
  GPIO_DIS_OUTPUT( gpioPin );
  // DIRECT_MODE_INPUT(reg, mask);    // allow it to float
  os_delay_us(70);
  // r = !DIRECT_READ(reg, mask);
  //r = !GPIO_INPUT_GET( gpioPin );
  // interrupts();
  os_delay_us(410);
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline void write_bit( int v )
{
  // IO_REG_TYPE mask=bitmask;
  //    volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
  GPIO_OUTPUT_SET( gpioPin, 0 );
  if( v ) {
    // noInterrupts();
    //    DIRECT_WRITE_LOW(reg, mask);
    //    DIRECT_MODE_OUTPUT(reg, mask);    // drive output low
    os_delay_us(10);
    GPIO_OUTPUT_SET( gpioPin, 1 );
    // DIRECT_WRITE_HIGH(reg, mask);    // drive output high
    // interrupts();
    os_delay_us(55);
  } else {
    // noInterrupts();
    //    DIRECT_WRITE_LOW(reg, mask);
    //    DIRECT_MODE_OUTPUT(reg, mask);    // drive output low
    os_delay_us(65);
    GPIO_OUTPUT_SET( gpioPin, 1 );
    //    DIRECT_WRITE_HIGH(reg, mask);    // drive output high
    //        interrupts();
    os_delay_us(5);
  }
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline int read_bit(void)
{
  //IO_REG_TYPE mask=bitmask;
  //volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
  int r;
  // noInterrupts();
  GPIO_OUTPUT_SET( gpioPin, 0 );
  // DIRECT_MODE_OUTPUT(reg, mask);
  // DIRECT_WRITE_LOW(reg, mask);
  os_delay_us(3);
  GPIO_DIS_OUTPUT( gpioPin );
  // DIRECT_MODE_INPUT(reg, mask);    // let pin float, pull up will raise
  os_delay_us(10);
  // r = DIRECT_READ(reg, mask);
  r = GPIO_INPUT_GET( gpioPin );
  // interrupts();
  os_delay_us(53);
  return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set ‘power’ to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void ICACHE_FLASH_ATTR  ds_write( uint8_t v, int power ) {
  uint8_t bitMask;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    write_bit( (bitMask & v)?1:0);
  }
  if ( !power) {
    // noInterrupts();
    GPIO_DIS_OUTPUT( gpioPin );
    GPIO_OUTPUT_SET( gpioPin, 0 );
    // DIRECT_MODE_INPUT(baseReg, bitmask);
    // DIRECT_WRITE_LOW(baseReg, bitmask);
    // interrupts();
  }
}

//
// Read a byte
//
uint8_t ICACHE_FLASH_ATTR ds_read() {
  uint8_t bitMask;
  uint8_t r = 0;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    if ( read_bit()) r |= bitMask;
  }
  return r;
}

void get_temp()
{
  ds_reset();
  ds_write(0xcc,1);
  ds_write(0xbe,1);

  g_temperature=(int)ds_read();
  os_printf("ds_read: %d \r\n", g_temperature);
  g_temperature=g_temperature+(int)ds_read()*256;
  g_temperature/=16;
  if (g_temperature>100) g_temperature-=4096;
  ds_reset();
  ds_write(0xcc,1);
  ds_write(0x44,1);
  os_printf("temperature: %d \r\n", g_temperature);
}

void some_timerfunc(void *arg)
{
  get_temp();
  if (g_temperature >= g_thresholdTemperature) {
    //Set GPIO5 to LOW
    os_printf("Set GPIO5 to LOW", g_temperature);
    gpio_output_set(0, BIT5, BIT5, 0);
    // Set GPIO4 as high-level output,GPIO5 as low-level output,
    gpio_output_set(BIT4, BIT5, BIT4|BIT5, 0);
  }
  else {
    //Set GPIO5 to HIGH
    os_printf("Set GPIO5 to HIGH", g_temperature);
    // Set GPIO4 as high-level output,GPIO5 as low-level output,
    gpio_output_set(BIT5, BIT4, BIT5|BIT4, 0);
  }
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
  os_delay_us(10);
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
  // Initialize UART0
  uart_div_modify(0, UART_CLK_FREQ / 115200);

  // Initialize the GPIO subsystem.
  gpio_init();

  ds_init(2);
  get_temp();

  //Disarm timer
  os_timer_disarm(&some_timer);

  //Set GPIO5 to output mode
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);

  //Set GPIO4 to output mode
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);

  //Set GPIO4 and GPIO5 to LOW
  //gpio_output_set(0, BIT5|BIT4, BIT5|BIT4, 0);

  //Setup timer
  os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

  //Arm the timer
  //&some_timer is the pointer
  //1000 is the fire time in ms (aka 1 second)
  //0 for once and 1 for repeating
  os_timer_arm(&some_timer, 1000, 1);

  //Start os task
  system_os_task(user_procTask, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
  system_os_post(user_procTaskPrio, 0, 0 );
}
