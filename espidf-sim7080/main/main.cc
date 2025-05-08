#include <stddef.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <cstring>
#include <limits.h>
#include <stdbool.h>
#include <cstdint>
#include <ctype.h>
#include <array>
#include <vector>
#include <errno.h>

constexpr uint8_t UART{UART_NUM_2};
constexpr char PATTERN_CHAR{'\n'};
constexpr size_t PATTERN_CHR_NUM{1};
constexpr size_t BUF_SIZE{256};
constexpr size_t RD_BUF_SIZE{256};
constexpr gpio_num_t PIN_DTR{GPIO_NUM_25};
constexpr gpio_num_t PIN_TX{GPIO_NUM_27}; // Microcontroller side
constexpr gpio_num_t PIN_RX{GPIO_NUM_26}; // Microcontroller side
constexpr gpio_num_t PIN_PWR{GPIO_NUM_4}; // GPIO pin used for PWRKEY
constexpr int BAUD{9600};

static QueueHandle_t uart0_queue;

static void uart_event_task(void *pvParameters)
{
	uart_event_t event;
	size_t buffered_size;
	uint8_t *dtmp = new uint8_t[RD_BUF_SIZE];
	std::vector<AtParam_t *> list;
	int pos{0};
	while (true)
	{
		// Waiting for UART event.
		if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
		{
			switch (event.type)
			{
			case UART_FIFO_OVF:
				ESP_LOGI(TAG, "hw fifo overflow");
				// If fifo overflow happened, you should consider adding flow control for your application.
				// The ISR has already reset the rx FIFO,
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input(UART);
				xQueueReset(uart0_queue);
				break;
			// Event of UART ring buffer full
			case UART_BUFFER_FULL:
				ESP_LOGI(TAG, "ring buffer full");
				// If buffer full happened, you should consider increasing your buffer size
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input(UART);
				xQueueReset(uart0_queue);
				break;

			// Event of UART parity check error
			case UART_PARITY_ERR:
				ESP_LOGI(TAG, "uart parity error");
				break;
			// Event of UART frame error
			case UART_FRAME_ERR:
				ESP_LOGI(TAG, "uart frame error");
				break;
			// UART_PATTERN_DET
			case UART_PATTERN_DET:
			{
				uart_get_buffered_data_len(UART, &buffered_size);
				pos = uart_pattern_pop_pos(UART);
				if (pos == -1)
				{
					ESP_LOGE(TAG, "There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not");
					uart_flush_input(UART);
					continue;
				}
				uart_read_bytes(UART, dtmp, pos + PATTERN_CHR_NUM, 0);
				if (dtmp[pos - 1] != '\r')
				{
					uart_flush_input(UART);
					ESP_LOGE(TAG, "CR missing before LF, flushing buffer!");
					continue;
				}
				if (pos == 1)
				{
					ESP_LOGD(TAG, "Found an empty string");
					continue;
				}

				int NULL_DETECTED{-1};
				int CR_DETECTED{-1};
				for (int i = 0; i < pos - 1; i++)
				{
					if (dtmp[i] == 0)
						NULL_DETECTED = i;
					if (dtmp[i] == '\r')
						CR_DETECTED = i;
				}
				if (NULL_DETECTED != -1 || CR_DETECTED != -1)
				{
					ESP_LOGE(TAG, "NULL_DETECTED!=-1 || CR_DETECTED!=-1");
				}
				dtmp[pos - 1] = '\0';
				ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d: %s NULL:%d CR:%d ", pos, buffered_size, dtmp, NULL_DETECTED, CR_DETECTED);
				// parse((char*)dtmp);
				at_parse_param(&dtmp, list, 20);
				if (list.size() > 0 &&)
					xSemaphoreGive(waitForResultSemaphore);
			}

			break;
			// Others
			default:
				ESP_LOGD(TAG, "uart event type: %d", event.type);
				break;
			}
		}
	}
	free(dtmp);
	dtmp = NULL;
	vTaskDelete(NULL);
}
/*
I
*/

extern "C" void app_main(void)
{


	uart_config_t uart_config = {};
	uart_config.baud_rate = BAUD;
	uart_config.data_bits = UART_DATA_8_BITS;
	uart_config.parity = UART_PARITY_DISABLE;
	uart_config.stop_bits = UART_STOP_BITS_1;
	uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uart_config.source_clk = UART_SCLK_DEFAULT;
	// Install UART driver, and get the queue.
	uart_driver_install(UART, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
	uart_param_config(UART, &uart_config);

	uart_set_pin(UART, PIN_TX, PIN_RX, GPIO_NUM_NC, GPIO_NUM_NC);

	// Set uart pattern detect function.
	uart_enable_pattern_det_baud_intr(UART, PATTERN_CHAR, PATTERN_CHR_NUM, 9, 0, 0);
	// Reset the pattern queue length to record at most 20 pattern positions.
	uart_pattern_queue_reset(UART, 20);

	// Create a task to handler UART event from ISR
	xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

	// Try to take mutex -->means, that device responded ready

	// issue simple AT commands in the meanwhile

	// give mutex back

	// upgrade to faster UART (1MHz)

	// get device info
}
