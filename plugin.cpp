/*
 * Fledge "email" notification plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora
 */

#include <string>
#include <plugin_api.h>
#include <config_category.h>
#include <logger.h>
#include <email_config.h>
#include <version.h>


#define PLUGIN_NAME "email"

static const char * def_cfg = QUOTE({
	"plugin" : { 
		"description" : "Email notification plugin",
		"type" : "string",
		"default" : PLUGIN_NAME,
		"readonly" : "true" },
	"email_to" : {
		"description" : "The address to send the alert to",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "1",
		"displayName" : "To address"
		},
	"email_to_name" : {
		"description" : "The name to send the alert to",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "2",
		"displayName" : "To "
		},
	"email_cc" : {
		"description" : "The address to send the alert to Cc",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "1",
		"displayName" : "Cc address"
		},
	"email_cc_name" : {
		"description" : "The name to send the alert to Cc",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "2",
		"displayName" : "Cc "
		},
	"email_bcc" : {
		"description" : "The address to send the alert to Bcc",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "1",
		"displayName" : "Bcc address"
		},
	"email_bcc_name" : {
		"description" : "The name to send the alert to Bcc",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "2",
		"displayName" : "Bcc "
		},	
	"subject" : {
		"description" : "The email subject",
		"type" : "string",
		"displayName" : "Subject",
		"order" : "3",
		"default" : "Fledge alert notification"
		},
	"email_from" : {
		"description" : "The address the email will come from",
		"type" : "string",
		"displayName" : "From address",
		"default" : "dianomic.alerts@gmail.com",
		"order" : "4"
		},
	"email_from_name" : {
		"description" : "The name used to send the alert email",
		"type" : "string",
		"default" : "Notification alert", 
		"displayName" : "From name",
		"order" : "5"
		},
	"server" : {
		"description" : "The SMTP server name/address",
		"type" : "string",
		"displayName" : "SMTP Server",
		"order" : "6",
		"default" : "smtp.gmail.com"
		},
	"port" : {
		"description" : "The SMTP server port",
		"type" : "integer",
		"displayName" : "SMTP Port",
		"order" : "7",
		"default" : "587"
		},
	"use_ssl_tls" : {
		"description" : "Use SSL/TLS for email transfer",
		"type" : "boolean",
		"displayName" : "SSL/TLS",
		"order" : "8",
		"default" : "true"
		},
	"username" : {
		"description" : "Email account name",
		"type" : "string",
		"displayName" : "Username",
		"order" : "9",
		"default" : "dianomic.alerts@gmail.com"
		},
	"password" : {
		"description" : "Email account password",
		"type" : "string",
		"displayName" : "Password",
		"order" : "10",
		"default" : "pass"
		},
	"enable": {
		"description": "A switch that can be used to enable or disable execution of the email notification plugin.",
		"type": "boolean",
		"displayName" : "Enabled",
		"default": "false", 
		"order" : "11" }
	});

using namespace std;
using namespace rapidjson;

/**
 * The Notification plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
        PLUGIN_NAME,              // Name
        VERSION,                  // Version
        0,                        // Flags
        PLUGIN_TYPE_NOTIFICATION_DELIVERY,       // Type
        "1.0.0",                  // Interface version
        def_cfg	          // Default plugin configuration
};

typedef struct
{
	EmailCfg emailCfg;
} PLUGIN_INFO;

extern int sendEmailMsg(const EmailCfg *emailCfg, const char *msg);
extern char *errorString(int result);

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Reset/init EmailCfg structure
 */
void resetConfig(EmailCfg *emailCfg)
{
	emailCfg->email_from = "";
	emailCfg->email_from_name = "";
	emailCfg->email_to = "";
	emailCfg->email_to_name = "";
	emailCfg->email_cc = "";
	emailCfg->email_cc_name = "";
	emailCfg->email_bcc = "";
	emailCfg->email_bcc_name = "";
	emailCfg->server = "";
	emailCfg->port = 0;
	emailCfg->subject = "";
	emailCfg->use_ssl_tls = false;
	emailCfg->username = "";
	emailCfg->password = "";
}

/**
 * Print EmailCfg structure
 */
void printConfig(EmailCfg *emailCfg)
{
	Logger::getLogger()->info("email_from=%s, email_from_name=%s, email_to=%s, email_to_name=%s",
						emailCfg->email_from.c_str(), emailCfg->email_from_name.c_str(), 
						emailCfg->email_to.c_str(), emailCfg->email_to_name.c_str());
	Logger::getLogger()->info("server=%s, port=%d, subject=%s, use_ssl_tls=%s, username=%s, password=%s",
						emailCfg->server.c_str(), emailCfg->port, emailCfg->subject.c_str(), 
						emailCfg->use_ssl_tls?"true":"false", emailCfg->username.c_str(), emailCfg->password.c_str());
}

/**
 * Fill EmailCfg structure from JSON document representing email server/account config
 */
void parseConfig(ConfigCategory *config, EmailCfg *emailCfg)
{
	if (config->itemExists("email_from"))
	{
		emailCfg->email_from = config->getValue("email_from");
	}
	if (config->itemExists("email_from_name"))
	{
		emailCfg->email_from_name = config->getValue("email_from_name");
	}
	if (config->itemExists("email_to"))
	{
		emailCfg->email_to = config->getValue("email_to");
	}
	if (config->itemExists("email_to_name"))
	{
		emailCfg->email_to_name = config->getValue("email_to_name");
	}
	if (config->itemExists("email_cc"))
	{
		emailCfg->email_cc = config->getValue("email_cc");
	}
	if (config->itemExists("email_cc_name"))
	{
		emailCfg->email_cc_name = config->getValue("email_cc_name");
	}
	if (config->itemExists("email_bcc"))
	{
		emailCfg->email_bcc = config->getValue("email_bcc");
	}
	if (config->itemExists("email_bcc_name"))
	{
		emailCfg->email_bcc_name = config->getValue("email_bcc_name");
	}
	if (config->itemExists("server"))
	{
		emailCfg->server = config->getValue("server");
	}
	if (config->itemExists("port"))
	{
		emailCfg->port = (unsigned int)atoi(config->getValue("port").c_str());
	}
	if (config->itemExists("subject"))
	{
		emailCfg->subject = config->getValue("subject");
	}
	if (config->itemExists("use_ssl_tls"))
	{
		emailCfg->use_ssl_tls = config->getValue("use_ssl_tls").compare("true") ? false : true;
	}
	if (config->itemExists("username"))
	{
		emailCfg->username = config->getValue("username");
	}
	if (config->itemExists("password"))
	{
		emailCfg->password = config->getValue("password");
	}
}

/**
 * Initialise the plugin, called to get the plugin handle and setup the
 * plugin configuration
 *
 * @param config	The configuration category for the plugin
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config)
{
	PLUGIN_INFO *info = new PLUGIN_INFO;

	// Handle plugin configuration
	if (config)
	{
		Logger::getLogger()->info("Email plugin config=%s", config->toJSON().c_str());

		resetConfig(&info->emailCfg);
		parseConfig(config, &info->emailCfg);
		printConfig(&info->emailCfg);
		
		if (info->emailCfg.email_to == "" || info->emailCfg.server == "" || info->emailCfg.port == 0)
		{
			Logger::getLogger()->error("Config for email notification plugin is incomplete, exiting...");
			delete info;
			info = NULL;
		}
	}
	else
	{
		Logger::getLogger()->fatal("No config provided for email plugin, exiting...");
		delete info;
		info = NULL;
	}
	
	return (PLUGIN_HANDLE)info;
}

/**
 * Deliver received notification data
 *
 * @param handle		The plugin handle returned from plugin_init
 * @param deliveryName		The delivery category name
 * @param notificationName	The notification name
 * @param triggerReason		The trigger reason for notification
 * @param message		The message from notification
 */
bool plugin_deliver(PLUGIN_HANDLE handle,
                    const std::string& deliveryName,
                    const std::string& notificationName,
                    const std::string& triggerReason,
                    const std::string& message)
{
	Logger::getLogger()->info("Email notification plugin_deliver(): deliveryName=%s, notificationName=%s, triggerReason=%s, message=%s",
							deliveryName.c_str(), notificationName.c_str(), triggerReason.c_str(), message.c_str());
	PLUGIN_INFO *info = (PLUGIN_INFO *) handle;
	
	int rv = sendEmailMsg(&info->emailCfg, message.c_str());
	if (rv)
	{
		Logger::getLogger()->error("Email notification failed: sendEmailMsg() returned %d, %s", rv, errorString(rv));
		return true;
	}
	else
	{
		Logger::getLogger()->info("sendEmailMsg() returned SUCCESS");
		return false;
	}
}

/**
 * Reconfigure the plugin
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, string& newConfig)
{
	Logger::getLogger()->info("Email notification plugin: plugin_reconfigure()");
	PLUGIN_INFO *info = (PLUGIN_INFO *) handle;
	ConfigCategory  config("new", newConfig); 
	Logger::getLogger()->info("Email plugin reconfig=%s", newConfig.c_str());

	parseConfig(&config, &info->emailCfg);
	
	if (info->emailCfg.email_to == "" || info->emailCfg.server == "" || info->emailCfg.port == 0)
	{
		Logger::getLogger()->error("New config for email notification plugin is incomplete");
		// How to indicate failure to caller?
		// Maybe keep a copy of previous emailCfg structure and restore it here in case of failure
	}
	
	return;
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	PLUGIN_INFO *info = (PLUGIN_INFO *) handle;
	delete info;
}

// End of extern "C"
};

