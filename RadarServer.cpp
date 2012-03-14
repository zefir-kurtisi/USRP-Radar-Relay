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
#include <csignal>
#include <iostream>

#include "Debug.hpp"
#include "RadarJsonRpc.hpp"
#include "RadarRpcServer.hpp"
#include "UsrpRadarRelay.hpp"

static bool stop_signal_called;
void sig_int_handler(int)
{
	DINFO << "Signal received. Terminating..." << std::endl;
	stop_signal_called = true;
}

static void print_pattern(struct pulse_pattern *p)
{
	DLOG	<< "Got pattern with " << p->num_pulses << " pulses, "
		<< "dur=" << p->duration << ", rep=" << p->repeats << ", "
		<< "rep_int=" << p->repeat_interval << std::endl;
	for (uint i = 0; i < p->num_pulses; i++)
		DLOG	<< "\t ts=" << p->pulses[i].ts
			<< ",\t dur=" << p->pulses[i].width
			<< ",\t ampl=" << p->pulses[i].ampl << std::endl;
}

int main(int argc, char *argv[])
{
	std::signal(SIGINT, &sig_int_handler);
#if USE_CONTINUOUS_STREAMING
	UsrpRadarRelayContinuous usrp;
#else
	UsrpRadarRelayOnDemand usrp;
#endif
	usrp.setup_tx();
	RadarServer server;
	RadarJsonRpc json_rpc;
	while (!stop_signal_called) {
		std::string& request = server.get_request();
		if (request.length() > 0) {
			bool result = false;
			struct json_cmd *cmd = json_rpc.get_request(request);
			if (cmd == NULL) {
				DERR << "Got invalid request." << std::endl;
			} else if (cmd->cmd_id == CMD_PULSE_PATTERN) {
				struct pulse_pattern *pp = &cmd->pattern;
				DLOG << "Got valid pulse request" << std::endl;
				print_pattern(pp);
				result = usrp.set_pulse_pattern(pp);
			} else if (cmd->cmd_id == CMD_CONFIGURE) {
				DLOG << "Got valid config request" << std::endl;
				result = usrp.setup_tx(cmd->config.freq,
						cmd->config.rate,
						cmd->config.gain);
			}
			server.send_response(json_rpc.set_result(result));
		}
		if (!usrp.send_sequence())
			break;
	}
	return 0;
}
