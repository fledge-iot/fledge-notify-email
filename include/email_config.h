#ifndef _EMAIL_CONFIG_H
#define _EMAIL_CONFIG_H
/*
 * Fledge email notification plugin
 *
 * Copyright (c) 2018 OSIsoft, LLC
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora
 */

struct EmailCfg {
	std::string email_from;
	std::string email_from_name;
	std::vector<std::string> email_to;
	std::vector<std::string> email_to_name;
	std::vector<std::string> email_cc;
	std::vector<std::string> email_cc_name;
	std::vector<std::string> email_bcc;
	std::vector<std::string> email_bcc_name;
	std::string email_body;
	std::string server;
	unsigned int port;
	std::string subject;
	bool use_ssl_tls;
	std::string username; // required only in case of SSL/TLS
	std::string password; // required only in case of SSL/TLS
};

#endif

