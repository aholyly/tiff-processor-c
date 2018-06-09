/****************************************************************************/ 
/* HW01_141044015_Ahmet_Mert_Gulbahce 										*/ 
/****************************************************************************/
/* 141044015_main.c	 														*/
/****************************************************************************/
/*																			*/
/* Created on 22/03/2018 by Ahmet Mert Gulbahce 							*/
/*																			*/
/* Description                                                              */
/* ----------- 																*/
/* This program reads uncompressed and 8bit black-white .tiff image format  */
/* and write images on terminal like an image. This program runs only 		*/
/* under 32 pixels width size.												*/
/* 																			*/
/****************************************************************************/

/****************************************************************************/
/*								Includes									*/ 
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>

/****************************************************************************/
/*								Defines 									*/ 
/****************************************************************************/
#define TAG_STRIP_OFFSET 273
#define TAG_IMAGE_WIDTH 256
#define TAG_IMAGE_LENGHT 257


/****************************************************************************/
/*								Structures									*/ 
/****************************************************************************/
struct Header
{
	char byteOrder[2];
	uint16_t version;
	uint32_t offset;
};

struct Entry
{
	uint16_t tag;
	uint16_t type;
	uint32_t numOfVal;
	uint32_t valOffset;
};

struct IFD
{
	uint16_t numOfEntries;
	struct Entry *entries;
	uint32_t nextOffset;
};

struct TIFF
{
	int inputFd;
	char *path;
	struct Header *header;
	struct IFD *ifd;
};

/****************************************************************************/
/*							Functions Definitons							*/ 
/****************************************************************************/
int open_file(struct TIFF *tiff, char const *path);
int close_file(struct TIFF *tiff);
void close_and_free(struct TIFF *tiff);
void free_all(struct TIFF *tiff);
int read_header(struct TIFF *tiff);
int read_file(struct TIFF *tiff);
int read_color(struct TIFF *tiff, char const *path);
void print_info(struct TIFF *tiff);
uint32_t byteswap32(uint32_t nLongNumber);
uint16_t byteswap16(uint16_t nValue);
void int2bin(int a, char *buffer, int buf_size);


/****************************************************************************/
/*								Main										*/ 
/****************************************************************************/
int main(int argc, char const *argv[])
{
	struct TIFF *tiff;

	/* USAGE */
	if(argc != 2)
	{
		printf("Usage: %s FILENAME.tif\n", argv[0]);
		exit(1);
	}


	tiff = malloc(sizeof(struct TIFF));
	
	/* Open file for all functions */
	if(open_file(tiff,argv[1]) == 0)
	{
		free(tiff);
		exit(1);
	}

	/* Reads header for both MM and II */
	if(read_header(tiff) == 0)
	{
		close_and_free(tiff);
		exit(1);
	}

	/* If byteorder dont match with I and M */
	/* exits from program */
	if(tiff->header->byteOrder[0] != 'I' && tiff->header->byteOrder[0] != 'M')
	{
		printf("Wrong byte order!\n");
		close_file(tiff);
		exit(1);
	}	

	/* Reads informations from tiff file */
	if(read_file(tiff) == 0)
	{
		close_and_free(tiff);
		exit(1);
	}

	/* Prints width, height and byte order informations on screen */
	print_info(tiff);

	/* Reads color informations from tiff file */
	if(read_color(tiff,argv[1]) == 0)
	{
		close_and_free(tiff);
		exit(1);
	}


	close_and_free(tiff);

	return 0;
}

/* Prints width, height and byte order informations on screen */
void print_info(struct TIFF *tiff)
{
	int width, height;

	for (int i = 0; i < tiff->ifd->numOfEntries; ++i)
	{
		if(tiff->ifd->entries[i].tag == TAG_IMAGE_WIDTH)
			width = tiff->ifd->entries[i].valOffset;
		if(tiff->ifd->entries[i].tag == TAG_IMAGE_LENGHT)
			height = tiff->ifd->entries[i].valOffset;
	}

	/* PRINT */
	printf("Width : %d pixels\n", width);
	printf("Height : %d pixels\n", height);
	if(tiff->header->byteOrder[0] == 'I')
		printf("Byte order: Intel\n");
	if(tiff->header->byteOrder[0] == 'M')
		printf("Byte order: Motorola\n");
}

/* Opens file for all functions */
/* FD opens one time in program */
int open_file(struct TIFF *tiff, char const *path)
{
	ssize_t numRead;

	tiff->inputFd = open(path, O_RDONLY);
	if (tiff->inputFd == -1)
	{
		perror("Cannot open file! ");
		return 0;
	}

	return 1;
}

/* Closes fd  */
int close_file(struct TIFF *tiff)
{
	if(close(tiff->inputFd) == -1)
	{
		perror("Cannot close file!");
		return 0;
	}

	return 1;
}

/* Closes fd and free all blocks in program */
void close_and_free(struct TIFF *tiff)
{
	if(close(tiff->inputFd) == -1)
	{
		perror("Cannot close file!");
		exit(1);
	}

	free_all(tiff);
}

/* Free all blocks */
void free_all(struct TIFF *tiff)
{
	if(tiff->header != NULL)
		free(tiff->header);
	if(tiff->ifd->entries != NULL)
		free(tiff->ifd->entries);
	if (tiff->ifd != NULL)
		free(tiff->ifd);
	if(tiff != NULL)
		free(tiff);
}

/* Reads header information in tiff file */
int read_header(struct TIFF *tiff)
{
	ssize_t numRead;
	
	tiff->header = malloc(sizeof(struct Header));

	numRead = read(tiff->inputFd, tiff->header, sizeof(struct Header));
	if (numRead == -1)
	{
		perror("Cannot read from file!");
		return 0;
	}

	//printf("%s %d %d\n", tiff->header->byteOrder, tiff->header->version, tiff->header->offset);

	return 1;
}

/* Reads informations from tiff file */
int read_file(struct TIFF *tiff)
{
	ssize_t numRead;
	uint8_t seek;
	uint16_t temp;

	if(tiff->header->byteOrder[0] == 'M')
		tiff->header->offset = byteswap32(tiff->header->offset);

	/* Seek */
	for (int i = 0; i < tiff->header->offset-(sizeof(struct Header)); ++i)
	{
		numRead = read(tiff->inputFd, &seek, sizeof(uint8_t));
		if (numRead == -1)
		{
			perror("Cannot read from file!");
			return 0;
		}
	}

	/* Read number of directory entries */
	numRead = read(tiff->inputFd, &temp, sizeof(uint16_t));
	if (numRead == -1)
	{
		perror("Cannot read from file!");
		return 0;
	}

	tiff->ifd = malloc(sizeof(struct IFD));

	if(tiff->header->byteOrder[0] == 'M')
		tiff->ifd->numOfEntries = byteswap16(temp);
	else
		tiff->ifd->numOfEntries = temp;
	
	tiff->ifd->entries = malloc(tiff->ifd->numOfEntries*sizeof(struct Entry));
	
	for (int i = 0; i < tiff->ifd->numOfEntries; ++i)
	{
		/* Read TAG - TYPE- NUMBER OF VALUES */
		read(tiff->inputFd, &tiff->ifd->entries[i].tag, sizeof(uint16_t));
		read(tiff->inputFd, &tiff->ifd->entries[i].type, sizeof(uint16_t));
		read(tiff->inputFd, &tiff->ifd->entries[i].numOfVal, sizeof(uint32_t));

		/* If M byteorder, swap bytes */
		if(tiff->header->byteOrder[0] == 'M')
		{
			tiff->ifd->entries[i].tag = byteswap16(tiff->ifd->entries[i].tag);
			tiff->ifd->entries[i].type = byteswap16(tiff->ifd->entries[i].type);
			tiff->ifd->entries[i].numOfVal = byteswap32(tiff->ifd->entries[i].numOfVal);
		}

		/* If Type is 3, read 16bits */
		if(tiff->ifd->entries[i].type == 3)
		{
			read(tiff->inputFd, &tiff->ifd->entries[i].valOffset, sizeof(uint16_t));
			read(tiff->inputFd, &temp, sizeof(uint16_t));
			if(tiff->header->byteOrder[0] == 'M')
				tiff->ifd->entries[i].valOffset = byteswap16(tiff->ifd->entries[i].valOffset);

		}
		/* Else, read 32bits */
		else
		{
			read(tiff->inputFd, &tiff->ifd->entries[i].valOffset, sizeof(uint32_t));
			if(tiff->header->byteOrder[0] == 'M')
				tiff->ifd->entries[i].valOffset = byteswap32(tiff->ifd->entries[i].valOffset);
		}

		//printf("%d %d %d %d\n", tiff->ifd->entries[i].tag,tiff->ifd->entries[i].type,
		//	tiff->ifd->entries[i].numOfVal,tiff->ifd->entries[i].valOffset);
	}

	read(tiff->inputFd, &tiff->ifd->nextOffset, sizeof(uint32_t));

	/* If file has more than one page, exit from function and program */
	if(tiff->ifd->nextOffset != 0)
	{
		printf("File has more than one page!\n");
		return 0;
	}

	if(close_file(tiff) == 0)
		return 0;

	return 1;
}

/* Reads color informations from StripOffset and writes to screen */
int read_color(struct TIFF *tiff, char const *path)
{
	ssize_t numRead;
	uint8_t seek;

	int offset, width, height;
	int found = -1;

	uint32_t *lines;
	char **binary;
	int BUF_SIZE = 33;


	if(open_file(tiff,path) == 0)
		return 0;

	/* Assign values for simplicity */
	for (int i = 0; i < tiff->ifd->numOfEntries; ++i)
	{
		if(tiff->ifd->entries[i].tag == TAG_STRIP_OFFSET)
		{
			found = i;
			offset = tiff->ifd->entries[i].valOffset;
		}
		if(tiff->ifd->entries[i].tag == TAG_IMAGE_WIDTH)
		{
			found = i;
			width = tiff->ifd->entries[i].valOffset;
		}
		if(tiff->ifd->entries[i].tag == TAG_IMAGE_LENGHT)
		{
			found = i;
			height = tiff->ifd->entries[i].valOffset;
		}
	}

	lines = malloc(height*sizeof(uint32_t));

	if(found == -1)
	{
		perror("Cannot found offset!");
		close_and_free(tiff);
		return 0;
	}

	for (int i = 0; i < offset; ++i)
	{
		numRead = read(tiff->inputFd, &seek, sizeof(uint8_t));
		if (numRead == -1)
		{
			perror("Cannot read from file!");
			return 0;
		}
	}

	binary = malloc( height * sizeof(char*) );
	for (int i = 0; i < height; ++i)
	{
		binary[i] = malloc(BUF_SIZE);
		binary[i][BUF_SIZE-1] = '\0';
	}

	for (int i = 0; i < height; ++i)
	{
		numRead = read(tiff->inputFd, &lines[i], sizeof(uint32_t));
		if (numRead == -1)
		{
			perror("Cannot read from file!");
			return 0;
		}
		if(tiff->header->byteOrder[0] == 'M')
			lines[i] = byteswap32(lines[i]);
		int2bin(lines[i],binary[i],BUF_SIZE-1);
	}

	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			printf("%c", binary[i][j]);
		
		}
		printf("\n");

		if(binary[i] != NULL)
			free(binary[i]);
	}

	if(binary != NULL)
		free(binary);

	if(lines != NULL)
		free(lines);
		
	return 1;
}


/* Reference: https://github.com/jkriege2/TinyTIFF/blob/master/tinytiffreader.cpp Line:331*/
uint32_t byteswap32(uint32_t nLongNumber)
{
	return (((nLongNumber&0x000000FF)<<24)+((nLongNumber&0x0000FF00)<<8)+
	((nLongNumber&0x00FF0000)>>8)+((nLongNumber&0xFF000000)>>24));
}


/* Reference: https://github.com/jkriege2/TinyTIFF/blob/master/tinytiffreader.cpp Line:337*/
uint16_t byteswap16(uint16_t nValue)
{
	return (((nValue>> 8)) | (nValue << 8));
}

/* Reference: https://stackoverflow.com/questions/1024389/print-an-int-in-binary-representation-using-c */
void int2bin(int a, char *buffer, int buf_size) 
{
	buffer += (buf_size - 1);

	for (int i = 31; i >= 0; i--) {
		*buffer-- = (a & 1) + '0';
		a >>= 1;
	}
}

/*****************************************************************************/
/*				 			ENF OF 141044015_main.c							 */
/*****************************************************************************/