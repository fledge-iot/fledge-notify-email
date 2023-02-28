/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include <iostream>
#include <cstring>
#include <vector>
#include <ctime>
#include <curl/curl.h>
#include <email_config.h>
#include <logger.h>
#include "string_utils.h"

using namespace std;

extern "C" {
	
struct upload_status {
  int lines_read;
  vector<std::string>* payload;
};

char *getCurrTime()
{
	time_t rawtime;
	struct tm * timeinfo;
	static char buffer[80];

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (buffer, 80, "%a, %e %G %X %z", timeinfo);
	return buffer;
}


void compose_payload(vector<std::string>* &payload, const EmailCfg *emailCfg, const char* msg)
{
	payload->push_back("Date: " + string(getCurrTime()) + "\r\n");
	
	// Parse address and name to compose CC pairs for payload
	
	if (emailCfg->email_to.size())
	{
		std::string ToList = {"To: "};
		for(int i = 0; i < emailCfg->email_to.size(); i++ )
		{
			if (i > 0)
			{
				ToList.append(",");
			}
			ToList.append(emailCfg->email_to_name[i] + " <" + emailCfg->email_to[i] + ">");
		}
		ToList.append(" \r\n");
		payload->push_back(ToList);
		
	}
	
	if (emailCfg->email_cc.size())
	{
		std::string CCList = {"CC: "};
		for(int i = 0; i < emailCfg->email_cc.size(); i++ )
		{
			if (i > 0)
			{
				CCList.append(",");
			}
			CCList.append(emailCfg->email_cc_name[i] + " <" + emailCfg->email_cc[i] + ">");
		}
		CCList.append(" \r\n");
		payload->push_back(CCList);
		
	}
	
	// Do not add BCC payload otherwise it will be visible to all the recipients
	
	payload->push_back("From: " + emailCfg->email_from_name + " <" + emailCfg->email_from + "> \r\n");
	payload->push_back("Subject: " + emailCfg->subject + "\r\n");
	payload->push_back("\r\n");
	payload->push_back(msg);
	payload->push_back("\r\n");
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;

	if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
	return 0;
	}

	if (upload_ctx->lines_read >= upload_ctx->payload->size())
		return 0;

	std::string s = (*upload_ctx->payload)[upload_ctx->lines_read];

	size_t len = s.length();
	memcpy(ptr, s.c_str(), len);
	upload_ctx->payload->erase(upload_ctx->payload->begin()+upload_ctx->lines_read);

	return len;
}

int sendEmailMsg(const EmailCfg *emailCfg, const char *msg)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;

  upload_ctx.lines_read = 0;
  
  upload_ctx.payload = new vector<std::string>;
  compose_payload(upload_ctx.payload, emailCfg, msg);

  curl = curl_easy_init();
  if(curl) {

	if(emailCfg->use_ssl_tls)
	{
		/* Set username and password */
		curl_easy_setopt(curl, CURLOPT_USERNAME, emailCfg->username.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, emailCfg->password.c_str());
	}
	
    /* This is the URL for your mailserver */
	string proto = "";
	if (emailCfg->server.find("smtp://") == std::string::npos) proto = "smtp://";
	string server_url = proto + emailCfg->server + ":" + to_string(emailCfg->port);
    curl_easy_setopt(curl, CURLOPT_URL, server_url.c_str());

	/* We'll start with a plain text connection, and upgrade     
	 * to Transport Layer Security (TLS) using the STARTTLS command. */
	if(emailCfg->use_ssl_tls)
	{
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);   
		//curl_easy_setopt(curl, CURLOPT_CAINFO, "/path/to/certificate.pem");
	 }
	
    string email_from = "<" + StringStripWhiteSpacesAll(emailCfg->email_from) + ">";
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email_from.c_str());
	
    // Parse CC and BCC list to append recipients
	
	if (emailCfg->email_to.size())
	{
		for(auto it =  emailCfg->email_to.begin(); it != emailCfg->email_to.end(); it++ )
		{
			string email_to = "<" + *it + ">";
			recipients = curl_slist_append(recipients, email_to.c_str() );
		}
	}

	if (emailCfg->email_cc.size())
	{
		for(auto it =  emailCfg->email_cc.begin(); it != emailCfg->email_cc.end(); it++ )
		{
			string email_cc = "<" + *it + ">";
			recipients = curl_slist_append(recipients, email_cc.c_str() );
		}
	}

	if (emailCfg->email_bcc.size())
	{
		for(auto it =  emailCfg->email_bcc.begin(); it != emailCfg->email_bcc.end(); it++ )
		{
			string email_bcc = "<" + *it + ">";
			recipients = curl_slist_append(recipients, email_bcc.c_str() );
		}
	}

    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* Send the message */
    res = curl_easy_perform(curl);

    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* Free the list of recipients */
    curl_slist_free_all(recipients);
	
    curl_easy_cleanup(curl);
  }

  delete upload_ctx.payload;
  return (int)res;
}

const char *errorString(int result)
{
	return curl_easy_strerror((CURLcode)result);
}
};

