// zaparoo.cpp
// 2025, Aitor Gomez Garcia (info@aitorgomez.net)

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <strings.h>

#include "spi.h"
#include "user_io.h"
#include "lib/imlib2/Imlib2.h"
#include "video.h"
#include "cfg.h"

#define SCALE_DIM(dim, ref, target) ((dim) * (target) / (ref))

char* zaparoo = NULL;
static const char *zaparoo_host = "127.0.0.1";
static const int zaparoo_port = 7497;

static int parse_content_length(const char *buf, int header_len)
{
    const char *p = buf;
    const char *end = buf + header_len;
    while (p < end) {
        const char *line_end = strstr(p, "\r\n");
        if (!line_end) break;
        int line_len = line_end - p;
        if (line_len > 0) {
            if (strncasecmp(p, "Content-Length:", 15) == 0) {
                return atoi(p + 15);
            }
        }
        p = line_end + 2;
    }
    return -1;
}

// Minimal blocking HTTP POST with short timeouts; writes body into resp (NUL-terminated).
static bool zaparoo_http_post(const char *payload, char *resp, size_t resp_sz)
{
    if (!payload || !resp || resp_sz < 2) return false;

    char headers[256];
    int payload_len = strlen(payload);
    int hdr_len = snprintf(headers, sizeof(headers),
        "POST /api/v0.1 HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        zaparoo_host, zaparoo_port, payload_len);
    if (hdr_len <= 0 || hdr_len >= (int)sizeof(headers)) return false;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return false;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(zaparoo_port);
    if (inet_pton(AF_INET, zaparoo_host, &addr.sin_addr) != 1) {
        close(sockfd);
        return false;
    }
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return false;
    }

    if (write(sockfd, headers, hdr_len) != hdr_len ||
        write(sockfd, payload, payload_len) != payload_len) {
        close(sockfd);
        return false;
    }

    char buf[4096];
    int total = 0;
    int content_len = -1;
    int header_len = -1;

    while (total < (int)sizeof(buf) - 1) {
        int n = read(sockfd, buf + total, (int)sizeof(buf) - 1 - total);
        if (n <= 0) break;
        total += n;

        if (header_len == -1) {
            char *hdr_end = strstr(buf, "\r\n\r\n");
            if (hdr_end) {
                header_len = (int)(hdr_end + 4 - buf);
                content_len = parse_content_length(buf, header_len);
            }
        }

        if (header_len != -1 && content_len >= 0) {
            int body_bytes = total - header_len;
            if (body_bytes >= content_len) break;
        }
    }
    close(sockfd);

    if (total <= 0) return false;
    buf[total] = '\0';

    char *body = strstr(buf, "\r\n\r\n");
    if (!body) return false;
    body += 4;
    strncpy(resp, body, resp_sz - 1);
    resp[resp_sz - 1] = '\0';
    return true;
}

Imlib_Image load_zaparoo_img(const char* s)
{
    char fname[256];
    snprintf(fname, sizeof(fname), "zaparoo_%s.png", s);

	if (!FileExists(fname)) fname[0] = '\0';
	    
	if (fname[0] != '\0')
	{
		Imlib_Load_Error error = IMLIB_LOAD_ERROR_NONE;
		Imlib_Image img = imlib_load_image_with_error_return(getFullPath(fname), &error);
		if (img) 
        {            
            imlib_context_set_image(img);
            int cover_w = imlib_image_get_width();
            int cover_h = imlib_image_get_height();
            if (cover_w != 512 || cover_h != 384) {
                printf("Zaparoo image dimensions are not as expected\n");
                return NULL;
            } else return img;            
        }        
		printf("Image %s loading error %d\n", fname, error);
	}

	return NULL;

}

Imlib_Image load_waiting_img()
{
	const char* fname = "waiting.png";
	if (!FileExists(fname)) fname = 0;
	    
	if (fname)
	{
		Imlib_Load_Error error = IMLIB_LOAD_ERROR_NONE;
		Imlib_Image img = imlib_load_image_with_error_return(getFullPath(fname), &error);
		if (img) 
        {            
            imlib_context_set_image(img);
            int cover_w = imlib_image_get_width();
            int cover_h = imlib_image_get_height();
            if (cover_w != 512 || cover_h != 64) {
                printf("Waiting image dimensions are not as expected\n");
                return NULL;
            } else return img;            
        }        
		printf("Image %s loading error %d\n", fname, error);
	}

	return NULL;

}

void getZaparoo() {    
    // New Zaparoo Core API (HTTP/JSON-RPC on localhost:7497)
    // Query readers first; only show icon/text when a reader is connected.
    const char *payload_version  = "{\"jsonrpc\":\"2.0\",\"id\":\"00000000-0000-0000-0000-000000000123\",\"method\":\"version\"}";
    char response[4096];
    char version[64] = {0};
    char info[128] = {0};
    bool reader_connected = false;

    if (zaparoo) { free(zaparoo); zaparoo = NULL; }

    // Helper to pull a string value from JSON without full parsing.
    auto extract = [](const char *json, const char *key, char *out, size_t out_sz)->bool {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
        const char *p = strstr(json, pattern);
        if (!p) return false;
        p += strlen(pattern);
        const char *end = strchr(p, '"');
        if (!end || end <= p) return false;
        size_t len = (size_t)(end - p);
        if (len >= out_sz) len = out_sz - 1;
        strncpy(out, p, len);
        out[len] = '\0';
        return true;
    };

    // First, check readers
    const char *payload_readers = "{\"jsonrpc\":\"2.0\",\"id\":\"00000000-0000-0000-0000-000000000124\",\"method\":\"readers\"}";
    if (zaparoo_http_post(payload_readers, response, sizeof(response))) {
        if (strstr(response, "\"connected\":true")) {
            reader_connected = true;
            extract(response, "info", info, sizeof(info));
        }
    }

    if (!reader_connected) {
        zaparoo = NULL;
        return;
    }

    // Try to get version to display a nicer label
    zaparoo_http_post(payload_version, response, sizeof(response));
    extract(response, "version", version, sizeof(version));

    const char *label = info[0] ? info : (version[0] ? version : "reader connected");
    size_t len = strlen("Zaparoo: ") + strlen(label) + 1;
    zaparoo = (char*)malloc(len);
    if (zaparoo) snprintf(zaparoo, len, "Zaparoo: %s", label);
}

bool zaparoo_waiting(int menu_bgn, Imlib_Image bg1, Imlib_Image bg2)
{    
    Imlib_Image *bg = (menu_bgn == 1) ? &bg1 : &bg2;    
    static Imlib_Image zaparoo_img_1 = 0;
    static Imlib_Image zaparoo_img_2 = 0;
    static Imlib_Image waiting_img = 0;
    static bool zaparoo_state = false;

    static int scale_x_512 = SCALE_DIM(512, 1366, fb_width);
    static int scale_y_384 = SCALE_DIM(384, 768, fb_height);
    static int scale_y_64 = SCALE_DIM(64, 768, fb_height);

    if (zaparoo && !zaparoo_img_1) zaparoo_img_1 = load_zaparoo_img("1");
    if (zaparoo && !zaparoo_img_2) zaparoo_img_2 = load_zaparoo_img("2");
    if (!waiting_img) waiting_img = load_waiting_img();
    imlib_context_set_image(*bg);

    zaparoo_state = !zaparoo_state;
    if (zaparoo && zaparoo_img_1 && zaparoo_img_2)
    {					
        int zaparoo_x = (fb_width - scale_x_512) / 2;
        int zaparoo_y = (fb_height - scale_y_384) / (cfg.waiting_txt_up ? 1.5 : 2);
        imlib_blend_image_onto_image(zaparoo_state ? zaparoo_img_1 : zaparoo_img_2, 1,
            0, 0,
            512, 384,
            zaparoo_x, zaparoo_y,
            scale_x_512, scale_y_384
        );
        if (waiting_img)
        {
            int waiting_x = (fb_width - scale_x_512) / 2;
            int waiting_y = (fb_height - scale_y_64) / (cfg.waiting_txt_up ? 4 : 1.27);
            imlib_blend_image_onto_image(waiting_img, 1,
                0, 0,
                512, 64,
                waiting_x, waiting_y,
                scale_x_512, scale_y_64
            );
        }
        return true;
    }
    else
    {
        return false;
    }
}
