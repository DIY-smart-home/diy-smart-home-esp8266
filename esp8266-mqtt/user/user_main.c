/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "os_type.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

#define settings_name "green"

MQTT_Client mqttClient;

int g_temperature = 0;
int g_thresholdTemperature = 30;
bool g_settingsUpdated = false;

os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

static int gpioPin;

char* itoa(int value, char* result, int base);
int atoi(const char *s);

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		INFO("MQTT: wifiConnectCb\r\n");
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	if (FALSE == MQTT_Subscribe(client, "/settings/temperature", 0))
	{
		INFO("MQTT: Unable to subscribe\r\n");
	}
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	if (0 == strcmp(topicBuf, "/settings/temperature"))
	{
		INFO("Settings have been received\r\n");
		g_thresholdTemperature = atoi(dataBuf);
		g_settingsUpdated = true;
		INFO("Threshold temperature: %d\r\n", g_thresholdTemperature);
	}

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

////////////////////////////////////////////////////////////////////////////////
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

int get_temp()
{
	ds_reset();
	ds_write(0xcc,1);
	ds_write(0xbe,1);

	int temperature=(int)ds_read();
	temperature=temperature+(int)ds_read()*256;
	temperature/=16;
	if (temperature>100) {
		temperature-=4096;
	}

	ds_reset();
	ds_write(0xcc,1);
	ds_write(0x44,1);
	return temperature;
}
////////////////////////////////////////////////////////////////////////////////

void some_timerfunc(void *arg)
{
	int currentTemperature = get_temp();
	os_printf("temperature: %d \r\n", currentTemperature);

	if ( (false == g_settingsUpdated) && (g_temperature == currentTemperature) ) {
		//no need to change anything
		return;
	}

	g_settingsUpdated = false;
	g_temperature = currentTemperature;

	char *tempStr = "000.0";
	itoa(g_temperature, tempStr, 10);

	char str[255];
	strcpy (str,"{ \"name\": \"");
	strcat (str, settings_name);
	strcat (str,"\", \"temperature\": ");
	strcat (str, tempStr);
	strcat (str,"} ");

	MQTT_Client* client = (MQTT_Client*)arg;
	MQTT_Publish(client, "/sensors/temperature", str, strlen(str), 0, 1);

	INFO("Temperature %d, Threshold temperature: %d\r\n", g_temperature, g_thresholdTemperature);
	if (g_temperature >= g_thresholdTemperature) {
		// Set GPIO4 as high-level output,GPIO5 as low-level output
		gpio_output_set(BIT4, BIT5, BIT4|BIT5, 0);
	}
	else {
		// Set GPIO4 as high-level output,GPIO5 as low-level output
		gpio_output_set(BIT5, BIT4, BIT5|BIT4, 0);
	}
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
	//os_delay_us(10);
}

char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = (char) 0; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = (char) 0;
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

int atoi (const char *s)
{
	return (int) strtol (s, NULL, 10);
}

void user_init(void)
{

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	INFO("\r\nTest ...\r\n");

	CFG_Load();

	//MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitConnection(&mqttClient, "iot.anavi.org", 1883, 0);
	INFO("\r\nInit ...\r\n");
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	//MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitClient(&mqttClient, "eps-leon", "", "", 120, 1);

	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	//WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
	WIFI_Connect("", "", wifiConnectCb);

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

	//// os_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg)
	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, &mqttClient);
	//// void os_timer_arm(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag)
	os_timer_arm(&some_timer, 1000, 1);
}
