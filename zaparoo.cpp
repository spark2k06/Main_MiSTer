// zaparoo.cpp
// 2024, Aitor Gomez Garcia (info@aitorgomez.net)

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "spi.h"
#include "user_io.h"
#include "lib/imlib2/Imlib2.h"
#include "video.h"
#include "cfg.h"

#define SCALE_DIM(dim, ref, target) ((dim) * (target) / (ref))

char* zaparoo = NULL;

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
    int sockfd;
    struct sockaddr_un addr;
    const char* socket_path = "/tmp/zaparoo/core.sock";
    const char* message = "connection";
    char buffer[1024] = {0};
    char *response;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        zaparoo = NULL;
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        zaparoo = NULL;
        return;
    }

    if (write(sockfd, message, strlen(message)) == -1) {
        perror("write");
        close(sockfd);
        zaparoo = NULL;
        return;
    }

    if (read(sockfd, buffer, sizeof(buffer)) == -1) {
        perror("read");
        close(sockfd);
        zaparoo = NULL;
        return;
    }

    close(sockfd);

    if (strncmp(buffer, "true,", 5) == 0) {
        response = (char*)malloc(strlen(buffer) - 4 + strlen("Zaparoo: "));
        if (response) {
            strcpy(response, "Zaparoo: ");
            strcat(response, buffer + 5);
        }
        zaparoo = response;
        return;
    } else {
        zaparoo = NULL;
        return;
    }
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