/* infect: Copyright (C) 2017 by ZhilongWang <mg1633081@smail.nju.edu.cn>
 * License GPLv2+: GNU GPL version 2 or later.
 * This is free software; you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utime.h>
#include <elf.h>


/* Display an error message and exit the program.
 */
static void bail(char const *prefix, char const *msg)
{
    fprintf(stderr, "%s: %s\n", prefix, msg);
    exit(EXIT_FAILURE);
}

/* Map a file into read-write memory. The return value is a pointer to
 * the beginning of the file image. If utimbuf is not NULL, it receives
 * the file's current access and modification times.
 */
static void *mapfile(char const *filename, struct utimbuf *utimbuf)
{
    struct stat stat;
    void *ptr;
    int fd;

    fd = open(filename, O_RDWR);
    if (fd < 0)
       bail(filename, strerror(errno));
    if (fstat(fd, &stat))
       bail(filename, strerror(errno));
    if (!S_ISREG(stat.st_mode))
       bail(filename, "not an ordinary file.");
    ptr = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
       bail(filename, strerror(errno));
    if (utimbuf) {
       utimbuf->actime = stat.st_atime;
       utimbuf->modtime = stat.st_mtime;
    }
    return ptr;
}


/* main().
 */
int main(int argc, char *argv[])
{
    char const *filename;
    struct utimbuf timestamps;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    char *image;

    if (argc != 2){

       bail("Usage", "infect FILENAME");
	    printf("no");
	}
    filename = argv[1];

    /* Load the file into memory and verify that it is a 64-bit ELF
     * executable.
     */
    image = mapfile(filename, &timestamps);
    if (memcmp(image, ELFMAG, SELFMAG))
       printf("no");
    if (image[EI_CLASS] != ELFCLASS64)
       printf("no");
    ehdr = (Elf64_Ehdr*)image;
    if (ehdr->e_type != ET_EXEC)
       printf("no");
    
    return 0;
}
