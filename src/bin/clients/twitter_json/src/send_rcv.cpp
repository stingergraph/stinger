#include "proto/twitter_json.pb.h"

#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace gt::stinger;

bool
send_message(int socket, StingerBatch & batch) {
  google::protobuf::uint64 batch_length = batch.ByteSize();
  int64_t prefix_length = sizeof(batch_length);
  int64_t buffer_length = prefix_length + batch_length;
  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [buffer_length];

  google::protobuf::io::ArrayOutputStream array_output(buffer, buffer_length);
  google::protobuf::io::CodedOutputStream coded_output(&array_output);

  coded_output.WriteLittleEndian64(batch_length);
  batch.SerializeToCodedStream(&coded_output);

  int sent_bytes = write(socket, buffer, buffer_length);

  delete [] buffer;

  if (sent_bytes != buffer_length) {
    return false;
  }

  return true;
}

bool
recv_message(int socket, gt::stinger::StingerBatch & batch) {
  google::protobuf::uint64 batch_length;
  int64_t prefix_length = sizeof(batch_length);
  google::protobuf::uint8 prefix[prefix_length];

  if (prefix_length != read(socket, prefix, prefix_length)) {
    return false;
  }
  google::protobuf::io::CodedInputStream::ReadLittleEndian64FromArray(prefix,
      &batch_length);

  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [batch_length];
  if (batch_length != read(socket, buffer, batch_length)) {
    delete [] buffer;
    return false;
  }
  google::protobuf::io::ArrayInputStream array_input(buffer, batch_length);
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  if (!batch.ParseFromCodedStream(&coded_input)) {
    delete [] buffer;
    return false;
  }

  delete [] buffer;
  return true;
}
