#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define STACK_SIZE 1024 * 2

QueueHandle_t GlobalQueue;

const char keys[4][4] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};
typedef enum
{
    KEY_UP,
    KEY_DOWN,
    KEY_NONE
} my_key_state_t;

const gpio_num_t rowPins[4] = {GPIO_NUM_14, GPIO_NUM_27, GPIO_NUM_26, GPIO_NUM_25};
const gpio_num_t colPins[4] = {GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_18, GPIO_NUM_19};

esp_err_t create_tasks(void);
void print_received_key(void *pvParameters);
void send_pressed_key_task(void *pvParameters);

esp_err_t init_keys(void);
char scan_keys(void);

void app_main(void)
{
    int ledLevel = 0;

    init_keys();
    create_tasks();
    GlobalQueue = xQueueCreate(20, sizeof(char));
    gpio_reset_pin(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);

    while (1)
    {
        ledLevel = !ledLevel;
        gpio_set_level(GPIO_NUM_13, ledLevel);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return;
}

esp_err_t create_tasks()
{
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    xTaskCreate(send_pressed_key_task,
                "reader",
                STACK_SIZE,
                &ucParameterToPass,
                1,
                &xHandle);
    xTaskCreate(print_received_key,
                "printer",
                STACK_SIZE,
                &ucParameterToPass,
                1,
                &xHandle);

    return ESP_OK;
}

void print_received_key(void *pvParameters)
{
    char receivedValue = '\0';

    while (1)
    {
        if (xQueueReceive(GlobalQueue, &receivedValue, pdMS_TO_TICKS(50)))
        {
            ESP_LOGI("printer", "Received key: %c", receivedValue);
        }
    }
}

void send_pressed_key_task(void *pvParameters)
{
    char value = '\0';

    while (1)
    {
        value = scan_keys();
        if (value)
        {
            if (!xQueueSend(GlobalQueue, &value, pdMS_TO_TICKS(100)))
            {
                ESP_LOGE("sender", "Sended key: %c", value);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

esp_err_t init_keys(void)
{
    for (int i = 0; i < 4; i++)
    {
        gpio_reset_pin(rowPins[i]);
        gpio_set_direction(rowPins[i], GPIO_MODE_OUTPUT);

        gpio_reset_pin(colPins[i]);
        gpio_set_direction(colPins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(colPins[i], GPIO_PULLDOWN_ENABLE);
    }

    return ESP_OK;
}

char scan_keys(void)
{
    my_key_state_t keyState;

    for (int row = 0; row < 4; row++)
    {
        for (int col = 0; col < 4; col++)
        {

            gpio_set_level(rowPins[row], 1);
            keyState = KEY_UP;

            switch (gpio_get_level(colPins[col]))
            {
            case KEY_DOWN:
                keyState = KEY_DOWN;
                break;
            case KEY_UP:
                keyState = KEY_UP;
                break;
            default:
                keyState = KEY_NONE;
                break;
            }

            gpio_set_level(rowPins[row], 0);

            if (keyState == KEY_DOWN)
            {
                return keys[row][col];
            }
        }
    }

    return '\0';
}