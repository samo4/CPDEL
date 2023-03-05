#ifndef _ROUTES_H_
#define _ROUTES_H_

#include <ESPAsyncWebServer.h>

void routes_begin(void);

void routes_sent_event(uint8_t idx);

#endif
