#ifndef WEB_H_
#define WEB_H_

#include "stinger_core/stinger.h"
#include "mongoose/mongoose.h"

void
web_start_stinger(stinger_t * stinger, const char * port);

int
begin_request_handler(struct mg_connection *conn);

#endif
