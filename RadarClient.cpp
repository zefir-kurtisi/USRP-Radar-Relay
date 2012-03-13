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

#include "Debug.hpp"
#include "RadarRpcClient.hpp"

#include <string>
#include <fstream>
#include <iostream>

#define DEFAULT_SERVER_PORT	15432
#define DEFAULT_SERVER_IP	"127.0.0.1"

std::string read_file(const char *fname)
{
	std::string fstring;
	std::ifstream infile(fname);
	if (infile.is_open()) {
		char c;
		while (!infile.eof()) {
			c = infile.get();
			fstring.push_back(c);
		}
		infile.close();
		DLOG << "Read bytes from '" << fname << "' "
				<< fstring.length() << std::endl;
	} else
		DERR << "Could not open file " << fname << std::endl;

	return fstring;
}

int main(int argc, char **argv)
{
	int retval = -1;
	const char *ip_addr = DEFAULT_SERVER_IP;
	ushort port = DEFAULT_SERVER_PORT;
	if (argc != 2) {
		DERR << "usage: " << argv[0] << " <JSON-file>\n" << std::endl;
		exit(-1);
	}
	std::string request_string = read_file(argv[1]);
	if (request_string.length() <= 1)
		return -2;
	RadarRpcClient rc(ip_addr, port);
	if (rc.execute_rpc(request_string)) {
		DLOG << "Request successfully handled." << std::endl;
		retval = 0;
	} else {
		DERR << "Request failed: " << rc.last_error << std::endl;
	}
	return retval;
}
