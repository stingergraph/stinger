#ifndef WEB_H_
#define WEB_H_

#include "mongoose.h"
#include "stinger.h"

void
web_start_stinger(stinger_t * stinger, const char * port);

int
begin_request_handler(struct mg_connection *conn);

#endif
