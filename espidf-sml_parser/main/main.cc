#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <driver/uart.h>
#include <soc/uart_reg.h>
#include <driver/gpio.h>
#include <string.h>
#include <math.h>
#include <sml/sml_file.h>
#include <sml/sml_transport.h>
#include <sml/sml_value.h>
#include "unit.h"

#define TAG "MAIN"

constexpr size_t BUF_SIZE = (9600 / (1 + 8 + 1)) * 0.4 * 2;
constexpr uart_port_t UART_NUM{UART_NUM_2};
constexpr uint8_t PATTERN_CHAR{0x1b};
constexpr uint8_t PATTERN_SIZE{4};

constexpr TickType_t TIMEOUT{pdMS_TO_TICKS(400)};

static QueueHandle_t uart_queue;

// globals
int sflag = false; // flag to process only a single OBIS data stream
int vflag = false; // verbose flag

/*
Example Output
129-129:199.130.3*255#ISK#
1-0:0.0.9*255#06 49 53 4b 01 0e 1f 5c 5f 8b #
1-0:1.8.0*255#19497150.0#Wh
1-0:1.8.1*255#19497150.0#Wh
1-0:1.8.2*255#0.0#Wh
1-0:2.8.0*255#638079.3#Wh
1-0:2.8.1*255#638079.3#Wh
1-0:2.8.2*255#0.0#Wh
1-0:16.7.0*255#1055#W
129-129:199.130.5*255#e8 9d 69 [...]
*/

void transport_receiver(unsigned char *buffer, size_t buffer_len)
{
	int i;
	// the buffer contains the whole message, with transport escape sequences.
	// these escape sequences are stripped here.
	sml_file *file = sml_file_parse(buffer + 8, buffer_len - 16);
	// the sml file is parsed now

	// this prints some information about the file
	if (vflag)
		sml_file_print(file);

	// read here some values ...
	if (vflag)
		printf("OBIS data\n");
	for (i = 0; i < file->messages_len; i++)
	{
		sml_message *message = file->messages[i];
		if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE)
		{
			sml_list *entry;
			sml_get_list_response *body;
			body = (sml_get_list_response *)message->message_body->data;
			for (entry = body->val_list; entry != NULL; entry = entry->next)
			{
				if (!entry->value)
				{ // do not crash on null value
					fprintf(stderr, "Error in data stream. entry->value should not be NULL. Skipping this.\n");
					continue;
				}
				if (entry->value->type == SML_TYPE_OCTET_STRING)
				{
					char *str;
					printf("%d-%d:%d.%d.%d*%d#%s#\n",
						   entry->obj_name->str[0], entry->obj_name->str[1],
						   entry->obj_name->str[2], entry->obj_name->str[3],
						   entry->obj_name->str[4], entry->obj_name->str[5],
						   sml_value_to_strhex(entry->value, &str, true));
					free(str);
				}
				else if (entry->value->type == SML_TYPE_BOOLEAN)
				{
					printf("%d-%d:%d.%d.%d*%d#%s#\n",
						   entry->obj_name->str[0], entry->obj_name->str[1],
						   entry->obj_name->str[2], entry->obj_name->str[3],
						   entry->obj_name->str[4], entry->obj_name->str[5],
						   entry->value->data.boolean ? "true" : "false");
				}
				else if (((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_INTEGER) ||
						 ((entry->value->type & SML_TYPE_FIELD) == SML_TYPE_UNSIGNED))
				{
					double value = sml_value_to_double(entry->value);
					int scaler = (entry->scaler) ? *entry->scaler : 0;
					int prec = -scaler;
					if (prec < 0)
						prec = 0;
					value = value * pow(10, scaler);
					printf("%d-%d:%d.%d.%d*%d#%.*f#",
						   entry->obj_name->str[0], entry->obj_name->str[1],
						   entry->obj_name->str[2], entry->obj_name->str[3],
						   entry->obj_name->str[4], entry->obj_name->str[5], prec, value);
					const char *unit = NULL;
					if (entry->unit && // do not crash on null (unit is optional)
						(unit = dlms_get_unit((unsigned char)*entry->unit)) != NULL)
						printf("%s", unit);
					printf("\n");
					// flush the stdout puffer, that pipes work without waiting
					fflush(stdout);
				}
			}
			if (sflag)
				exit(0); // processed first message - exit
		}
	}

	// free the malloc'd memory
	sml_file_free(file);
}

void sml_read(uint8_t *buf, size_t exactSize)
{
	size_t alreadyRead{0};
	while (alreadyRead < exactSize)
	{
		alreadyRead += uart_read_bytes(UART_NUM, buf + alreadyRead, exactSize - alreadyRead, TIMEOUT);
	}
}

void sml_loop()
{
	size_t bufPos{0};
	uint8_t *buf = new uint8_t[BUF_SIZE];
	int delimiterLen{0};
	TickType_t previousTime = xTaskGetTickCount();
	while (true)
	{
		bufPos += uart_read_bytes(UART_NUM, buf + bufPos, BUF_SIZE - bufPos, TIMEOUT);
		while (bufPos>16 && delimiterLen == 13)
		{
			// Search Start
			size_t start{0};
			delimiterLen=0;
			while (start < bufPos && delimiterLen < 8)
			{
				if ((buf[start] == 0x1b && delimiterLen < 4) || (buf[start] == 0x01 && delimiterLen >= 4))
				{
					delimiterLen++;
				}
				else
				{
					delimiterLen = 0;
				}
				start++;
			}
			if (delimiterLen < 8)
			{
				//aaah, gefährlich. Der Anfang des Delimiters könnte ja gerade am ende des Buffers sein.
				//Das würde aber nur beim Start passieren oder wenn zwischendurch "Müll" gesendet wird -->ist ok!
				//in den anderen Fällen ist davon auszugehen, dass ein start gefunden wird
				bufPos = 0;		  // Kein Start gefunden - wir löschen den buffer
				delimiterLen = 0; // und stellen fest, dass wir keinen delimiter gefunden haben
				continue;
			}
			// Search end
			size_t end{start};
			while (end < bufPos && delimiterLen < 13)
			{
				if ((buf[end] == 0x1b && delimiterLen < 12) || (buf[end] == 0x1a && delimiterLen == 12))
				{
					delimiterLen++;
				}
				else
				{
					delimiterLen = 0;
				}
				end++;
			}
			if (delimiterLen < 13) // aber größer, als 8!
			{
				//Nachricht nach vorne schieben
				memmove((void*)buf, (void*)buf+start, bufPos-start);
				bufPos-=start;
				continue;
			}
			transport_receiver(buf+start, end-start);
			memmove((void*)buf, (void*)buf+start, bufPos-end);
			bufPos-=end;
		}

		vTaskDelayUntil(&previousTime, pdMS_TO_TICKS(500));
	}
}

extern "C" void app_main(void)
{
	ESP_LOGI(TAG, "Willkommen!");
	uart_config_t uart_config = {};
	uart_config.baud_rate = 9600;
	uart_config.data_bits = UART_DATA_8_BITS;
	uart_config.parity = UART_PARITY_DISABLE;
	uart_config.stop_bits = UART_STOP_BITS_1;
	uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uart_config.source_clk = UART_SCLK_DEFAULT;
	uart_config.rx_flow_ctrl_thresh = 0;
	ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));

	// Set UART pins(TX: IO4, RX: IO5)
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE, 0, UART_EVENT_QUEUE_SIZE, nullptr, 0));

	const uint8_t esc_seq[] = {0x1b, 0x1b, 0x1b, 0x1b};

	while (true)
	{
		memset(buf, 0, BUF_SIZE);
		size_t len = 0;
		while (len < 8)
		{
			sml_read(buf + len, 1);
			if ((buf[len] == 0x1b && len < 4) || (buf[len] == 0x01 && len >= 4))
			{
				len++;
			}
			else
			{
				len = 0;
			}
		}
		ESP_LOGI(TAG, "found start sequence, now consume message");

		while ((len + 8) < BUF_SIZE)
		{
			sml_read(&(buf[len]), 4);

			if (memcmp(&buf[len], esc_seq, 4) != 0)
			{
				len += 4;
			}
			else
			{
				// found esc sequence
				len += 4;
				sml_read(&(buf[len]), 4);

				if (buf[len] == 0x1a)
				{
					ESP_LOGI(TAG, "found end sequence");
					len += 4;
					transport_receiver(buf, len);
				}
				else
				{
					// don't read other escaped sequences yet
					ESP_LOGW(TAG, "error: unrecognized sequence");
				}
				break;
			}
		}
	}
}
