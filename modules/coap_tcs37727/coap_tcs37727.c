#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msg.h"
#include "thread.h"
#include "xtimer.h"
#include "periph/i2c.h"

#include "tcs37727_params.h"
#include "tcs37727.h"

#include "net/gcoap.h"

#include "coap_utils.h"
#include "coap_tcs37727.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define SEND_INTERVAL        (1000000UL)    /* set updates interval to 5 seconds */

#define TCS37727_QUEUE_SIZE    (8)

/* TCS37727 sensor */
#define I2C_DEVICE (0)

static msg_t _tcs37727_msg_queue[TCS37727_QUEUE_SIZE];
static char tcs37727_stack[THREAD_STACKSIZE_DEFAULT];

static tcs37727_t tcs37727_dev;
static uint8_t response[64] = { 0 };

ssize_t tcs37727_illuminance_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len)
{
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    memset(response, 0, sizeof(response));
    tcs37727_data_t light_data;
    tcs37727_read(&tcs37727_dev, &light_data);
    sprintf((char*)response, "%ilx", (int)light_data.lux);
    size_t payload_len = sizeof(response);
    memcpy(pdu->payload, response, payload_len);

    return gcoap_finish(pdu, payload_len, COAP_FORMAT_TEXT);
    return 0;
}

void *tcs37727_thread(void *args)
{
    msg_init_queue(_tcs37727_msg_queue, TCS37727_QUEUE_SIZE);

    for(;;) {
        size_t p = 0;
        p += sprintf((char*)&response[p], "illuminance:");
        tcs37727_data_t light_data;
        tcs37727_read(&tcs37727_dev, &light_data);
        p += sprintf((char*)&response[p],
                     "%ilx", (int)light_data.lux);
        response[p] = '\0';
        send_coap_post((uint8_t*)"/server", response);

        /* wait 5 seconds */
        xtimer_usleep(SEND_INTERVAL);
    }

    return NULL;
}

void init_tcs37727_sender(void)
{
    /* Initialize the TCS37727 sensor */
    printf("+------------Initializing TCS37727 sensor ------------+\n");
    int result = tcs37727_init(&tcs37727_dev, &tcs37727_params[0]);
    if (result == -1) {
        puts("[Error] The given i2c is not enabled");
    }
    else if (result != -TCS37727_OK) {
        puts("[Error] The sensor did not answer correctly on the given address");
    }
    else {
        printf("Initialization successful\n\n");
    }

    /* create the sensors thread that will send periodic updates to
       the server */
    int tcs37727_pid = thread_create(tcs37727_stack, sizeof(tcs37727_stack),
                                    THREAD_PRIORITY_MAIN - 1,
                                    THREAD_CREATE_STACKTEST, tcs37727_thread,
                                    NULL, "tcs37727 thread");
    if (tcs37727_pid == -EINVAL || tcs37727_pid == -EOVERFLOW) {
        puts("Error: failed to create tcs37727 thread, exiting\n");
    }
    else {
        puts("Successfuly created tcs37727 thread !\n");
    }
}
