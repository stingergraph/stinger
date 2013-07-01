#include "proto/stinger-batch.pb.h"

using namespace gt::stinger;


bool send_message(int socket, gt::stinger::StingerBatch & batch);
bool recv_message(int socket, gt::stinger::StingerBatch & batch);
