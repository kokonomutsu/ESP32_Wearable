#ifndef _IO_CONFIG_H_
#define _IO_CONFIG_H_

#define USE_LED
    #define USE_LED_1
	#define USE_LED_2
    #define USE_LED_3

#define USE_BUTTON_IO
	#define NUMBER_IO_BUTTON_USE 2
	#define USE_BUTTON_IO_1
	#define USE_BUTTON_IO_2
	//#define USE_BUTTON_IO_3
	//#define USE_BUTTON_IO_4

/************************OUTPUT******************************/
#ifdef USE_LED
	#ifdef USE_LED_1
		#define LED1			2
        #define LED1_GPIO_INIT  pinMode(LED1, OUTPUT)
		#define LED1_GPIO_HIGH	digitalWrite(LED1, HIGH)
		#define LED1_GPIO_LOW	digitalWrite(LED1, LOW)
	#endif
	#ifdef USE_LED_2
		#define LED2			15
        #define LED2_GPIO_INIT  pinMode(LED2, OUTPUT)
		#define LED2_GPIO_HIGH	digitalWrite(LED2, HIGH)
		#define LED2_GPIO_LOW	digitalWrite(LED2, LOW)
	#endif
    #ifdef USE_LED_3
		#define LED3			4
        #define LED3_GPIO_INIT  pinMode(LED3, OUTPUT)
		#define LED3_GPIO_HIGH	digitalWrite(LED3, HIGH)
		#define LED3_GPIO_LOW	digitalWrite(LED3, LOW)
	#endif
    #ifdef USE_LED_4
		#define LED4			2
        #define LED4_GPIO_INIT  pinMode(LED4, OUTPUT)
		#define LED4_GPIO_HIGH	digitalWrite(LED4, HIGH)
		#define LED4_GPIO_LOW	digitalWrite(LED4, LOW)
	#endif
#endif

/************************INPUT******************************/
#ifdef USE_BUTTON_IO
	#ifdef USE_BUTTON_IO_1
		#define BT1				25
        #define BT1_GPIO_INIT   pinMode(BT1, INPUT)
		#define BT1_GPIO_VAL	digitalRead(BT1)
	#endif
	#ifdef USE_BUTTON_IO_2
		#define BT2				26
        #define BT2_GPIO_INIT   pinMode(BT2, INPUT)
		#define BT2_GPIO_VAL	digitalRead(BT2)
	#endif
	#ifdef USE_BUTTON_IO_3
		#define BT3				23
        #define BT3_GPIO_INIT   pinMode(BT3, INPUT)
		#define BT3_GPIO_VAL	digitalRead(BT3)
	#endif
#endif

#endif