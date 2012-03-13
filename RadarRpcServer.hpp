#ifndef RADAR_RPC_SERVER_H
#define RADAR_RPC_SERVER_H

#include <string>
#include "Common.hpp"

class RadarServer {
public:
	RadarServer(unsigned short port = DEFAULT_RADAR_RPC_SERVER_PORT);
	~RadarServer();

	// get zero-terminated request string
	std::string& get_request(void);
	// send zero-terminated response string
	bool send_response(std::string& resp);

private:
	bool start_listening(void);
	bool accept_client(void);
	void finalize_client(void);
	void finalize_server(void);

	bool send_data(std::string& data);
	int sock_getcmd(void);

	std::string transmit_buffer;
	int sock_listen;
	int sock_client;
	short port_listen;
};

#endif /* RADAR_RPC_SERVER_H */
