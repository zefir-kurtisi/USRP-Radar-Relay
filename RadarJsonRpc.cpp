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
#include <cstdio>
#include <cstring>

#include "Debug.hpp"
#include "RadarJsonRpc.hpp"
#include "json/json.h"

#define MAX_PULSES	10000

RadarJsonRpc::RadarJsonRpc()
{
	pulse_pattern.pulses = new struct pulse[MAX_PULSES];
}

RadarJsonRpc::~RadarJsonRpc(void)
{
	delete(pulse_pattern.pulses);
//	delete(request_buffer);
}

bool RadarJsonRpc::parse_pattern_request(struct json_object *j_root)
{
	float default_width;
	float default_ampl;
	struct json_object *j_pattern;
	struct json_object *j_duration;
	struct json_object *j_num_pulses;
	struct json_object *j_default_width;
	struct json_object *j_default_ampl;
	struct json_object *j_repeats;
	struct json_object *j_repeat_interval;
	struct json_object *j_pulses;
	struct pulse_pattern &pattern = pulse_pattern;
	request_cmd.cmd_id = CMD_PULSE_PATTERN;

	j_pattern = json_object_object_get(j_root, PULSE_PATTERN_CMD);
	j_duration = json_object_object_get(j_pattern, "duration");
	j_num_pulses = json_object_object_get(j_pattern, "num_pulses");
	j_default_width = json_object_object_get(j_pattern, "default_width");
	j_default_ampl = json_object_object_get(j_pattern, "default_ampl");
	j_repeats = json_object_object_get(j_pattern, "repeats");
	j_repeat_interval = json_object_object_get(j_pattern, "repeat_interval");

	pattern.duration = json_object_get_int(j_duration);
	pattern.num_pulses = json_object_get_int(j_num_pulses);
	default_width = (float)json_object_get_double(j_default_width);
	default_ampl = (float)json_object_get_double(j_default_ampl);
	pattern.repeats = json_object_get_int(j_repeats);
	pattern.repeat_interval = json_object_get_int(j_repeat_interval);

	json_object_put(j_duration);
	json_object_put(j_num_pulses);
	json_object_put(j_default_width);
	json_object_put(j_default_ampl);
	json_object_put(j_repeats);
	json_object_put(j_repeat_interval);

	j_pulses = json_object_object_get(j_pattern, "pulses");
	if (j_pulses == NULL) {
		DERR << "Failed to get pulses!" << std::endl;
		return false;
	}
	uint num_pulses = json_object_array_length(j_pulses);
	if (num_pulses != pattern.num_pulses) {
		DWARN	<< "Warning: adjusting num_pulses="
			<< pattern.num_pulses
			<< " to array_pulses=" << num_pulses << std::endl;
		pattern.num_pulses = num_pulses;
	}

	struct pulse *pp = pattern.pulses;
	for (uint i=0; i < num_pulses; i++) {
		json_object *j_pulse = json_object_array_get_idx(j_pulses, i);
		json_object *j_ts = json_object_object_get(j_pulse, "ts");
		if (j_ts == NULL) {
			json_object_put(j_pulse);
			DINFO	<< "ignoring empty ts for pulse "
				<< i << std::endl;
			pattern.num_pulses--;
			continue;
		}
		json_object *j_width = json_object_object_get(j_pulse, "width");
		json_object *j_ampl = json_object_object_get(j_pulse, "ampl");
		pp->ts = json_object_get_int(j_ts);
		if (j_width != NULL)
			pp->width = (float)json_object_get_double(j_width);
		else
			pp->width = default_width;

		if (j_ampl != NULL)
			pp->ampl = (float)json_object_get_double(j_ampl);
		else
			pp->ampl = default_ampl;

		pp++;
		json_object_put(j_pulse);
		json_object_put(j_ts);
		json_object_put(j_width);
		json_object_put(j_ampl);
	}
	json_object_put(j_pulses);
	json_object_put(j_pattern);

	if (pattern.num_pulses <= 0)
		return false;
	request_cmd.pattern = pattern;
	return true;
}

bool RadarJsonRpc::parse_config_request(struct json_object *j_root)
{
	struct json_object *j_config;
	struct json_object *j_freq;
	struct json_object *j_rate;
	struct json_object *j_gain;
	struct device_config &config = request_cmd.config;
	request_cmd.cmd_id = CMD_CONFIGURE;

	j_config = json_object_object_get(j_root, USRP_CONFIG_CMD);
	j_freq = json_object_object_get(j_config, "freq");
	j_rate = json_object_object_get(j_config, "rate");
	j_gain = json_object_object_get(j_config, "gain");

	config.freq = json_object_get_double(j_freq);
	config.rate = json_object_get_double(j_rate);
	config.gain = json_object_get_double(j_gain);

	json_object_put(j_gain);
	json_object_put(j_rate);
	json_object_put(j_freq);
	json_object_put(j_config);

	return true;
}

std::string& RadarJsonRpc::set_result(bool result, std::string info)
{
	request_buffer = "{ '"RESULT_RESPONSE"' : ";
	if (result)
		request_buffer += "true ";
	else
		request_buffer += "false ";
	if (info.size() > 0)
		request_buffer += ", '"RESULT_INFO"' : \"" + info + "\"";
	request_buffer += " }";
	return request_buffer;
}

static void log_json_warning(json_object *j_object)
{
	DWARN	<< "  JSON reports: "
		<< json_tokener_errors[-(unsigned long)j_object]
		<< std::endl;
}

bool RadarJsonRpc::get_result(std::string& request)
{
	bool result = false;
	struct json_object *j_root;
	j_root = json_tokener_parse(request.c_str());
	if(is_error(j_root)) {
		log_json_warning(j_root);
		return false;
	}
	struct json_object *j_result;
	j_result = json_object_object_get(j_root, RESULT_RESPONSE);
	if(is_error(j_result)) {
		log_json_warning(j_result);
		json_object_put(j_root);
		return false;
	}
	result = json_object_get_boolean(j_result);
	json_object_put(j_root);
	json_object_put(j_result);
	return result;
}

struct json_cmd *RadarJsonRpc::parse_request(struct json_object *j_root)
{
	struct lh_table* lh_root;
	struct json_cmd *retval = NULL;
	char *cmd;

	if(is_error(j_root)) {
		log_json_warning(j_root);
		return NULL;
	}
	lh_root = json_object_get_object(j_root);
	if (lh_root == NULL) {
		DERR << "get_request(): Invalid format" << std::endl;
		goto done;
	}
	cmd = (char*)lh_root->head->k;
	if (cmd == NULL) {
		DERR << "get_request(): Invalid format" << std::endl;
		goto done;
	} else if (strcmp(cmd, PULSE_PATTERN_CMD) == 0) {
		DLOG << "got pulse_pattern cmd" << std::endl;
		if (parse_pattern_request(j_root))
			retval = &request_cmd;
		goto done;

	} else if (strcmp(cmd, USRP_CONFIG_CMD) == 0) {
		DLOG << "got usrp_config" << std::endl;
		if (parse_config_request(j_root))
			retval = &request_cmd;
		goto done;

	} else {
		DWARN << "Unknown command: " << cmd << std::endl;
	}
done:
	json_object_put(j_root);
	return retval;
}

struct json_cmd *RadarJsonRpc::get_request(std::string& request)
{
	return parse_request(json_tokener_parse(request.c_str()));
}

struct json_cmd *RadarJsonRpc::get_request_from_file(const char *filename)
{
	return parse_request(json_object_from_file((char*)filename));
}
