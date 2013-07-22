#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

template<typename T>
bool
send_message(int socket, T & message) {
  google::protobuf::uint64 message_length = message.ByteSize();
  int64_t prefix_length = sizeof(message_length);
  int64_t buffer_length = prefix_length + message_length;
  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [buffer_length];

  google::protobuf::io::ArrayOutputStream array_output(buffer, buffer_length);
  google::protobuf::io::CodedOutputStream coded_output(&array_output);

  coded_output.WriteLittleEndian64(message_length);
  message.SerializeToCodedStream(&coded_output);

  int sent_bytes = write(socket, buffer, buffer_length);

  delete [] buffer;

  if (sent_bytes != buffer_length) {
    return false;
  }

  return true;
}

template<typename T>
bool
recv_message(int socket, T & message) {
  google::protobuf::uint64 message_length;
  int64_t prefix_length = sizeof(message_length);
  google::protobuf::uint8 prefix[prefix_length];

  if (prefix_length != read(socket, prefix, prefix_length)) {
    return false;
  }
  google::protobuf::io::CodedInputStream::ReadLittleEndian64FromArray(prefix,
      &message_length);

  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [message_length];
  if (message_length != read(socket, buffer, message_length)) {
    delete [] buffer;
    return false;
  }
  google::protobuf::io::ArrayInputStream array_input(buffer, message_length);
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  if (!message.ParseFromCodedStream(&coded_input)) {
    delete [] buffer;
    return false;
  }

  delete [] buffer;
  return true;
}
