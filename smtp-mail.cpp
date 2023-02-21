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
	payload->push_back("To: " + emailCfg->email_to + " \r\n");
	if (!emailCfg->email_cc.empty())
		payload->push_back("Cc: " + emailCfg->email_cc + " \r\n");
	payload->push_back("From: " + emailCfg->email_from + " \r\n");
	//payload->push_back("Message-ID: <" + emailCfg->messageId + ">\r\n");
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
	
    string email_from = emailCfg->email_from ;
    email_from = email_from.substr(email_from.find("<"));
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email_from.c_str());
	
    string email_to = emailCfg->email_to;
    string email_cc = emailCfg->email_cc;
    string email_bcc = emailCfg->email_bcc;

    email_to = email_to.substr(email_to.find("<"));
    email_cc = email_cc.substr(email_cc.find("<"));
    email_bcc = email_bcc.substr(email_bcc.find("<"));

    recipients = curl_slist_append(recipients, email_to.c_str());
	if (!email_cc.empty())
		recipients = curl_slist_append(recipients, email_cc.c_str());
	if(!email_bcc.empty())
		recipients = curl_slist_append(recipients, email_bcc.c_str());
    //recipients = curl_slist_append(recipients, CC_ADDR);
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

