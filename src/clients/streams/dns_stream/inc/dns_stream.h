#ifndef _DNS_STREAM_H
#define _DNS_STREAM_H

#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"
#include <iostream>
#include <sstream>

#undef LOG_AT_F
#undef LOG_AT_E
#undef LOG_AT_W
#undef LOG_AT_I
#undef LOG_AT_V
#undef LOG_AT_D

#define LOG_AT_D
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int64_t get_current_timestamp(void);

struct NetflowParse {

	std::string src, dest, protocol, src_port, dest_port;
	double duration;
	int packets, bytes, flows;
	int64_t time;

	// First character of data lines is a date
	bool isHeader(char * line){
		return !isdigit(line[0]);
	}

	void parseLine(char *line) {
		// Split netflow line into tokens, space-delimited
		std::string str(line);
		std::istringstream iss(str);
		std::vector<std::string> tokens;
		std::copy(std::istream_iterator<std::string>(iss),
			 std::istream_iterator<std::string>(),
			 std::back_inserter<std::vector<std::string> >(tokens));

		// Get Source/Destination address and port
		int position = tokens[6].find(':');
		this->dest = tokens[6].substr(0, position);
		this->dest_port = tokens[6].substr(position, tokens[6].length() - position);
		position = tokens[4].find(':');
		this->src = tokens[4].substr(0, position);
		this->src_port = tokens[4].substr(position, tokens[4].length() - position);

		this->protocol = tokens[3];
		this->bytes = atoi(tokens[8].c_str());
		this->duration = atol(tokens[2].c_str());
		this->time = this->parseDatetime(tokens[0],tokens[1]);

	}

	uint64_t parseDatetime(std::string date, std::string time){
		uint64_t ret = 0;
		std::string intermediate = date.substr(0,4) + date.substr(5,2) + date.substr(8,2);
		intermediate += time.substr(0,2) + time.substr(3,2) + time.substr(6,2);
		ret = atoll(intermediate.c_str());
		/*
		std::cout << intermediate.c_str() << "\n";
		std::cout << ret << "\n";
		*/
		return ret;
	}

};
double dpc_tic(void);
double dpc_toc(double);

#endif /* _NETFLOW_STREAM_H */
