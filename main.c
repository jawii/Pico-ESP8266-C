#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <string.h>

#define UART_ID uart0
#define BAUD_RATE 115200

// Uart pins
#define UART_TX_PIN 0
#define UART_RX_PIN 1

char SSID[] = "SSID";
char password[] = "SSID_PASS";
char ServerIP[] = "SERVER_IP";
char Port[] = "SERVER_PORT";
char uart_command[50] = "";
char buf[256] = {0};

bool send_sensor_values(const char *sensorName, const char *humidity, const char *temperature);
bool sendCMD(const char *cmd, const char *act);
void connectToWifi();

int main()
{
    stdio_init_all();
    sleep_ms(5000);

    printf("\nProgram start\n");

    uart_init(UART_ID, BAUD_RATE); // Set up UART

    // Set the TX and RX pins using the function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_puts(UART_ID, "+++");
    sleep_ms(1000);
    while (uart_is_readable(UART_ID))
        uart_getc(UART_ID);
    sleep_ms(2000);

    /*
    // Remove comments to connect to wifi 
    sleep_ms(2000);
    connectToWifi(); 
    sleep_ms(2000);
    */

    // Uncomment to get the IP address of the ESP
    // sendCMD("AT+CIFSR", "OK"); // ASK IP

    // Example values
    send_sensor_values("test", "59.2", "42.2");

    while (1)
    {
        printf("\nProgram End");
        sleep_ms(1000);
    }
}

/// Send sensor values to REST api using TCP connection
bool send_sensor_values(const char *sensorName, const char *humidity, const char *temperature)
{
    // Open connection
    sprintf(uart_command, "AT+CIPSTART=\"TCP\",\"%s\",%s", ServerIP, Port);
    sendCMD(uart_command, "OK");

    // Send data
    sendCMD("AT+CIPMODE=1", "OK");
    sleep_ms(1000);
    sendCMD("AT+CIPSEND", ">");
    sleep_ms(2000);

    char data[100];
    sniprintf(data, sizeof(data), "{\"sensor\":\"%s\",\"temperature\":\"%s\", \"humidity\":\"%s\"}", sensorName, humidity, temperature);
    int data_size = strlen(data);
    char post[] = "POST /api/sensor-values HTTP/1.1\r\n";

    char host[100];
    snprintf(host, sizeof(host), "Host: %s\r\n", ServerIP);
    char content_type[] = "Content-Type: application/json\r\n";
    char accept[] = "Accept: application/json\r\n";
    char content_length[100];
    sniprintf(content_length, sizeof(content_length), "Content-Length: %d\r\n", data_size);

    snprintf(buf, sizeof(buf), "%s%s%s%s%s\r\n%s", post, host, content_type, accept, content_length, data);
    printf(buf);
    uart_puts(UART_ID, buf);

    sleep_ms(5000);            // Seems like this is critical
    uart_puts(UART_ID, "+++"); // Look for ESP docs

    // Close connection
    sleep_ms(1000);
    sendCMD("AT+CIPCLOSE", "OK");
    sleep_ms(1000);
    sendCMD("AT+CIPMODE=0", "OK");

    return true;
}

bool sendCMD(const char *cmd, const char *act)
{
    int i = 0;
    uint64_t t = 0;

    uart_puts(UART_ID, cmd);
    uart_puts(UART_ID, "\r\n");

    t = time_us_64();
    while (time_us_64() - t < 2500 * 1000)
    {
        while (uart_is_readable_within_us(UART_ID, 2000))
        {
            buf[i++] = uart_getc(UART_ID);
        }
        if (i > 0)
        {
            buf[i] = '\0';
            printf("%s\r\n", buf);
            if (strstr(buf, act) != NULL)
            {
                return true;
            }
            else
            {
                i = 0;
            }
        }
    }
    //printf("false\r\n");
    return false;
}

void connectToWifi()
{
    sendCMD("AT", "OK");
    sendCMD("AT+CWMODE=3", "OK");
    sprintf(uart_command, "AT+CWJAP=\"%s\",\"%s\"", SSID, password);
    sendCMD(uart_command, "OK");
}