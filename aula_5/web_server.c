#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "web_server.h"
#include "html_support.h"

const char* TAG = "web_server.c";

static const char *HTML_FORM =
"<html><body><h1>WEB Form Demo using ESP-IDF</h1>"
"<form action=\"/\" method=\"post\">"
"<label for=\"ssid\">Local SSID:</label><br>"
"<input type=\"text\" id=\"ssid\" name=\"ssid\"><br>"
"<label for=\"pwd\">Password:</label><br>"
"<input type=\"text\" id=\"pwd\" name=\"pwd\"><br>"
"<input type=\"submit\" value=\"Submit\">"
"</form></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t root_post_handler(httpd_req_t *req);
static esp_err_t handle_http_get(httpd_req_t *req);
static esp_err_t handle_http_post(httpd_req_t *req);

/* HTTP get handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "root_get_handler req->uri=[%s]", req->uri);
	esp_err_t err;

	// Send HTML header
	httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html>");
	Text2Html(req, "/html/head.html");

	httpd_resp_sendstr_chunk(req, "<body>");
	httpd_resp_sendstr_chunk(req, "<h1>Exercicio Dilsão da massa </h1>");

	httpd_resp_sendstr_chunk(req, "<h2> INSIRA O LOGIN DA REDE </h2>");
	httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/post\">");
	httpd_resp_sendstr_chunk(req, "SSID: <input type=\"text\" name=\"ssid\" value=\"");
	httpd_resp_sendstr_chunk(req, "\">");

	httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/post\">");
	httpd_resp_sendstr_chunk(req, "PASSWORD: <input type=\"text\" name=\"password\" value=\"");
	httpd_resp_sendstr_chunk(req, "\">");
	httpd_resp_sendstr_chunk(req, "<br> <br>");

	
	httpd_resp_sendstr_chunk(req, "<input type=\"submit\" value=\"Submit\">");
	httpd_resp_sendstr_chunk(req, "</form><br>");

	httpd_resp_sendstr_chunk(req, "<hr>");	

    // httpd_resp_sendstr_chunk(req, "<hr>");


	// httpd_resp_sendstr_chunk(req, "<h2>input number</h2>");
	// httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/post\">");
	// httpd_resp_sendstr_chunk(req, "number1(2 Digit): <input type=\"number\" class=\"dig2\" name=\"number1\" value=\"");

	// httpd_resp_sendstr_chunk(req, "\">");
	// httpd_resp_sendstr_chunk(req, "<br>");
	// httpd_resp_sendstr_chunk(req, "number2(4 Digit): <input type=\"number\" class=\"dig4\" name=\"number2\" value=\"");

	// httpd_resp_sendstr_chunk(req, "\">");
	// httpd_resp_sendstr_chunk(req, "<br>");
	// httpd_resp_sendstr_chunk(req, "number3(6 Digit): <input type=\"number\" class=\"dig6\" name=\"number3\" value=\"");

	// httpd_resp_sendstr_chunk(req, "\">");
	// httpd_resp_sendstr_chunk(req, "<br>");
	// httpd_resp_sendstr_chunk(req, "<input type=\"submit\" value=\"submit2\">");
	// httpd_resp_sendstr_chunk(req, "</form><br>");

	// httpd_resp_sendstr_chunk(req, "<hr>");


	// httpd_resp_sendstr_chunk(req, "<h2>input checkbox</h2>");
	// httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/post\">");

    // httpd_resp_sendstr_chunk(req, "<input type=\"checkbox\" name=\"checkred\">RED");
    // httpd_resp_sendstr_chunk(req, "<input type=\"checkbox\" name=\"checkgreen\">GREEN");
    // httpd_resp_sendstr_chunk(req, "<input type=\"checkbox\" name=\"checkblue\">BLUE");

	// httpd_resp_sendstr_chunk(req, "<br>");
	// httpd_resp_sendstr_chunk(req, "<input type=\"submit\" value=\"submit3\">");

	httpd_resp_sendstr_chunk(req, "</form><br>");


	/* Send Image to HTML file */
	//Image2Html(req, "/html/ESP-IDF.txt", "png");
	Image2Html(req, "/html/padolabs.txt", "png");

	/* Send remaining chunk of HTML file to complete it */
	httpd_resp_sendstr_chunk(req, "</body></html>");

	/* Send empty chunk to signal HTTP response completion */
	httpd_resp_sendstr_chunk(req, NULL);

	return ESP_OK;
}

/* HTTP post handler */
static esp_err_t root_post_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "root_post_handler req->uri=[%s]", req->uri);
	ESP_LOGI(TAG, "root_post_handler content length %d", req->content_len);
	
    if(req->content_len > 0) 
    {
        char*  buf = malloc(req->content_len + 1);
        size_t off = 0;
        while (off < req->content_len) {
            /* Read data received in the request */
            int ret = httpd_req_recv(req, buf + off, req->content_len - off);
            if (ret <= 0) {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                    httpd_resp_send_408(req);
                }
                free (buf);
                return ESP_FAIL;
            }
            off += ret;
            ESP_LOGI(TAG, "root_post_handler recv length %d", ret);
        }
        buf[off] = '\0';
        ESP_LOGI(TAG, "root_post_handler buf=[%s]", buf);

        ESP_LOGI(TAG, "%.*s", off, buf);
        post_messages_t post_message;
        post_message.lenght = off;
        strncpy(post_message.message, buf, (off < 128)? (off): 128);

        if (xQueueSend(xQueueHttp, &post_message, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "xQueueSend Fail");
        }
        free(buf);
    }
    
	/* Redirect onto root to see the updated file list */
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
	httpd_resp_set_hdr(req, "Connection", "close");
#endif
	httpd_resp_sendstr(req, "post successfully");
	return ESP_OK;
}
/* favicon get handler */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon_get_handler req->uri=[%s]", req->uri);
	return ESP_OK;
}

void start_webserver(void)
{    
    httpd_uri_t _root_get_handler = {
    .uri		 = "/",
    .method		 = HTTP_GET,
    .handler	 = root_get_handler,
    //.user_ctx  = server_data	// Pass server data as context
	};

    /* URI handler for post */
	httpd_uri_t _root_post_handler = {
		.uri		 = "/post",
		.method		 = HTTP_POST,
		.handler	 = root_post_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};

    	/* URI handler for favicon.ico */
	httpd_uri_t _favicon_get_handler = {
		.uri		 = "/favicon.ico",
		.method		 = HTTP_GET,
		.handler	 = favicon_get_handler,
		//.user_ctx  = server_data	// Pass server data as context
	};

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &_root_get_handler);
        httpd_register_uri_handler(server, &_root_post_handler);
        httpd_register_uri_handler(server, &_favicon_get_handler);
    }
}

static esp_err_t handle_http_get(httpd_req_t *req)
{
 return httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_http_post(httpd_req_t *req)
{
    char content[100];
    if (httpd_req_recv(req, content, req->content_len) <= 0)
    {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "%.*s", req->content_len, content);
    return httpd_resp_send(req, "received", HTTPD_RESP_USE_STRLEN);
}
