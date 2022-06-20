#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdint.h>
#define DPTR block*     // disk pointer

#define BLOCK_SIZE 512  // block size ;equal to sector and cluster
#define BLOCK_NUM 1024  // block number
#define BLOCK block*

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef u32 BlkID;

struct block {
    u8 a[BLOCK_SIZE];
} __attribute__((packed));

struct fsInfo
{
    /* 2*5 Bytes */
    u16 FSI_BlkSize;
    u16 FSI_BlkCnt;
    u16 FSI_RootEntBlk;
    u16 FSI_FATEntBlk;
    u16 FSI_FATCnt;
    u16 FSI_BitMapEntBlk;
    u16 FSI_BitMapCnt;
    u16 FSI_UserEntBlk;
    u16 FSI_UserCnt;
    char FSI_INFO [50];
    u8 _panding_zero_[BLOCK_SIZE-9*2-50];
}__attribute__((packed));


#define ATTR_EXEC 0x01
#define ATTR_WRITE 0x02
#define ATTR_READ 0x04
#define ATTR_DIC 0x08
#define ATTR_ROOT 0X07
// directory is same as file in format
struct dir {
    u8 DIR_Name[11];
    u32 DIR_FstBlock;
    u32 DIR_FileSize;
    u8 DIR_Attr[13];
} __attribute__((packed));



#define FAT_END  0xFFFF
#define FAT_SYSTEM 0x0001
#define FAT_FREE 0x0000
/* 0x0080 to MAX  means that block is allocated  */

struct fat {
    u16 entry;
} __attribute__((packed));

struct user{
    char USER_NAME[10];
    char USER_PWD[20];
    u16 USER_ID;
}__attribute__((packed));

#define FD_FULL -2
#define FD_SIZE 10
#endif