#include "communications.h"
#include "control.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This module implements a small line-based command interface:
 *
 *   UART bytes -> rx_buffer -> completed line -> command handler -> response
 *
 * The application repeatedly calls Communications_Process(). Each call waits
 * briefly for one character, so a command is assembled over several calls.
 */

/*
 * A C string needs one extra byte for its terminating '\0'. Therefore the
 * longest accepted command is RX_BUFFER_SIZE - 1 characters.
 */
#define RX_BUFFER_SIZE 32U
#define RESPONSE_SIZE  48U

/*
 * This pointer remembers the UART handle supplied by main.c. It points to
 * CubeMX's huart2 object; it does not make a copy of that object. NULL means
 * Communications_Init() has not supplied a usable UART yet.
 */
static UART_HandleTypeDef *communication_uart = NULL;

/* File-private state used while receiving one command a character at a time. */
static char rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_length = 0U;

/* These helpers are static because only this source file needs to call them. */
static void Communications_Send(const char *message);
static void Communications_HandleCommand(const char *command);


void Communications_Init(UART_HandleTypeDef *uart)
{
    /* Save the address of huart2 and begin with an empty receive buffer. */
    communication_uart = uart;
    rx_length = 0U;

    /* Tell the computer that initialization reached the UART interface. */
    Communications_Send("READY\r\n");
}


void Communications_Process(void)
{
    /* One invocation receives at most one character. */
    uint8_t received_byte;
    HAL_StatusTypeDef result;

    /* Avoid dereferencing the pointer if initialization was skipped. */
    if (communication_uart == NULL)
    {
        return;
    }

    /*
     * Wait up to 10 ms for one byte.
     * A timeout is normal when the user is not typing.
     */
    result = HAL_UART_Receive(
        communication_uart,  /* UART peripheral to read from. */
        &received_byte,       /* Address where HAL stores the byte. */
        1U,                   /* Number of bytes requested. */
        10U                   /* Maximum wait in milliseconds. */
    );

    /* HAL_TIMEOUT is expected whenever no character arrives within 10 ms. */
    if (result != HAL_OK)
    {
        return;
    }

    /*
     * Treat either Enter character as the end of a command.
     * This works with CR, LF, and CRLF terminal settings.
     */
    if ((received_byte == '\r') || (received_byte == '\n'))
    {
        if (rx_length > 0U)
        {
            /* '\0' turns the collected characters into a valid C string. */
            rx_buffer[rx_length] = '\0';
            Communications_HandleCommand(rx_buffer);

            /* The next received character will begin a new command. */
            rx_length = 0U;
        }

        return;
    }

    /*
     * Leave one position available for the terminating '\0'.
     */
    if (rx_length < (RX_BUFFER_SIZE - 1U))
    {
        /* Append this character, then advance to the next free position. */
        rx_buffer[rx_length] = (char)received_byte;
        rx_length++;
    }
    else
    {
        rx_length = 0U;
        Communications_Send("ERR line\r\n");
    }
}


static void Communications_HandleCommand(const char *command)
{
    /* Temporary storage for replies that contain a numeric value. */
    char response[RESPONSE_SIZE];

    /* strcmp() returns zero only when the complete strings are identical. */
    if (strcmp(command, "OFF") == 0)
    {
        Load_Off();
        Communications_Send("OK PWM=0\r\n");
        return;
    }

    if (strcmp(command, "STATUS") == 0)
    {
        /*
         * snprintf() inserts the two values without writing past response.
         * The conditional operator converts bool into the protocol's 1 or 0.
         */
        snprintf(
            response,
            sizeof(response),
            "STATUS pwm=%u enabled=%u\r\n",
            (unsigned int)Load_GetPWM(),
            Load_IsEnabled() ? 1U : 0U
        );

        Communications_Send(response);
        return;
    }

    if (strncmp(command, "PWM ", 4U) == 0)
    {
        /*
         * "PWM " occupies indexes 0 through 3. Adding 4 advances the pointer
         * to the first character of the number: "PWM 25" -> "25".
         */
        const char *number_text = command + 4;
        char *number_end;
        long requested_pwm;

        /* Base 10 converts the decimal text and reports where parsing ended. */
        requested_pwm = strtol(number_text, &number_end, 10);

        /*
         * Reject missing numbers and extra characters:
         * "PWM abc", "PWM", and "PWM 25x".
         */
        if ((number_end == number_text) || (*number_end != '\0'))
        {
            Communications_Send("ERR command\r\n");
            return;
        }

        /*
         * Reject out-of-range commands rather than allowing
         * Load_SetPWM() to silently clamp them.
         */
        if ((requested_pwm < 0L) || (requested_pwm > 100L))
        {
            Communications_Send("ERR range\r\n");
            return;
        }

        Load_SetPWM((int32_t)requested_pwm);

        /* Build the acknowledgement using the accepted numeric request. */
        snprintf(
            response,
            sizeof(response),
            "OK PWM=%ld\r\n",
            requested_pwm
        );

        Communications_Send(response);
        return;
    }

    /* Reaching this point means no supported command matched the line. */
    Communications_Send("ERR command\r\n");
}


static void Communications_Send(const char *message)
{
    if (communication_uart == NULL)
    {
        return;
    }

    /*
     * strlen() supplies the number of characters before '\0'. The HAL API
     * accepts a byte pointer, so the string pointer is cast to uint8_t *.
     * This blocking call waits at most 100 ms for transmission to complete.
     */
    HAL_UART_Transmit(
        communication_uart,
        (uint8_t *)message,
        (uint16_t)strlen(message),
        100U
    );
}
