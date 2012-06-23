/*
 * Copyright (c) 2012 Neratec Solutions AG
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "Debug.hpp"
#include "RadarRpcClient.hpp"

RadarRpcClient::RadarRpcClient(std::string ip, ushort port)
{
	server_ip = ip;
	server_port = port;
	rpc_socket = -1;
	DLOG	<< "RadarRpcClient " << server_ip << ":"
		<< server_port << std::endl;
}

RadarRpcClient::~RadarRpcClient()
{
	shutdown(rpc_socket, SHUT_RDWR);
	close(rpc_socket);
}

bool RadarRpcClient::connect_server(void)
{
	int res;
	struct sockaddr_in addr;

	// Create socket
	rpc_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (rpc_socket < 0) {
		last_error = "Error creating socket: ";
		last_error += strerror(errno);
		return false;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

	res = connect(rpc_socket, (struct sockaddr *) &addr, sizeof(addr));
	if (res < 0) {
		close(rpc_socket);
		last_error = "Error connecting: ";
		last_error += strerror(errno);
		return false;
	}
	return true;
}

bool RadarRpcClient::send_request(std::string& request)
{
	size_t sent = 0;
	size_t size = request.size() + 1;
	const char *buff = request.data();
	while (sent < size) {
		int wrote = write(rpc_socket, buff + sent, size - sent);
		if (wrote < 0) {
			last_error = "Failed to write data to socket";
			return false;
		}
		sent += wrote;
	}
	return true;
}

#define MAX_RESPONSE_SIZE 512
bool RadarRpcClient::get_response(void)
{
	int rx_bytes = 0;
	std::string result;
	char response[MAX_RESPONSE_SIZE + 1];
	*response = 0;
	do {
		int received = recv(rpc_socket, response + rx_bytes,
				MAX_RESPONSE_SIZE - rx_bytes, 0);
		if (received < 0)
			return false;
		rx_bytes += received;
	} while (response[rx_bytes - 1] != 0);
	result = response;
	DLOG << "Received " << rx_bytes << " bytes: " << response << std::endl;
	bool success = json_rpc.get_result(result);
	if (!success)
		last_error = "RPC error: " + result;
	return success;
}

bool RadarRpcClient::execute_rpc(std::string& request)
{
	/* first check that we have a valid request to send */
	if (json_rpc.get_request(request) == NULL) {
		last_error = "Invalid RPC command format.";
		return false;
	}
	return connect_server() && send_request(request) && get_response();
}

