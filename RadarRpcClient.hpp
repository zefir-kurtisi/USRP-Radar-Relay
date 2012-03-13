#ifndef RADAR_RPC_CLIENT_HPP
#define RADAR_RPC_CLIENT_HPP

#include <string>

#include "RadarJsonRpc.hpp"
#include "Common.hpp"

class RadarRpcClient
{
public:
	RadarRpcClient(std::string server_ip = DEFAULT_RADAR_RPC_SERVER_IP,
		       ushort server_port = DEFAULT_RADAR_RPC_SERVER_PORT);
	~RadarRpcClient();
	bool execute_rpc(std::string& request);
	std::string last_error;

private:
	int rpc_socket;
	std::string server_ip;
	ushort server_port;
	RadarJsonRpc json_rpc;
	bool connect_server(void);
	bool send_request(std::string& request);
	bool get_response(void);
};

#endif /* RADAR_RPC_CLIENT_HPP */
