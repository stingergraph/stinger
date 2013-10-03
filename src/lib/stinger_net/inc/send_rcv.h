#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#ifndef  SEND_RCV_H
#define  SEND_RCV_H


template<typename T>
bool
send_message(int socket, T & message) {
  int32_t message_length = message.ByteSize();
  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [message_length];

  google::protobuf::io::ArrayOutputStream array_output(buffer, message_length);
  google::protobuf::io::CodedOutputStream coded_output(&array_output);

  message.SerializeToCodedStream(&coded_output);

  int32_t nl_message_length = htonl(message_length);
  int sent_bytes = write(socket, &nl_message_length, sizeof(nl_message_length));

  if(sent_bytes > 0) {
    sent_bytes += write(socket, buffer, message_length);
  }

  delete [] buffer;

  if (sent_bytes != message_length + sizeof(message_length)) {
    return false;
  }

  return true;
}

template<typename T>
bool
recv_message(int socket, T & message) {
  int32_t message_length = 0;

  if (sizeof(message_length) != read(socket, &message_length, sizeof(message_length))) {
    return false;
  }
  message_length = ntohl(message_length);

  struct timeval tv;
  tv.tv_sec = 30;
  tv.tv_usec = 0;
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

  google::protobuf::uint8 * buffer = new google::protobuf::uint8 [message_length];
  int32_t bytes_read = 0;
  int32_t last_read = 0;
  while(message_length != (bytes_read += read(socket, buffer + bytes_read, message_length - bytes_read))) {
    usleep(100);
    if(bytes_read == last_read) {
      delete [] buffer;
      return false;
    }
    last_read = bytes_read;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 0;
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

  google::protobuf::io::ArrayInputStream array_input(buffer, message_length);
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  if (!message.ParseFromCodedStream(&coded_input)) {
    delete [] buffer;
    return false;
  }

  delete [] buffer;
  return true;
}

#endif  /*SEND_RCV_H*/
