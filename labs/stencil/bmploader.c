#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "bmp.h"

extern "C" void LoadBMPFile(uchar3 **dst, BMPHeader *my_hdr, 
		 BMPInfoHeader *my_infohdr, char *name){

  BMPHeader hdr; BMPInfoHeader infoHdr;

    int x, y,i;
    //    uchar3 **dst;

    FILE *fd;


    printf("Loading %s...\n", name); 
    if(sizeof(uchar3) != 3){
      printf("***Bad uchar3 size***\n"); exit(0); }

    if( !(fd = fopen(name,"rb")) ){ 
      printf("***BMP load error: file access denied***\n"); exit(0); }

    if (fread(&hdr.type, sizeof(hdr.type), 1, fd) != 1)
    {
      printf("***BMP load error: can't read hdr type***\n"); exit(0); 
    }
    if(hdr.type != 0x4D42) 
    {
      printf("***BMP load error: bad file format***\n"); exit(0); 
    }

    if (fread(&hdr.size, sizeof(hdr.size), 1, fd) != 1)
    {
      printf("***BMP load error: can't read header***\n"); exit(0);
    }

    //    printf("Header size = %d\n", hdr.size);
    if (fread(&hdr.reserved1, sizeof(hdr.reserved1), 1, fd) != 1)
    {
       printf("***BMP load error: can't read header reserved1***\n"); exit(0);
    }
    if (fread(&hdr.reserved2, sizeof(hdr.reserved2), 1, fd) != 1)
    {
      printf("***BMP load error: can't read header reserved2***\n"); exit(0);
    }
    if (fread(&hdr.offset, sizeof(hdr.offset), 1, fd) != 1)
    {
      printf("***BMP load error: can't read header offset***\n"); exit(0);
    }
    
    if (fread(&infoHdr, sizeof(infoHdr), 1, fd) != 1)
    {
      printf("***BMP load error: can't read infoheader***\n"); exit(0);
    }
    bcopy(&hdr, my_hdr, sizeof(hdr));
    bcopy(&infoHdr, my_infohdr, sizeof(infoHdr));

    if(infoHdr.bitsPerPixel != 24){
      printf("***BMP load error: invalid color depth*** %d\n",infoHdr.bitsPerPixel);
      exit(0);
     }

    if(infoHdr.compression){
        printf("***BMP load error: compressed image***\n");
        exit(0);
    }
    
    *dst    = (uchar3 *)malloc(infoHdr.width * infoHdr.height * 3);

    printf("BMP width: %u\n", infoHdr.width);
    printf("BMP height: %u\n", infoHdr.height);

    fseek(fd, hdr.offset - sizeof(hdr) - sizeof(infoHdr), SEEK_CUR);

    for(y = 0; y < infoHdr.height; y++){
        for(x = 0; x < infoHdr.width; x++){
            (*dst)[(y * infoHdr.width + x)].z = fgetc(fd);
            (*dst)[(y * infoHdr.width + x)].y = fgetc(fd);
            (*dst)[(y * infoHdr.width + x)].x = fgetc(fd);
        }

        for(x = 0; x < (4 - (3 * infoHdr.width) % 4) % 4; x++)
	  {
	    printf("what is this junk %d\n",x);
            fgetc(fd);
	  }
    }


    if(ferror(fd)){
        printf("***Unknown BMP load error.***\n");
        free(*dst);
        exit(0);
    }else
        printf("BMP file loaded successfully!\n");

    fclose(fd);
}


extern "C" void WriteBMPFile(uchar3 **img, BMPHeader hdr, BMPInfoHeader infoHdr, const char *name){

    int x, y;

    FILE *fd;


    printf("Wrting %s...\n", name); 
    if(sizeof(uchar3) != 3){
      printf("***Bad uchar3 size***\n"); exit(0); }

    if( !(fd = fopen(name,"w")) ){ 
      printf("***BMP write error: file access denied***\n"); exit(0); }

    //    fwrite(&hdr, sizeof(hdr), 1, fd); 
    
    fwrite(&hdr.type, sizeof(hdr.type), 1, fd);
    fwrite(&hdr.size, sizeof(hdr.size), 1, fd);

    //    printf("Header size = %d\n", hdr.size);
    fwrite(&hdr.reserved1, sizeof(hdr.reserved1), 1, fd);
    fwrite(&hdr.reserved2, sizeof(hdr.reserved2), 1, fd);
    fwrite(&hdr.offset, sizeof(hdr.offset), 1, fd);
    
    fwrite(&infoHdr, sizeof(infoHdr), 1, fd);

    fseek(fd, hdr.offset - sizeof(hdr) - sizeof(infoHdr), SEEK_CUR);

    for(y = 0; y < infoHdr.height; y++){
        for(x = 0; x < infoHdr.width; x++){
	  fputc((*img)[(y * infoHdr.width + x)].z,fd);
	  fputc((*img)[(y * infoHdr.width + x)].y,fd);
	  fputc((*img)[(y * infoHdr.width + x)].x,fd);

	  /*	  fputc(img[y][x].z,fd);
	  fputc(img[y][x].y,fd);
	  fputc(img[y][x].x,fd);
	  */
        }

	for(x = 0; x < (4 - (3 * infoHdr.width) % 4) % 4; x++)
	  {
	    printf("Writing extra %d\n",x);
	    fputc('0',fd);
	  }
    }


    fputc('A',fd);
    fputc('A',fd);
    if(ferror(fd)){
        printf("***Unknown BMP write error.***\n");
        exit(0);
    }else
        printf("BMP file written successfully!\n");

    fclose(fd);
}
