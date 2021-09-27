#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "lab_png.h"
#include "crc.h"
#include "zutil.h"

#define RESULT_FILE "all.png"

void inflate_and_concat(U8 **out, U32 *out_size, U32 *out_offset, simple_PNG_p png)
{
     data_IHDR_p d = (data_IHDR_p)(png->p_IHDR->p_data);
     int w = get_png_width(d);
     int h = get_png_height(d);
     U64 inf_size = h * (w * 4 + 1);
     int ret;
     if (*out == NULL) {
         *out = malloc(inf_size);
         if (*out == NULL) {
             perror("malloc");
             exit(1);
         }
         *out_offset = 0;
         *out_size = inf_size;
     }
     if (inf_size + *out_offset > *out_size) {
         *out_size = inf_size + *out_offset;
         *out = realloc(*out, *out_size);
         if (*out == NULL) {
             perror("realloc");
             exit(2);
         }
     }

     ret = mem_inf(*out + *out_offset, &inf_size, png->p_IDAT->p_data, (U64)png->p_IDAT->length);
     if (ret == 0) { /* success */
         *out_offset += inf_size;
     } else { /* failure */
         fprintf(stderr,"mem_inf failed. ret = %d.\n", ret);
         exit(3);
     }
}

int main(int argc, char *argv[])
{
    int i;
    struct simple_PNG result;
    data_IHDR_p p_result_data;
    U32 result_height;
    struct simple_PNG png;
    U8 *inf_buf = NULL;
    U32 inf_buf_offset = 0;
    U32 inf_buf_size = 0;
    U8 *def_buf = NULL;
    U64 def_buf_size = 0;
    int ret;
    FILE *fp;
    if (argc < 2) return 1;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        exit(1);
    }
    open_png(&result, fp);
    fclose(fp);
    p_result_data = (data_IHDR_p)(result.p_IHDR->p_data);
    result_height = get_png_height(p_result_data);
    inflate_and_concat(&inf_buf, &inf_buf_size, &inf_buf_offset, &result);
    for (i = 2; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (fp == NULL) {
            fprintf(stderr, "cannot open %s\n", argv[i]);
            exit(1);
        }
        open_png(&png, fp);
        fclose(fp);
        inflate_and_concat(&inf_buf, &inf_buf_size, &inf_buf_offset, &png);
        result_height += get_png_height((data_IHDR_p)(png.p_IHDR->p_data));
        destroy_png(&png);
    }
    def_buf = malloc(inf_buf_offset);
    ret = mem_def(def_buf, &def_buf_size, inf_buf, inf_buf_offset, Z_DEFAULT_COMPRESSION);
    if (ret != 0) {
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }
    p_result_data->height = htonl(result_height);
    result.p_IHDR->crc = crc(result.p_IHDR->type, CHUNK_TYPE_SIZE);
    result.p_IHDR->crc = update_crc(result.p_IHDR->crc ^ 0xffffffffL, result.p_IHDR->p_data, result.p_IHDR->length) ^ 0xffffffffL;
    result.p_IDAT->length = def_buf_size;
    free(result.p_IDAT->p_data);
    result.p_IDAT->p_data = def_buf;
    result.p_IDAT->crc = crc(result.p_IDAT->type, CHUNK_TYPE_SIZE);
    result.p_IDAT->crc = update_crc(result.p_IDAT->crc ^ 0xffffffffL, result.p_IDAT->p_data, result.p_IDAT->length) ^ 0xffffffffL;
    save_png(&result, RESULT_FILE);
    
    destroy_png(&result);
    free(inf_buf);
    return 0;
}
