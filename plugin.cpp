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
#include <string_utils.h>
#include <regex>


#define PLUGIN_NAME "email"

static const char * def_cfg = QUOTE({
	"plugin" : { 
		"description" : "Email notification plugin",
		"type" : "string",
		"default" : PLUGIN_NAME,
		"readonly" : "true",
		"group" : "Headers" },
	"email_to" : {
		"description" : "The comma separated address list to send the alert to",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "1",
		"displayName" : "To address",
		"group" : "Headers"
		},
	"email_to_name" : {
		"description" : "The comma separated name list to send the alert to",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "2",
		"displayName" : "To name",
		"group" : "Headers"
		},
	"email_cc" : {
		"description" : "The comma separated address list to send the CC alert",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "3",
		"displayName" : "CC address",
		"group" : "Headers"
		},
	"email_cc_name" : {
		"description" : "The comma separated name list to send the CC alert",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "4",
		"displayName" : "CC name",
		"group" : "Headers"
		},
	"email_bcc" : {
		"description" : "The comma separated address list to send the BCC alert",
		"type" : "string",
		"default" : "alert.subscriber@dianomic.com",
		"order" : "5",
		"displayName" : "BCC address",
		"group" : "Headers"
		},
	"email_bcc_name" : {
		"description" : "The comma separated name list to send the BCC alert",
		"type" : "string",
		"default" : "Notification alert subscriber",
		"order" : "6",
		"displayName" : "BCC name",
		"group" : "Headers"
		},
	"email_from" : {
		"description" : "The address the email will come from",
		"type" : "string",
		"displayName" : "From address",
		"default" : "dianomic.alerts@gmail.com",
		"order" : "7",
		"group" : "Headers"
		},
	"email_from_name" : {
		"description" : "The name used to send the alert email",
		"type" : "string",
		"default" : "Notification alert", 
		"displayName" : "From name",
		"order" : "8",
		"group" : "Headers"
		},
	"subject" : {
		"description" : "The email subject. Macro $NOTIFICATION_INSTANCE_NAME$ can be used to provide information about notification instance name. Macro $REASON$ can be use to provide the reason for notification.",
		"type" : "string",
		"displayName" : "Subject",
		"order" : "9",
		"default" : "Fledge alert notification sent by $NOTIFICATION_INSTANCE_NAME$ $REASON$ ",
		"group" : "Message"
		},
	"email_body" : {
		"description" : "The email body",
		"type" : "string",
		"displayName" : "Body",
		"order" : "10",
		"default" : "Notification alert",
		"group" : "Message"
		},	
	"server" : {
		"description" : "The SMTP server name/address",
		"type" : "string",
		"displayName" : "SMTP Server",
		"order" : "11",
		"default" : "smtp.gmail.com",
		"group" : "Mail Server"
		},
	"port" : {
		"description" : "The SMTP server port",
		"type" : "integer",
		"displayName" : "SMTP Port",
		"order" : "12",
		"default" : "587",
		"group" : "Mail Server"
		},
	"use_ssl_tls" : {
		"description" : "Use SSL/TLS for email transfer",
		"type" : "boolean",
		"displayName" : "SSL/TLS",
		"order" : "13",
		"default" : "true",
		"group" : "Mail Server"
		},
	"username" : {
		"description" : "Email account name",
		"type" : "string",
		"displayName" : "Username",
		"order" : "14",
		"default" : "dianomic.alerts@gmail.com",
		"group" : "Mail Server"
		},
	"password" : {
		"description" : "Email account password",
		"type" : "password",
		"displayName" : "Password",
		"order" : "15",
		"default" : "pass",
		"group" : "Mail Server"
		},
	"enable": {
		"description": "A switch that can be used to enable or disable execution of the email notification plugin.",
		"type": "boolean",
		"displayName" : "Enabled",
		"default": "false", 
		"order" : "16",
		"group" : "Headers" }
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
	bool isConfigValid;
} PLUGIN_INFO;

bool isAddressNamePairMatch = true;
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
	emailCfg->email_body = "";
	emailCfg->server = "";
	emailCfg->port = 0;
	emailCfg->subject = "";
	emailCfg->use_ssl_tls = false;
	emailCfg->username = "";
	emailCfg->password = "";

	emailCfg->email_to_tokens.clear();
	emailCfg->email_to_name_tokens.clear();
	emailCfg->email_cc_tokens.clear();
	emailCfg->email_cc_name_tokens.clear();
	emailCfg->email_bcc_tokens.clear();
	emailCfg->email_bcc_name_tokens.clear();
}

/**
 * Print EmailCfg structure
 */
void printConfig(EmailCfg *emailCfg)
{
	Logger::getLogger()->info("email_from=%s,  email_to=%s",
						emailCfg->email_from.c_str(), 
						emailCfg->email_to.c_str());
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
	if (config->itemExists("email_body"))
	{
		emailCfg->email_body = config->getValue("email_body");
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
 * Tokenize sting
 */
std::vector<std::string> stringTokenize(std::string searchString ,std::regex searchPattern)
{
	std::vector<std::string> vec;
	auto token_start = std::sregex_iterator(searchString.begin(), searchString.end(), searchPattern);
	auto token_end = std::sregex_iterator();
	for (std::sregex_iterator i = token_start; i != token_end; ++i)
	{
		std::smatch match = *i;
		std::string matchValue = match.str();
		vec.emplace_back(matchValue);
	}
	return vec;

}

/**
 * Validate count of email address and name pairs 
 */
void validateConfig(PLUGIN_HANDLE *handle, EmailCfg *emailCfg)
{
	PLUGIN_INFO *info = (PLUGIN_INFO *) handle;
	// Check for complete configuration
	if (emailCfg->email_to == "" || emailCfg->email_from =="" || emailCfg->server == "" || emailCfg->port == 0)
	{
		info->isConfigValid;
		Logger::getLogger()->error("To address, From address SMTP Server , SMTP port can not be blank. Please provide complete configuration");
		return;
	}
	// Check if To address and To Names have same count
	std::regex searchPattern("[^\\,]+");
	
	//clear old data
	emailCfg->email_to_tokens.clear();
	emailCfg->email_to_name_tokens.clear();
	emailCfg->email_cc_tokens.clear();
	emailCfg->email_cc_name_tokens.clear();
	emailCfg->email_bcc_tokens.clear();
	emailCfg->email_bcc_name_tokens.clear();

	emailCfg->email_to_tokens =  stringTokenize(emailCfg->email_to,searchPattern);
	emailCfg->email_to_name_tokens =  stringTokenize(emailCfg->email_to_name,searchPattern);
	
	if ( emailCfg->email_to_tokens.size() != emailCfg->email_to_name_tokens.size() )
	{
		info->isConfigValid = false;
		Logger::getLogger()->error("There is a mismatch between To address and To name count.");
		return;
	}

	// Check if CC address and CC Names have same count
	emailCfg->email_cc_tokens =  stringTokenize(emailCfg->email_cc,searchPattern);
	emailCfg->email_cc_name_tokens =  stringTokenize(emailCfg->email_cc_name,searchPattern);
	
	if ( emailCfg->email_cc_tokens.size() != emailCfg->email_cc_name_tokens.size() )
	{
		info->isConfigValid = false;
		Logger::getLogger()->error("There is a mismatch between CC address and CC name count.");
		return;
	}
	
	// Check if BCC address and BCC Names have same count
	emailCfg->email_bcc_tokens =  stringTokenize(emailCfg->email_bcc,searchPattern);
	emailCfg->email_bcc_name_tokens =  stringTokenize(emailCfg->email_bcc_name,searchPattern);
	
	if ( emailCfg->email_bcc_tokens.size() != emailCfg->email_bcc_name_tokens.size() )
	{
		info->isConfigValid = false;
		Logger::getLogger()->error("There is a mismatch between BCC address and BCC names count.");
		return;
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
		info->isConfigValid = true;
		resetConfig(&info->emailCfg);
		parseConfig(config, &info->emailCfg);
		printConfig(&info->emailCfg);
		validateConfig((PLUGIN_HANDLE*)info,&info->emailCfg);
		
	}
	else
	{
		info->isConfigValid = false;
		Logger::getLogger()->fatal("No config provided for email plugin");
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
	StringReplace(info->emailCfg.subject, "$NOTIFICATION_INSTANCE_NAME$", notificationName);
	
	// Parse JSON triggerReason 
	Document doc;
	doc.Parse(triggerReason.c_str());
	if (doc.HasParseError())
	{
		Logger::getLogger()->error("Email notification delivery: failure parsing JSON trigger reason '%s'", triggerReason.c_str());
		return false;
	}

	string reason = doc["reason"].GetString();

	StringReplace(info->emailCfg.subject, "$REASON$", reason);


	int rv = 0;
	if (info->isConfigValid)
	{
		rv = sendEmailMsg(&info->emailCfg, info->emailCfg.email_body.c_str());
	}
	else
	{
		Logger::getLogger()->warn("Email delivery notification aborted due to mismatch in email Id and name count");
	}	

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
	validateConfig(handle,&info->emailCfg);
	
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

