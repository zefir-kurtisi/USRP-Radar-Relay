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
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#include "Debug.hpp"
#include "RadarRpcServer.hpp"

// read request on client_socket terminated by '\0'
bool RadarServer::sock_getcmd(void)
{
	char c;
	transmit_buffer.clear();
	do {
		if (read(sock_client, &c, 1) <= 0)
			return false;
		transmit_buffer.push_back(c);
	} while (c);
	return (transmit_buffer.length() > 0);
}

// prepare listener and start listening
bool RadarServer::start_listening(void)
{
	struct sockaddr_in serv_addr;
	int reuse_addr = 1;

	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_listen < 0) {
		DERR << "socket error" << std::endl;
		return false;
	}

	// enable re-bind without time_wait problems
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
		   sizeof(reuse_addr));

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port_listen);
	int result = bind(sock_listen, (struct sockaddr *) &serv_addr,
			  sizeof(serv_addr));
	if (result < 0) {
		DERR << "unable to bind to port " << port_listen << std::endl;
		finalize_server();
		return false;
	}

	if (listen(sock_listen, 5) < 0) {
		DERR << "unable to listen" << std::endl;
		finalize_server();
		return false;
	}
	return true;
}

RadarServer::RadarServer(unsigned short port)
{
	sock_listen = -1;
	port_listen = port;
}

RadarServer::~RadarServer(void)
{
	finalize_server();
}

void RadarServer::finalize_server(void)
{
	close(sock_listen);
	sock_listen = -1;
}

void RadarServer::finalize_client(void)
{
	shutdown(sock_client, SHUT_RDWR);
	close(sock_client);
	sock_client = -1;
}

bool RadarServer::accept_client(void)
{
	fd_set myset;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&myset);
	FD_SET(sock_listen, &myset);
	if (select(sock_listen + 1, &myset, 0, 0, &tv) < 1)
		return false;

	sock_client = accept(sock_listen, 0, 0);
	if (sock_client < 0) {
		finalize_server();
		return false;
	}
	return true;
}

std::string& RadarServer::get_request(void)
{
	transmit_buffer.clear();
	if (sock_client > 0)
		finalize_client();
	if ((sock_listen < 0) && !start_listening()) {
		DERR	<< "listener socket invalid, "
			<< "restarting in some seconds..." << std::endl;
		sleep(5);
	} else if (accept_client()) {
		if (sock_getcmd())
			shutdown(sock_client, SHUT_RD);
		else
			finalize_client();
	}
	return transmit_buffer;
}

bool RadarServer::send_data(std::string& data)
{
	int sent = 0;
	int size = data.size() + 1;
	while (sent < size) {
		int wrote = write(sock_client, data.data() + sent, size - sent);
		if (wrote < 0)
			return false;
		sent += wrote;
	}
	return true;
}

bool RadarServer::send_response(std::string& response)
{
	if (!send_data(response)) {
		finalize_client();
		return false;
	}
	return true;
}
