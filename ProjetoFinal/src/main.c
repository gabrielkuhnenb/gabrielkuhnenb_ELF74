#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tx_api.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"

#include "inc/hw_memmap.h"

#include "system_TM4C1294.h"

#include "utils/uartstdio.h"

#define BUFFER_SIZE 16
#define DEMO_BYTE_POOL_SIZE 9120
#define DEMO_STACK_SIZE 1024
#define DEMO_QUEUE_SIZE 100

#define GPIO_PA0_U0RX 0x00000001
#define GPIO_PA1_U0TX 0x00000401

TX_THREAD controller;

TX_THREAD elevator_l;
TX_THREAD elevator_c;
TX_THREAD elevator_r;

TX_THREAD door_timer_l;
TX_THREAD door_timer_c;
TX_THREAD door_timer_r;

TX_QUEUE queue_l;
TX_QUEUE queue_c;
TX_QUEUE queue_r;

TX_QUEUE exec_queue_l;
TX_QUEUE exec_queue_c;
TX_QUEUE exec_queue_r;

TX_MUTEX mutex_0;

TX_BYTE_POOL byte_pool_0;

UCHAR byte_pool_memory[DEMO_BYTE_POOL_SIZE];

uint8_t buffer[BUFFER_SIZE];

void UARTInit(void);

void elevator_startup(char elevator);
void toggle_door(char elevator, char door_state);
void toggle_led(char elevator, char button, char button_state);
void move_elevator(char elevator, char direction);
char get_floor(char ms, char ls);

void controller_thread(ULONG param);
void left_elevator_thread(ULONG param);
void center_elevator_thread(ULONG param);
void right_elevator_thread(ULONG param);

void tx_application_define(void *first_unused_memory);

int main()
{
    IntMasterEnable();

    SysTickPeriodSet(10000000);
    SysTickIntEnable();
    SysTickEnable();

    UARTInit();

    tx_kernel_enter();
}

void UARTInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0))
    {
    }
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, (GPIO_PIN_0 | GPIO_PIN_1));
    UARTConfigSetExpClk(UART0_BASE, SystemCoreClock, (uint32_t)115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    UARTFIFOEnable(UART0_BASE);
}

void elevator_startup(char elevator)
{
    // get mutex
    UINT mutex_operation = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }

    // xr\n\r (ready)
    UARTCharPut(UART0_BASE, elevator);
    UARTCharPut(UART0_BASE, 'r');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');

    tx_thread_sleep(100);

    // xf\n\r (close door)
    UARTCharPut(UART0_BASE, elevator);
    UARTCharPut(UART0_BASE, 'f');
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');

    // put mutex
    mutex_operation = tx_mutex_put(&mutex_0);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }
}

void toggle_door(char elevator, char door_state)
{
    // get mutex
    UINT mutex_operation = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }

    if ((door_state == 'a') || (door_state == 'f'))
    {
        UARTCharPut(UART0_BASE, elevator);
        UARTCharPut(UART0_BASE, door_state);
        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');
    }
    else
    {
        // invalid
    }

    // put mutex
    mutex_operation = tx_mutex_put(&mutex_0);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }
    
    tx_thread_sleep(100);
}

void toggle_led(char elevator, char button, char button_state)
{
    // get mutex
    UINT mutex_operation = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }

    if ((button_state == 'L') || (button_state == 'D'))
    {
        // xLb\r (turn led on) or xDb\r (turn led off)
        UARTCharPut(UART0_BASE, elevator);
        UARTCharPut(UART0_BASE, button_state);
        UARTCharPut(UART0_BASE, button);
        UARTCharPut(UART0_BASE, '\r');
    }
    else
    {
        // invalid
    }

    // put mutex
    mutex_operation = tx_mutex_put(&mutex_0);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }
}

void move_elevator(char elevator, char direction)
{
    // get mutex
    UINT mutex_operation = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }

    if ((direction == 's') || (direction == 'd') || (direction == 'p'))
    {
        // xs\n\r (up), xd\n\r (down) or xp\n\r (stop)
        UARTCharPut(UART0_BASE, elevator);
        UARTCharPut(UART0_BASE, direction);
        UARTCharPut(UART0_BASE, '\n');
        UARTCharPut(UART0_BASE, '\r');
    }
    else
    {
        // invalid
    }

    // put mutex
    mutex_operation = tx_mutex_put(&mutex_0);

    if (mutex_operation != TX_SUCCESS)
    {
        return;
    }
}

char get_floor(char ms, char ls)
{
    int floor = (ms - '0') * 10 + (ls - '0');

    switch (floor)
    {
        case 0:
            return 'a';
        case 1:
            return 'b';
        case 2:
            return 'c';
        case 3:
            return 'd';
        case 4:
            return 'e';
        case 5:
            return 'f';
        case 6:
            return 'g';
        case 7:
            return 'h';
        case 8:
            return 'i';
        case 9:
            return 'j';
        case 10:
            return 'k';
        case 11:
            return 'l';
        case 12:
            return 'm';
        case 13:
            return 'n';
        case 14:
            return 'o';
        case 15:
            return 'p';
        default:
            return ' ';
    }
}

void controller_thread(ULONG param)
{
    char request_buffer[BUFFER_SIZE];
    char request_char;
    int iterator = 0;
    int processed = 0;
    UINT queue_operation;

    while (1)
    {
        while (UARTCharsAvail(UART0_BASE))
        {
            tx_thread_sleep(2);

            request_char = UARTCharGet(UART0_BASE);

            if ((request_char != '\n') && (request_char != '\r'))
            {
                request_buffer[iterator] = request_char;
                iterator++;

                if (request_char == 'F')
                {
                    memset(request_buffer, 0, sizeof(request_buffer));
                    iterator = 0;
                }
            }
            else
            {
                iterator = 0;
                processed = 1;
            }
        }

        if (processed == 1)
        {
            if (request_buffer[0] == 'e')
            {
                queue_operation = tx_queue_send(&queue_l, request_buffer, TX_WAIT_FOREVER);

                if (queue_operation != TX_SUCCESS)
                {
                    break;
                }
            }
            else if (request_buffer[0] == 'c')
            {
                queue_operation = tx_queue_send(&queue_c, request_buffer, TX_WAIT_FOREVER);
                
                if (queue_operation != TX_SUCCESS)
                {
                    break;
                }
            }
            else if (request_buffer[0] == 'd')
            {
                queue_operation = tx_queue_send(&queue_r, request_buffer, TX_WAIT_FOREVER);
                
                if (queue_operation != TX_SUCCESS)
                {
                    break;
                }
            }
            else
            {
                // invalid
            }

            memset(request_buffer, 0, sizeof(request_buffer));
            processed = 0;
        }
    }
}

void left_elevator_thread(ULONG param)
{
    char request[16];
    char floor = 'a';
    char new_request[16];
    char new_floor = 'a';
    UINT queue_operation;
    int button_pressed = 0;
    int handled_request = 0;

    elevator_startup('e');

    while (1)
    {
        memset(request, 0, sizeof(request));

        queue_operation = tx_queue_receive(&queue_l, request, TX_WAIT_FOREVER);

        if (queue_operation != TX_SUCCESS)
        {
            break;
        }

        if ((request[1] >= '0') && (request[1] <= '9'))
        {
            if ((request[2] >= '0') && (request[2] <= '9'))
            {
                floor = get_floor(request[1], request[2]);
            }
            else
            {
                floor = get_floor('0', request[1]);
            }
        }

        if ((request[1] == 'E') || (request[1] == 'I'))
        {
            button_pressed++;

            if (request[1] == 'I')
            {
                toggle_led('e', request[2], 'L');
            }

            toggle_door('e', 'f');

            queue_operation = tx_queue_send(&exec_queue_l, request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if ((button_pressed > 0) && (strlen(new_request) == 0))
        {
            queue_operation = tx_queue_receive(&exec_queue_l, new_request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if (new_request[1] == 'E')
        {
            new_floor = get_floor(new_request[2], new_request[3]);
        }
        else if (new_request[1] == 'I')
        {
            new_floor = new_request[2];
        }

        if (new_floor > floor && handled_request == 0)
        {
            move_elevator('e', 's');
            handled_request = 1;
        }
        else if (new_floor < floor && handled_request == 0)
        {
            move_elevator('e', 'd');
            handled_request = 1;
        }
        if (new_floor == floor && strlen(new_request) != 0)
        {
            move_elevator('e', 'p');
            toggle_door('e', 'a');
            toggle_led('e', new_floor, 'D');
            button_pressed--;
            memset(new_request, 0, sizeof new_request);
            handled_request = 0;
        }
    }
}

void center_elevator_thread(ULONG param)
{
    char request[16];
    char floor = 'a';
    char new_request[16];
    char new_floor = 'a';
    UINT queue_operation;
    int button_pressed = 0;
    int handled_request = 0;

    elevator_startup('c');

    while (1)
    {
        memset(request, 0, sizeof(request));

        queue_operation = tx_queue_receive(&queue_c, request, TX_WAIT_FOREVER);

        if (queue_operation != TX_SUCCESS)
        {
            break;
        }

        if (request[1] >= '0' && request[1] <= '9')
        {
            if (request[2] >= '0' && request[2] <= '9')
            {
                floor = get_floor(request[1], request[2]);
            }
            else
            {
                floor = get_floor('0', request[1]);
            }
        }

        if (request[1] == 'E' || request[1] == 'I')
        {
            button_pressed++;

            if (request[1] == 'I')
            {
                toggle_led('c', request[2], 'L');
            }

            toggle_door('c', 'f');

            queue_operation = tx_queue_send(&exec_queue_c, request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if (button_pressed > 0 && strlen(new_request) == 0)
        {
            queue_operation = tx_queue_receive(&exec_queue_c, new_request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if (new_request[1] == 'E')
        {
            new_floor = get_floor(new_request[2], new_request[3]);
        }
        else if (new_request[1] == 'I')
        {
            new_floor = new_request[2];
        }

        if (new_floor > floor && handled_request == 0)
        {
            move_elevator('c', 's');
            handled_request = 1;
        }
        else if (new_floor < floor && handled_request == 0)
        {
            move_elevator('c', 'd');
            handled_request = 1;
        }
        if (new_floor == floor && strlen(new_request) != 0)
        {
            move_elevator('c', 'p');
            toggle_door('c', 'a');
            toggle_led('c', new_floor, 'D');
            button_pressed--;
            memset(new_request, 0, sizeof new_request);
            handled_request = 0;
        }
    }
}

void right_elevator_thread(ULONG param)
{
    char request[16];
    char floor = 'a';
    char new_request[16];
    char new_floor = 'a';
    UINT queue_operation;
    int button_pressed = 0;
    int handled_request = 0;

    elevator_startup('d');

    while (1)
    {
        memset(request, 0, sizeof(request));

        queue_operation = tx_queue_receive(&queue_r, request, TX_WAIT_FOREVER);

        if (queue_operation != TX_SUCCESS)
        {
            break;
        }

        if (request[1] >= '0' && request[1] <= '9')
        {
            if (request[2] >= '0' && request[2] <= '9')
            {
                floor = get_floor(request[1], request[2]);
            }
            else
            {
                floor = get_floor('0', request[1]);
            }
        }

        if (request[1] == 'E' || request[1] == 'I')
        {
            button_pressed++;

            if (request[1] == 'I')
            {
                toggle_led('d', request[2], 'L');
            }

            toggle_door('d', 'f');

            queue_operation = tx_queue_send(&exec_queue_r, request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if (button_pressed > 0 && strlen(new_request) == 0)
        {
            queue_operation = tx_queue_receive(&exec_queue_r, new_request, TX_WAIT_FOREVER);

            if (queue_operation != TX_SUCCESS)
            {
                break;
            }
        }

        if (new_request[1] == 'E')
        {
            new_floor = get_floor(new_request[2], new_request[3]);
        }
        else if (new_request[1] == 'I')
        {
            new_floor = new_request[2];
        }

        if (new_floor > floor && handled_request == 0)
        {
            move_elevator('d', 's');
            handled_request = 1;
        }
        else if (new_floor < floor && handled_request == 0)
        {
            move_elevator('d', 'd');
            handled_request = 1;
        }
        if (new_floor == floor && strlen(new_request) != 0)
        {
            move_elevator('d', 'p');
            toggle_door('d', 'a');
            toggle_led('d', new_floor, 'D');
            button_pressed--;
            memset(new_request, 0, sizeof new_request);
            handled_request = 0;
        }
    }
}

void tx_application_define(void *first_unused_memory)
{
    CHAR *pointer;

#ifdef TX_ENABLE_EVENT_TRACE
    tx_trace_enable(trace_buffer, sizeof(trace_buffer), 32);
#endif

    // byte pool, threads, mutex, queues

    tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory, DEMO_BYTE_POOL_SIZE);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&controller, "controller thread", controller_thread, 1, pointer, DEMO_STACK_SIZE, 0, 0, 20, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&elevator_l, "left elevator thread", left_elevator_thread, 2, pointer, DEMO_STACK_SIZE, 0, 0, 20, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&elevator_c, "center elevator thread", center_elevator_thread, 3, pointer, DEMO_STACK_SIZE, 0, 0, 20, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&elevator_r, "right elevator thread", right_elevator_thread, 4, pointer, DEMO_STACK_SIZE, 0, 0, 20, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_mutex_create(&mutex_0, "mutex 0", TX_NO_INHERIT);

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&queue_l, "left elevator queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&queue_c, "center elevator queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&queue_r, "right elevator queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&exec_queue_l, "left executed queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&exec_queue_c, "center executed queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    tx_queue_create(&exec_queue_r, "right executed queue", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    tx_block_release(pointer);
}