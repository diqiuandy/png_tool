#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lab_png.h"
#include "crc.h"

int main(int argc, char *argv[])
{
    const char *filepath;
    struct data_IHDR d;
    struct simple_PNG png;
    FILE *fp;
    U32 crc_value;

    if (argc != 2) {
        printf("Usage: %s FILE\n", argv[0]);
        return 1;
    }
    filepath = argv[1];
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        printf("Cannot open %s\n", filepath);
        return 1;
    }
    printf("%s: ", filepath);
    if (!is_png(fp)) {
        printf("Not a PNG file\n");
        fclose(fp);
        return 1;
    }
    if (get_png_data_IHDR(&d, fp)) {
        printf("No IHDR data found\n");
        fclose(fp);
        return 1;
    }
    printf("%u x %u\n", get_png_width(&d), get_png_height(&d));

    open_png(&png, fp);

    // IHDR CRC
    if (png.p_IHDR == NULL) {
        printf("missing IHDR\n");
        destroy_png(&png);
        fclose(fp);
        return 1;
    }
    crc_value = crc(png.p_IHDR->type, CHUNK_TYPE_SIZE);
    crc_value = update_crc(crc_value ^ 0xffffffffL, png.p_IHDR->p_data, png.p_IHDR->length) ^ 0xffffffffL;
    if (crc_value != png.p_IHDR->crc) {
        printf("IDAT chunk CRC error: computed %x, expected %x\n", crc_value, png.p_IHDR->crc);
        destroy_png(&png);
        fclose(fp);
        return 1;
    }

    // IDAT CRC
    if (png.p_IDAT == NULL) {
        printf("missing IDAT\n");
        destroy_png(&png);
        fclose(fp);
        return 1;
    }
    crc_value = crc(png.p_IDAT->type, CHUNK_TYPE_SIZE);
    crc_value = update_crc(crc_value ^ 0xffffffffL, png.p_IDAT->p_data, png.p_IDAT->length) ^ 0xffffffffL;
    if (crc_value != png.p_IDAT->crc) {
        printf("IDAT chunk CRC error: computed %x, expected %x\n", crc_value, png.p_IDAT->crc);
        destroy_png(&png);
        fclose(fp);
        return 1;
    }

    // IEND CRC
    if (png.p_IEND == NULL) {
        printf("missing IEND\n");
        destroy_png(&png);
        fclose(fp);
        return 1;
    }
    crc_value = crc(png.p_IEND->type, CHUNK_TYPE_SIZE);
    if (crc_value != png.p_IEND->crc) {
        printf("CRC error in chunk IEND (computed %x, expected %x)\n", crc_value, png.p_IEND->crc);
        destroy_png(&png);
        fclose(fp);
        return 1;
    }
    destroy_png(&png);
    fclose(fp);
    return 0;
}
