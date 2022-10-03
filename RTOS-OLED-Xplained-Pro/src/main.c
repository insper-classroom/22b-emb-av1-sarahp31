#include <asf.h>
#include "conf_board.h"

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/* Botao da placa */

#define BUT_1_PIO PIOD
#define BUT_1_PIO_ID ID_PIOD
#define BUT_1_IDX 28
#define BUT_1_IDX_MASK (1u << BUT_1_IDX)

#define BUT_2_PIO PIOC
#define BUT_2_PIO_ID ID_PIOC
#define BUT_2_IDX 31
#define BUT_2_IDX_MASK (1u << BUT_2_IDX)

#define BUT_3_PIO PIOA
#define BUT_3_PIO_ID ID_PIOA
#define BUT_3_IDX 19
#define BUT_3_IDX_MASK (1u << BUT_3_IDX)

#define IN_1_PIO PIOD
#define IN_1_PIO_ID ID_PIOD
#define IN_1_IDX 30
#define IN_1_IDX_MASK (1u << IN_1_IDX)

#define IN_2_PIO PIOA
#define IN_2_PIO_ID ID_PIOA
#define IN_2_IDX 6
#define IN_2_IDX_MASK (1u << IN_2_IDX)

#define IN_3_PIO PIOC
#define IN_3_PIO_ID ID_PIOC
#define IN_3_IDX 19
#define IN_3_IDX_MASK (1u << IN_3_IDX)

#define IN_4_PIO PIOA
#define IN_4_PIO_ID ID_PIOA
#define IN_4_IDX 2
#define IN_4_IDX_MASK (1u << IN_4_IDX)

/** RTOS  */
#define TASK_OLED_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_OLED_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/** prototypes */
void but_callback(void);
void but1_callback(void);
void but2_callback(void);
void but3_callback(void);
static void BUT_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* global variables                                                     */
/************************************************************************/

QueueHandle_t xQueueMode;
QueueHandle_t xQueueSteps;
SemaphoreHandle_t xSemaphoreRTT;


/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void but_callback(void) {
}

void but1_callback(void) {
	int angulo = 180;
	xQueueSendFromISR(xQueueMode, &angulo, 0);
}

void but2_callback(void) {
	int angulo = 90;
	xQueueSendFromISR(xQueueMode, &angulo, 0);
}


void but3_callback(void) {
	int angulo = 45;
	xQueueSendFromISR(xQueueMode, &angulo, 0);
}

void RTT_Handler(void) {
	uint32_t ul_status;
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		xSemaphoreGiveFromISR(xSemaphoreRTT, 0);
	}
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_modo(void *pvParameters){
	
	gfx_mono_ssd1306_init();
	BUT_init();
	
	int angulo;
	int n_passos;
	for(;;){
		if( xQueueReceive(xQueueMode, &angulo, ( TickType_t ) 500 )){
			printf("angulo = %d \n", angulo);
			
			if(angulo == 45){
				gfx_mono_draw_string("angulo = 45 ", 0, 0, &sysfont);
			}
			if(angulo == 90){
				gfx_mono_draw_string("angulo = 90 ", 0, 0, &sysfont);
			}
			if(angulo == 180){
				gfx_mono_draw_string("angulo = 180", 0, 0, &sysfont);
			}
			
			//transformando o angulo para nuemro de passos
			n_passos = angulo / 0.17578125;
			
			xQueueSend(xQueueSteps, &n_passos, 0);	
		}
	}
}

static void task_motor(void *pvParameters){
	
	int passos;
	int sequencia = 0;
	for(;;){
		if( xQueueReceive(xQueueSteps, &passos, ( TickType_t ) 500 )){
			printf("passos = %d \n", passos);
			
			for(int i = 0; i< passos; i++){
				
				printf("i = %d \n", i);
				//tempo para mudar de fase 5ms
				RTT_init(1000, 5, RTT_MR_ALMIEN);
				
				if( xSemaphoreTake(xSemaphoreRTT, 500 / portTICK_PERIOD_MS) == pdTRUE ){
					sequencia += 1;
					
					printf("sequencia = %d \n", sequencia);
					if(sequencia > 4){
						sequencia = 1;
					}
				}
				
				
				if(sequencia == 1){
					pio_set(IN_1_PIO, IN_1_IDX_MASK);
					pio_clear(IN_2_PIO, IN_2_IDX_MASK);
					pio_clear(IN_3_PIO, IN_3_IDX_MASK);
					pio_clear(IN_4_PIO, IN_4_IDX_MASK);
				}
				
				if(sequencia == 2){
					pio_clear(IN_1_PIO, IN_1_IDX_MASK);
					pio_set(IN_2_PIO, IN_2_IDX_MASK);
					pio_clear(IN_3_PIO, IN_3_IDX_MASK);
					pio_clear(IN_4_PIO, IN_4_IDX_MASK);
				}
				
				if(sequencia == 3){
					pio_clear(IN_1_PIO, IN_1_IDX_MASK);
					pio_clear(IN_2_PIO, IN_2_IDX_MASK);
					pio_set(IN_3_PIO, IN_3_IDX_MASK);
					pio_clear(IN_4_PIO, IN_4_IDX_MASK);
				}
				
				if(sequencia == 4){
					pio_clear(IN_1_PIO, IN_1_IDX_MASK);
					pio_clear(IN_2_PIO, IN_2_IDX_MASK);
					pio_clear(IN_3_PIO, IN_3_IDX_MASK);
					pio_set(IN_4_PIO, IN_4_IDX_MASK);
				}
					
			}
		}
	}
}


/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		.charlength = CONF_UART_CHAR_LENGTH,
		.paritytype = CONF_UART_PARITY,
		.stopbits = CONF_UART_STOP_BITS,
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

static void BUT_init(void) {
	
	pmc_enable_periph_clk(BUT_1_PIO_ID);
	pmc_enable_periph_clk(BUT_2_PIO_ID);
	pmc_enable_periph_clk(BUT_3_PIO_ID);
	
	pio_configure(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
	pio_configure(BUT_2_PIO, PIO_INPUT, BUT_2_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
	pio_configure(BUT_3_PIO, PIO_INPUT, BUT_3_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
	
	pio_handler_set(BUT_1_PIO, BUT_1_PIO_ID, BUT_1_IDX_MASK, PIO_IT_FALL_EDGE,
	but1_callback);
	pio_handler_set(BUT_2_PIO, BUT_2_PIO_ID, BUT_2_IDX_MASK, PIO_IT_FALL_EDGE,
	but2_callback);
	pio_handler_set(BUT_3_PIO, BUT_3_PIO_ID, BUT_3_IDX_MASK, PIO_IT_FALL_EDGE,
	but3_callback);
	
	pio_enable_interrupt(BUT_1_PIO, BUT_1_IDX_MASK);
	pio_enable_interrupt(BUT_2_PIO, BUT_2_IDX_MASK);
	pio_enable_interrupt(BUT_3_PIO, BUT_3_IDX_MASK);
	
	pio_get_interrupt_status(BUT_1_PIO);
	pio_get_interrupt_status(BUT_2_PIO);
	pio_get_interrupt_status(BUT_3_PIO);

	NVIC_EnableIRQ(BUT_1_PIO_ID);
	NVIC_SetPriority(BUT_1_PIO_ID, 4);

	NVIC_EnableIRQ(BUT_2_PIO_ID);
	NVIC_SetPriority(BUT_2_PIO_ID, 4);

	NVIC_EnableIRQ(BUT_3_PIO_ID);
	NVIC_SetPriority(BUT_3_PIO_ID, 4);
	
	//configuracao motor de passos
	pmc_enable_periph_clk(IN_1_PIO_ID);
	pmc_enable_periph_clk(IN_2_PIO_ID);
	pmc_enable_periph_clk(IN_3_PIO_ID);
	pmc_enable_periph_clk(IN_4_PIO_ID);
	
	pio_configure(IN_1_PIO, PIO_OUTPUT_0, IN_1_IDX_MASK, PIO_DEFAULT);
	pio_configure(IN_2_PIO, PIO_OUTPUT_0, IN_2_IDX_MASK, PIO_DEFAULT);
	pio_configure(IN_3_PIO, PIO_OUTPUT_0, IN_3_IDX_MASK, PIO_DEFAULT);
	pio_configure(IN_4_PIO, PIO_OUTPUT_0, IN_4_IDX_MASK, PIO_DEFAULT);
}

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}


/************************************************************************/
/* main                                                                 */
/************************************************************************/


int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	/* Initialize the console uart */
	configure_console();

	/* Create task to control oled */
	
	if (xTaskCreate(task_modo, "modo", TASK_OLED_STACK_SIZE, NULL, TASK_OLED_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create modo task\r\n");
	}
	
	if (xTaskCreate(task_motor, "motor", TASK_OLED_STACK_SIZE, NULL, TASK_OLED_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create motor task\r\n");
	}
	
	// cria fila de 64 slots de int
	xQueueMode = xQueueCreate(64, sizeof(int) );
	xQueueSteps = xQueueCreate(64, sizeof(int) );
	
	// verifica se fila foi criada corretamente
	if (xQueueMode == NULL){
		printf("falha em criar a fila mode \n");
	}
	// verifica se fila foi criada corretamente
	if (xQueueSteps == NULL){
		printf("falha em criar a fila mode \n");
	}
	
	xSemaphoreRTT = xSemaphoreCreateBinary();
	if (xSemaphoreRTT == NULL){
		printf("falha em criar o semaforo \n");
	}
	

	/* Start the scheduler. */
	vTaskStartScheduler();

  /* RTOS não deve chegar aqui !! */
	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
