#ifndef COAP_TCS37727_H
#define COAP_TCS37727_H

#include <inttypes.h>

#include "net/gcoap.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t tcs37727_illuminance_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len);

void init_tcs37727_sender(void);

#ifdef __cplusplus
}
#endif

#endif /* COAP_TCS37727_H */
