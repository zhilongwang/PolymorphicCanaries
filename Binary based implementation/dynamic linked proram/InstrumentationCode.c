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




static unsigned char call_chk[9] = {
    0x00,
    0x00,
    0x00,
    0xe8, 0x00, 0x00, 0x00, 0x00,   //call <stack smash>
    0x00

};


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


/* Examine the program segment header table and look for a segment
 * that is loaded into executable memory and is followed by enough padding
 * for our infection program to fit into. The return value is negative if
 * an appropriate segment cannot be found.
 */
static int findinfectionphdr(Elf64_Phdr const *phdr, int count,char *image)
{
    Elf64_Off pos, endpos;
    int i, j;

    Elf64_Word chkaddress=0x00;
    for (i = 0 ; i < count ; ++i) {

        if (phdr[i].p_filesz > 0 && phdr[i].p_filesz == phdr[i].p_memsz
                     && (phdr[i].p_flags & PF_X)) {
            char * pointer=(char *)(image +phdr[i].p_offset);
            char * pointerend=(char *)(image +phdr[i].p_offset + phdr[i].p_filesz);
            //printf("i:%d\n",i);
           
            for(;pointer<pointerend;pointer++){
/*64 48 33 14 25 28 00  xor    %fs:0x28,%rdx*/
                if(*pointer==0x64&&*(pointer+1)==0x48){
                        printf("find_stack_guard%d\n",*(pointer+1));
                    if(*(Elf64_Half*)(pointer+1)==0x3348){
                        printf("find_stack_guard_\n");
                        if(*(Elf64_Word*)(pointer+4)==0x2825){
                           char * find_call_chk=pointer+9;
                            printf("find_stack_guard_____\n");
                            for(;find_call_chk<pointer+15;find_call_chk++){//right
                                //printf("find_next%d\n",*find_call_chk);
                                //printf("find_next%d\n\n",*(find_call_chk+1));
                                if(*find_call_chk==0x75){
                                    chkaddress=*(Elf64_Word *)(*(find_call_chk+1)+find_call_chk+3)+
                                        *(find_call_chk+1)+find_call_chk+7;
                                    printf("chkjne:%x\n",chkaddress);
                                    break;
                                    
                                }else if(*find_call_chk==0x74){//right
                                    char * find_chk=find_call_chk+2;
                                    for(;find_chk<find_call_chk+8;find_chk++){
                                
                                        if((*find_chk)==-24){
                                            chkaddress=*(Elf64_Word *)(find_chk+1)+find_chk+5;
                                            printf("chkje:%x\n",chkaddress);
                                            break;
                                        }
                                    }
                                    break;
                                }else if(*find_call_chk==15&&*(find_call_chk+1)==-123){//right
                                    chkaddress=*(Elf64_Word *)(find_call_chk+7+*(Elf64_Word *)(find_call_chk+2)) +find_call_chk+11+*(Elf64_Word *)(find_call_chk+2);
                                    printf("chkjne--L:%x\n",chkaddress);
                                    break;
                                }/*else if(*find_call_chk==0x0f&&*(find_call_chk+1)==0x84){//right
                                    char * find_chk=find_call_chk+5;
                                    for(;find_chk<find_call_chk+8;find_chk++){
                                        if((*find_chk)==-24){

                                            chkaddress=*(Elf64_Word *)(find_chk+1)+find_chk+5;
                                            printf("chkje--L:%x\n",chkaddress);
                                            break;
                                        }
                                    }
                                    break;
                                }*/
                            }
                            printf("get_chk_address\n");
                            //insertnewguard(guard);
                           
                            pointer+=22; 
                        }
                        
                    }  
                }
                if(chkaddress!=0x00){
                    break;
                }
            }
            if(chkaddress!=0x00){
                    break;
            }
        }
     
    }
    if(chkaddress==0x00){
        return 0;
    }else{
        if(chkaddress%16!=0){
            if(chkaddress%16<8){
                chkaddress=chkaddress-chkaddress%16;
            }else{
                chkaddress=chkaddress+(16-chkaddress%16);
            }
        }
    }
    printf("ok");
    for (i = 0 ; i < count ; ++i) {

        if (phdr[i].p_filesz > 0 && phdr[i].p_filesz == phdr[i].p_memsz
                     && (phdr[i].p_flags & PF_X)) {
            char * pointer=(char *)(image +phdr[i].p_offset);
            char * pointerend=(char *)(image +phdr[i].p_offset + phdr[i].p_filesz);
            printf("i:%d\n",i);
           
            for(;pointer<pointerend;pointer++){
/*64 48 33 14 25 28 00  xor    %fs:0x28,%rdx*/
                if(*pointer==0x64){
                    if(*(Elf64_Half*)(pointer+1)==0x3348&&*(Elf64_Word*)(pointer+4)==0x2825){
                        char reg=*(pointer+3);
                        
                        char * find_zero=pointer+7;
                        //57                    push   %rdi
                        //
                        //
                        //5f                      pop    %rdi
                        if(reg==0x04){//rax
                            //57                    push   %rdi
                            //50                    push   %rax
                            //5f                      pop    %rdi
                            //5f                      pop    %rdi
                            call_chk[0]=0x57;
                            call_chk[1]=0x50;
                            call_chk[2]=0x5f;
                            call_chk[8]=0x5f;
                            *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                            memcpy(pointer,call_chk, sizeof(call_chk));
                            if(*(Elf64_Half*)find_zero==0x0000){
                                *(Elf64_Half*)find_zero=0x9090;
                            }

                        }else if(reg==0x0c){//rcx
                            //57                    push   %rdi
                            //0x51,                  push   %rcx 
                            //5f                    pop    %rdi
                            //5f                      pop    %rdi
                           if(*(Elf64_Half*)find_zero==0x0000){
                                call_chk[0]=0x57;
                                call_chk[1]=0x51;
                                call_chk[2]=0x5f;
                                call_chk[8]=0x5f;
                                *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                                memcpy(pointer,call_chk, sizeof(call_chk));
                            }
                            
                        }else if(reg==0x1c){//rbx
                            //57                    push   %rdi
                            //0x53,                  push   %rbx 
                            //5f                    pop    %rdi
                            //5f                      pop    %rdi

                            if(*(Elf64_Half*)find_zero==0x0000){
                                call_chk[0]=0x57;
                                call_chk[1]=0x53;
                                call_chk[2]=0x5f;
                                call_chk[8]=0x5f;
                                *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                                memcpy(pointer,call_chk, sizeof(call_chk));
                            }
                        }else if(reg==0x14){//rdx
                            //57                    push   %rdi
                            //0x52                    push   %rdx
                            //5f                      pop    %rdi
                            //5f                      pop    %rdi

                            if(*(Elf64_Half*)find_zero==0x0000){
                                call_chk[0]=0x57;
                                call_chk[1]=0x52;
                                call_chk[2]=0x5f;
                                call_chk[8]=0x5f;
                                *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                                memcpy(pointer,call_chk, sizeof(call_chk));
                            }
                        }else if(reg==0x3c){//rdi
                            //57                      push   %rdi
                            //0x57                    push   %rdi
                            //5f                      pop    %rdi
                            //5f                      pop    %rdi

                            if(*(Elf64_Half*)find_zero==0x0000){
                                call_chk[0]=0x57;
                                call_chk[1]=0x57;
                                call_chk[2]=0x5f;
                                call_chk[8]=0x5f;
                                *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                                memcpy(pointer,call_chk, sizeof(call_chk));
                            }
                        }else if(reg==0x34){
                            if(*(Elf64_Half*)find_zero==0x0000){
                                //56                    push   %rsi
                                call_chk[0]=0x57;
                                call_chk[1]=0x56;
                                call_chk[2]=0x5f;
                                call_chk[8]=0x5f;
                                *(Elf64_Word*)(call_chk+4)=chkaddress-(Elf64_Word)(pointer+8);
                                memcpy(pointer,call_chk, sizeof(call_chk));
                            }
                        }
                        printf("insertnewguard\n");
                        //insertnewguard(guard);
                       
                        pointer+=22;
                    }  
                }
            }
            
        }
     
    }

    return -1;
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

    if (argc != 2)
       bail("Usage", "infect FILENAME");
    filename = argv[1];

    /* Load the file into memory and verify that it is a 64-bit ELF
     * executable.
     */
    image = mapfile(filename, &timestamps);
    if (memcmp(image, ELFMAG, SELFMAG))
       bail(filename, "not an ELF file.");
    if (image[EI_CLASS] != ELFCLASS64)
       bail(filename, "not a 64-bit ELF file.");
    ehdr = (Elf64_Ehdr*)image;
    if (ehdr->e_type != ET_EXEC)
       bail(filename, "not an executable file.");

    /* Find a suitable location for our infection.
     */
    phdr = (Elf64_Phdr*)(image + ehdr->e_phoff);
    findinfectionphdr(phdr, ehdr->e_phnum,image);

    utime(filename, &timestamps);

    return 0;
}
