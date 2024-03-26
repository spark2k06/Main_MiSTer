// loadscreen.cpp
// 2024, Aitor Gomez Garcia (info@aitorgomez.net)

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "lib/imlib2/Imlib2.h"
#include "video.h"
#include "menu.h"
#include "cfg.h"

#define FB_SIZE  (1920*1080)


Imlib_Image load_screen_bg()
{
	const char* fname = "loader.png";
	if (!FileExists(fname))
	{
		fname = "loader.jpg";
		if (!FileExists(fname)) fname = 0;
	}
    
	if (fname)
	{
		Imlib_Load_Error error = IMLIB_LOAD_ERROR_NONE;
		Imlib_Image img = imlib_load_image_with_error_return(getFullPath(fname), &error);
		if (img) 
        {
            loader_bg = 0;
            return img;
        }        
		printf("Image %s loading error %d\n", fname, error);
	}

    loader_bg = -1;
	return NULL;

}

Imlib_Image load_loading_txt()
{
	const char* fname = "loading.png";
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
            if (cover_w != 448 || cover_h != 64) {
                printf("Loading image dimensions are not as expected\n");
                return NULL;
            } else return img;            
        }        
		printf("Image %s loading error %d\n", fname, error);
	}

	return NULL;

}

Imlib_Image load_cover_img(const char *s, char *coreDir) {
    Imlib_Load_Error error;
    static char full_path[1024];

    const char *lastDot = strrchr(s, '.');
    if (!lastDot || lastDot == s) return NULL;

    const char *lastSlash = strrchr(s, '/');
    if (!lastSlash) return NULL;

    bool isPng = strncmp(lastDot, ".png", 4) == 0;
    int filenameLength = isPng ? strlen(lastSlash + 1) : lastDot - lastSlash - 1;

    if (coreDir && coreDir[0] != '\0') {
        if (isPng) {
            snprintf(full_path, sizeof(full_path), "%s/%s/%s/%s", getRootDir(), COVERS_DIR, coreDir, lastSlash + 1);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s/%s/%.*s.png", getRootDir(), COVERS_DIR, coreDir, filenameLength, lastSlash + 1);
        }
    } else {
        if (isPng) {
            snprintf(full_path, sizeof(full_path), "%s/%s/%s", getRootDir(), COVERS_DIR, lastSlash + 1);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s/%.*s.png", getRootDir(), COVERS_DIR, filenameLength, lastSlash + 1);
        }
    }

    printf("cover: %s\n", full_path);
    Imlib_Image img = NULL;
    if (FileExists(full_path)) {
        img = imlib_load_image_with_error_return(full_path, &error);
        if (!img) {
            printf("Image %s loading error %d\n", full_path, error);
            return NULL;
        } else {
            imlib_context_set_image(img);
            int cover_w = imlib_image_get_width();
            int cover_h = imlib_image_get_height();
            if (cover_w != 256 || cover_h != 320) {
                printf("Cover image dimensions are not as expected\n");
                return NULL;
            }
        }
    }
    return img;
}

void fade_in_screen(const char *s, char *coreDir) {
    static Imlib_Image screen_bg = 0;
    static Imlib_Image cover_img = 0;
    static Imlib_Image loading_txt = 0;
    if (!screen_bg) screen_bg = load_screen_bg();
    if (!cover_img) cover_img = load_cover_img(s, coreDir);
    if (!loading_txt) loading_txt = load_loading_txt();

    if (screen_bg) {
        static Imlib_Image bg1 = 0;
        static Imlib_Image bg2 = 0;
        if (!bg1) bg1 = imlib_create_image_using_data(fb_width, fb_height, (uint32_t*)(fb_base + (FB_SIZE * 1)));
        if (!bg1) printf("Warning: bg1 is 0\n");
        if (!bg2) bg2 = imlib_create_image_using_data(fb_width, fb_height, (uint32_t*)(fb_base + (FB_SIZE * 2)));
        if (!bg2) printf("Warning: bg2 is 0\n");

        imlib_context_set_image(screen_bg);
        int src_w = imlib_image_get_width();
        int src_h = imlib_image_get_height();

        int loading_x = (fb_width - 448) / 2;
        int loading_y = (fb_height - 64) / 2;

        if (cover_img) {
            imlib_context_set_image(cover_img);
            int cover_x = (fb_width - 256) / 2;
            int cover_y = (fb_height - 320) / (cfg.loading_txt_up ? 1.75 : 2);
            int loading_y_cover = (fb_height - 64) / (cfg.loading_txt_up ? 4 : 1.27);

            imlib_context_set_image(bg1);
            imlib_blend_image_onto_image(screen_bg, 0, 0, 0, src_w, src_h, 0, 0, fb_width, fb_height);
            imlib_blend_image_onto_image(cover_img, 0, 0, 0, 256, 320, cover_x, cover_y, 256, 320);
            if (loading_txt) imlib_blend_image_onto_image(loading_txt, 0, 0, 0, 448, 64, loading_x, loading_y_cover, 448, 64);

            imlib_context_set_image(bg2);
            imlib_blend_image_onto_image(screen_bg, 0, 0, 0, src_w, src_h, 0, 0, fb_width, fb_height);
            imlib_blend_image_onto_image(cover_img, 0, 0, 0, 256, 320, cover_x, cover_y, 256, 320);
            if (loading_txt) imlib_blend_image_onto_image(loading_txt, 0, 0, 0, 448, 64, loading_x, loading_y_cover, 448, 64);
        }
        
        else

        {
            imlib_context_set_image(bg1);
            imlib_blend_image_onto_image(screen_bg, 0, 0, 0, src_w, src_h, 0, 0, fb_width, fb_height);
            if (loading_txt) imlib_blend_image_onto_image(loading_txt, 0, 0, 0, 448, 64, loading_x, loading_y, 448, 64);
            
            imlib_context_set_image(bg2);
            imlib_blend_image_onto_image(screen_bg, 0, 0, 0, src_w, src_h, 0, 0, fb_width, fb_height);
            if (loading_txt) imlib_blend_image_onto_image(loading_txt, 0, 0, 0, 448, 64, loading_x, loading_y, 448, 64);

        }
        
        static Imlib_Image curtain = 0;
        if (!curtain) {
            curtain = imlib_create_image(fb_width, fb_height);
            imlib_context_set_image(curtain);
            imlib_image_set_has_alpha(1);

            uint32_t *data = imlib_image_get_data();
            int sz = fb_width * fb_height;
            for (int i = 0; i < sz; i++) {
                *data++ = 0x9F000000;
            }
        }

        imlib_context_set_image(bg1);
        imlib_blend_image_onto_image(curtain, 1, 0, 0, fb_width, fb_height, 0, 0, fb_width, fb_height);
        
        video_fb_enable(1, 1, 1);

        usleep(500000);

        imlib_context_set_image(bg2);
        
        video_fb_enable(1, 2, 1);

        if (loader_bg) sleep(1); else sleep(3);
    }
}

void fade_out_screen() 
{
    video_fb_enable(1, 1, 1);
    usleep(250000);
    video_fb_enable(0);
}
