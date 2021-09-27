#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include "lab_png.h"

const static U8 png_sig[PNG_SIG_SIZE] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
int is_png(FILE* fp)
{
    U8 buf[PNG_SIG_SIZE];
    if (fseek(fp, 0, SEEK_SET)) {
        return 0;
    }

    if (fread(buf, 1, PNG_SIG_SIZE, fp) != PNG_SIG_SIZE) {
        return 0;
    }

    return memcmp(buf, png_sig, PNG_SIG_SIZE) == 0;
}

int get_png_height(struct data_IHDR *buf)
{
    return ntohl(buf->height);
}
int get_png_width(struct data_IHDR *buf)
{
    return ntohl(buf->width);
}

#define DATA_IHDR_OFFSET 16

int get_png_data_IHDR(struct data_IHDR *out, FILE *fp)
{
    if (fseek(fp, DATA_IHDR_OFFSET, SEEK_SET)) {
        return -1;
    }
    if (fread(out, 1, DATA_IHDR_SIZE, fp) != DATA_IHDR_SIZE) {
        return -1;
    }
    return 0;
}

static void free_chunk(chunk_p c)
{
    if (c == NULL) return;
    if (c->p_data != NULL) free(c->p_data);
    free(c);
}

void destroy_png(simple_PNG_p png)
{
     free_chunk(png->p_IHDR);
     free_chunk(png->p_IDAT); 
     free_chunk(png->p_IEND);
}

int open_png(simple_PNG_p png, FILE *fp)
{
    assert(png != NULL);
    assert(fp != NULL);
    png->p_IHDR = NULL;
    png->p_IDAT = NULL;
    png->p_IEND = NULL;
    if (fseek(fp, PNG_SIG_SIZE, SEEK_SET)) return -1;
    while (png->p_IEND == NULL) {
        chunk_p c = malloc(sizeof(*c));
        if (c == NULL) {
            return -1;
        }
        c->p_data = NULL;
        fread(&c->length, 1, CHUNK_LEN_SIZE, fp);
        c->length = ntohl(c->length);
        fread(c->type, 1, CHUNK_TYPE_SIZE, fp);
        if (c->length != 0) {
            c->p_data = malloc(c->length);
            if (c->p_data == NULL) {
                free_chunk(c);
                return -1;
            }
            fread(c->p_data, 1, c->length, fp);
        }
        fread(&c->crc, 1, CHUNK_CRC_SIZE, fp);
        c->crc = ntohl(c->crc);
        if (c->type[0] == 'I' && c->type[1] == 'H' && c->type[2] == 'D' && c->type[3] == 'R') {
            if (png->p_IHDR != NULL) {
                free_chunk(c);
                return -1;
            } 
            png->p_IHDR = c;
        } else if (c->type[0] == 'I' && c->type[1] == 'D' && c->type[2] == 'A' && c->type[3] == 'T') {
            if (png->p_IDAT != NULL) {
                free_chunk(c);
                return -1;
            } 
            png->p_IDAT = c;
        } else if (c->type[0] == 'I' && c->type[1] == 'E' && c->type[2] == 'N' && c->type[3] == 'D') {
            if (png->p_IEND != NULL) {
                free_chunk(c);
                return -1;
            } 
            png->p_IEND = c;
        } else {
            free_chunk(c);
            return -1;
        }
    }
    return 0;
}

int save_png(simple_PNG_p png, const char *filename)
{
    U32 len_be;
    U32 crc_be;
    FILE *fp;

    assert(png != NULL);
    assert(png->p_IHDR != NULL);
    assert(png->p_IDAT != NULL);
    assert(png->p_IEND != NULL);
    
    fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }
    fwrite(png_sig, 1, PNG_SIG_SIZE, fp);
    // IHDR
    len_be = htonl(png->p_IHDR->length);
    crc_be = htonl(png->p_IHDR->crc);
    fwrite(&len_be, 1, sizeof(U32), fp);
    fwrite(png->p_IHDR->type, 1, CHUNK_TYPE_SIZE, fp);
    if (png->p_IHDR->length != 0 && png->p_IHDR->p_data != NULL) {
        fwrite(png->p_IHDR->p_data, 1, png->p_IHDR->length, fp);
    }
    fwrite(&crc_be, 1, sizeof(U32), fp);
    
    // IDAT
    len_be = htonl(png->p_IDAT->length);
    crc_be = htonl(png->p_IDAT->crc);
    fwrite(&len_be, 1, sizeof(U32), fp);
    fwrite(png->p_IDAT->type, 1, CHUNK_TYPE_SIZE, fp);
    if (png->p_IDAT->length != 0 && png->p_IDAT->p_data != NULL) {
        fwrite(png->p_IDAT->p_data, 1, png->p_IDAT->length, fp);
    }
    fwrite(&crc_be, 1, sizeof(U32), fp);
    
    // IEND
    len_be = htonl(png->p_IEND->length);
    crc_be = htonl(png->p_IEND->crc);
    fwrite(&len_be, 1, sizeof(U32), fp);
    fwrite(png->p_IEND->type, 1, CHUNK_TYPE_SIZE, fp);
    if (png->p_IEND->length != 0 && png->p_IEND->p_data != NULL) {
        fwrite(png->p_IEND->p_data, 1, png->p_IEND->length, fp);
    }
    fwrite(&crc_be, 1, sizeof(U32), fp);

    fclose(fp);
    return 0;
}
