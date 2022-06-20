#include <bits/stdc++.h>
#include "h/block.h"
#include "utils/color.h"
using namespace std;
fsInfo* fsi;
u32* bm;
struct Status {
    BlkID curDicBlk;
    char* curDicName;
    u8 userID;
    char* username;

} curSt;
struct FileDec {
    u8 fd_active;
    dir* fd_dir;

} curFd[FD_SIZE];
struct PathList {
    char name[11];
    PathList* next = NULL;
    PathList* prior = NULL;
};
PathList* PathList_head;
PathList* PathList_tail;
int au_middleware(DPTR disk, u8 op, dir* d);
int get_free_fd() {
    for (int i = 0; i < FD_SIZE; i++) {
        if (!curFd[i].fd_active) {
            return i;
        }
    }
    return FD_FULL;
}
BLOCK get_free_block(DPTR disk) {
    return disk + 2;
}
void set_FAT(DPTR disk, BlkID blk, u16 value) {
    fat* p = (fat*)&disk[fsi->FSI_FATEntBlk];
    p[blk].entry = value;
}
BlkID get_fin_blk(DPTR disk, BlkID blk) {
    fat* p = (fat*)&disk[fsi->FSI_FATEntBlk];
    BlkID ans = blk;
    for (BlkID i = blk; p[i].entry != FAT_END && p[i].entry != FAT_SYSTEM;
         i = p[i].entry)
        ans = p[i].entry;
    return ans;
}
BlkID get_next_blk(DPTR disk, BlkID blk) {
    fat* p = (fat*)&disk[fsi->FSI_FATEntBlk];
    BlkID ans = -1;
    if (p[blk].entry != FAT_END && p[blk].entry != FAT_SYSTEM)
        ans = p[blk].entry;
    return ans;
}
void Wrblk2bm(uint32_t* bm, uint32_t blk, int o) {
    int r, off;
    r = blk / 32;
    off = blk % 32;
    if (o) {
        bm[r] |= (1 << (31 - off));
    } else {
        bm[r] &= ~(1 << (31 - off));
    }
}

void init_FSI(DPTR disk) {
    // write into #0 block

    fsi = (fsInfo*)&disk[0];
    memset(fsi, 0, sizeof(struct block));
    fsi->FSI_BlkSize = BLOCK_SIZE;
    fsi->FSI_BlkCnt = BLOCK_NUM;
    fsi->FSI_FATEntBlk = 1;
    fsi->FSI_FATCnt = 125;  // #1 to #125
    fsi->FSI_BitMapEntBlk = 126;
    fsi->FSI_BitMapCnt = 2;  // #126 to #127
    fsi->FSI_UserEntBlk = 128;
    fsi->FSI_UserCnt = 1;
    // must be integral multiples of byte ??? 我忘了为啥了
    fsi->FSI_RootEntBlk = 129;
    strcpy(fsi->FSI_INFO, "FILE SYSTEM beta0.114514 by Minorant");  // version
}
void init_FAT(DPTR disk) {
    // blocks from #1 to #125 are allocated to FAT
    fat* top = (fat*)&disk[fsi->FSI_FATEntBlk];
    memset(top, 0, fsi->FSI_FATCnt * sizeof(struct block));
    for (int i = 0; i < fsi->FSI_RootEntBlk; i++) {
        top[i].entry = FAT_SYSTEM;
    }
}
void init_BitMap(DPTR disk) {
    /*
         0 1 2 ... 31
      0  ? ? ? ... ?
      1  ? ? ? ... ?
      2  ? ? ? ... ?
      ...
      31 ? ? ? ... ?
    */
    bm = (u32*)&disk[fsi->FSI_BitMapEntBlk];
    memset(bm, 0, fsi->FSI_BitMapCnt * sizeof(struct block));
    memset(bm, 0xff, (fsi->FSI_RootEntBlk / 8));

    // for (int i = 0; i <= 10; i++) {
    //     for (int j = 0; j <= 31; j++) {
    //         int ans = bm[i] >> (31 - j) & 1;
    //         cout << ans << " ";
    //     }
    //     cout << endl;
    // }
}
void show_bitmap() {
    for (int i = 0; i <= 10; i++) {
        for (int j = 0; j <= 31; j++) {
            int ans = bm[i] >> (31 - j) & 1;
            cout << ans << " ";
        }
        cout << endl;
    }
}
void init_rootDir(DPTR disk) {
    Wrblk2bm(bm, fsi->FSI_RootEntBlk, 1);
    dir* root = (dir*)&disk[fsi->FSI_RootEntBlk];
    strcpy((char*)(root[0].DIR_Name), ".");
    root[0].DIR_Attr[0] |= ATTR_DIC | ATTR_ROOT;
    root[0].DIR_FstBlock = fsi->FSI_RootEntBlk;
    strcpy((char*)(root[1].DIR_Name), "..");
    root[1].DIR_Attr[0] |= ATTR_DIC | ATTR_ROOT;
    root[1].DIR_FstBlock = fsi->FSI_RootEntBlk;

    set_FAT(disk, fsi->FSI_RootEntBlk, FAT_END);
}
void init_User(DPTR disk) {
    for (int i = 0; i < fsi->FSI_UserCnt; i++)
        Wrblk2bm(bm, fsi->FSI_UserEntBlk + i, 1);
    user* u = (user*)&disk[fsi->FSI_UserEntBlk];
    strcpy((char*)(u[0].USER_NAME), "root");
    strcpy((char*)(u[0].USER_PWD), "admin");
    strcpy((char*)(u[1].USER_NAME), "user1");
    strcpy((char*)(u[1].USER_PWD), "123");
    u[0].USER_ID = 0;
    u[1].USER_ID = 1;
}
void show_dir(DPTR disk) {
    dir* root = (dir*)&disk[curSt.curDicBlk];
    BlkID curb = curSt.curDicBlk;
    char c[10];
    c[ATTR_DIC] = 'd';
    c[ATTR_READ] = 'r';
    c[ATTR_WRITE] = 'w';
    c[ATTR_EXEC] = 'x';

    do {
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir); i++) {
            if (!*(root[i].DIR_Name))
                continue;
            for (int j = 8; j; j >>= 1) {
                if (j & root[i].DIR_Attr[0])
                    putchar(c[j]);
                else
                    putchar('-');
            }
            for (int j = 4; j; j >>= 1) {
                if (j & root[i].DIR_Attr[curSt.userID])
                    putchar(c[j]);
                else
                    putchar('-');
            }
            printf(" %-5d ", root[i].DIR_FileSize);
            printf(" 0X%X ", root[i].DIR_FstBlock * BLOCK_SIZE);
            if (root[i].DIR_Attr[0] & ATTR_DIC) {
                printf(BLU "%s\n" RESET, root[i].DIR_Name);
            }

            else {
                printf(GRN "%s\n" RESET, root[i].DIR_Name);
            }
        }
        curb = get_next_blk(disk, curb);
        root = (dir*)&disk[curb];
    } while (curb != -1);
    puts("");
}
u32 get_free_block() {
    // scan by line
    int bmSize = fsi->FSI_BitMapCnt * fsi->FSI_BlkSize / 4;
    for (int i = 0; i < bmSize; i++) {
        if (bm[i] == 0xffffffff)
            continue;
        for (int j = 0; j < 32; j++) {
            int a = bm[i] >> (31 - j) & 1;
            if (!a)
                return i * 32 + j;
        }
    }
}
dir* get_free_dir_item(DPTR disk) {
    // add into parent directory
    BlkID finBlk = get_fin_blk(disk, curSt.curDicBlk);
    BlkID nowBlk = curSt.curDicBlk;
    dir* faDic = (dir*)&disk[curSt.curDicBlk];
    for (int i = 0; i <= BLOCK_SIZE / sizeof(dir); i++) {
        if (i == BLOCK_SIZE / sizeof(dir)) {
            if (nowBlk == finBlk) {
                // create a new block if parent directory is full
                BlkID fb = get_free_block();
                Wrblk2bm(bm, fb, 1);
                set_FAT(disk, fb, FAT_END);
                set_FAT(disk, nowBlk, fb);
                faDic = (dir*)&disk[fb];
                break;
            } else {
                nowBlk = get_next_blk(disk, nowBlk);
                faDic = (dir*)&disk[nowBlk];
                i = -1;
            }
        } else {
            if (*(faDic->DIR_Name)) {
                faDic++;
            } else {
                break;
            }
        }
    }
    return faDic;
}
void mkdir(DPTR disk, char* name) {
    /*  make a directory */

    // create a new block and init (add . ..)
    BlkID newBlk = get_free_block();
    Wrblk2bm(bm, newBlk, 1);
    set_FAT(disk, newBlk, FAT_END);
    dir* newdir = (dir*)&disk[newBlk];
    strcpy((char*)(newdir[0].DIR_Name), ".");
    newdir[0].DIR_Attr[0] |= ATTR_DIC | ATTR_ROOT;
    newdir[0].DIR_Attr[curSt.userID] |= ATTR_ROOT;
    newdir[0].DIR_FstBlock = newBlk;
    strcpy((char*)(newdir[1].DIR_Name), "..");
    newdir[1].DIR_Attr[0] |= ATTR_DIC | ATTR_ROOT;
    newdir[1].DIR_Attr[curSt.userID] |= ATTR_ROOT;
    newdir[1].DIR_FstBlock = curSt.curDicBlk;

    dir* faDic = get_free_dir_item(disk);
    strcpy((char*)(faDic->DIR_Name), name);
    faDic->DIR_Attr[0] |= ATTR_DIC | ATTR_ROOT;
    faDic->DIR_Attr[curSt.userID] |= ATTR_ROOT;

    faDic->DIR_FstBlock = newBlk;
}
void create(DPTR disk, char name[]) {
    /* create a file */

    // create a new block and init (add . ..)
    BlkID newBlk = get_free_block();
    Wrblk2bm(bm, newBlk, 1);
    set_FAT(disk, newBlk, FAT_END);

    dir* faDic = get_free_dir_item(disk);
    strcpy((char*)(faDic->DIR_Name), name);
    faDic->DIR_Attr[0] |= ATTR_ROOT;
    faDic->DIR_Attr[curSt.userID] |= ATTR_ROOT;
    faDic->DIR_FstBlock = newBlk;
}
void _delete(DPTR disk, BlkID bid) {
    if (bid == -1)
        return;
    _delete(disk, get_next_blk(disk, bid));

    set_FAT(disk, bid, FAT_FREE);
    Wrblk2bm(bm, bid, 0);
}
dir* get_dir_by_name(DPTR disk, char* name) {
    BlkID finBlk = get_fin_blk(disk, curSt.curDicBlk);
    BlkID nowBlk = curSt.curDicBlk;
    dir* faDic = (dir*)&disk[curSt.curDicBlk];
    for (int i = 0; i <= BLOCK_SIZE / sizeof(dir); i++) {
        if (i == BLOCK_SIZE / sizeof(dir)) {
            if (nowBlk == finBlk) {
                return NULL;
            } else {
                nowBlk = get_next_blk(disk, nowBlk);
                faDic = (dir*)&disk[nowBlk];
                i = -1;
            }
        } else {
            if (strcmp((char*)faDic->DIR_Name, name)) {
                faDic++;
            } else {
                return faDic;
            }
        }
    }
}
void delete1(DPTR disk, char name[]) {
    dir* d = get_dir_by_name(disk, name);
    if (d == NULL) {
        printf(RED "delete:[Error]: Could not find %s " RESET "\n", name);
        return;
    }
    _delete(disk, d->DIR_FstBlock);
    memset(d, 0, sizeof(dir));
}
dir* get_dir_by_name_blk(DPTR disk, char* name, BlkID bid) {
    BlkID finBlk = get_fin_blk(disk, bid);
    BlkID nowBlk = bid;
    dir* faDic = (dir*)&disk[bid];
    for (int i = 0; i <= BLOCK_SIZE / sizeof(dir); i++) {
        if (i == BLOCK_SIZE / sizeof(dir)) {
            if (nowBlk == finBlk) {
                return NULL;
            } else {
                nowBlk = get_next_blk(disk, nowBlk);
                faDic = (dir*)&disk[nowBlk];
                i = -1;
            }
        } else {
            if (strcmp((char*)faDic->DIR_Name, name)) {
                faDic++;
            } else {
                return faDic;
            }
        }
    }
}

void cd(DPTR disk, char name[]) {
    /* change the working directory */
    if (!strcmp(name, (char*)"."))
        return;

    dir* d = get_dir_by_name(disk, name);
    if (d == NULL) {
        printf(RED "cd:[Error] Could not find %s" RESET "\n", name);
        return;
    }
    if (!(d->DIR_Attr[0] & ATTR_DIC)) {
        printf(RED "cd:[Error] %s is not a directory\n" RESET, name);
        return;
    }

    curSt.curDicBlk = d->DIR_FstBlock;
    if (!strcmp(name, (char*)"..")) {
        // back
        if (curSt.curDicBlk == fsi->FSI_RootEntBlk) {
            curSt.curDicName = (char*)"/";
            if (PathList_head->next != PathList_tail) {
                PathList_tail->prior->next = NULL;
                PathList* temp = PathList_tail;
                PathList_tail = PathList_tail->prior;
                delete temp;
            }
        } else {
            PathList_tail->prior->next = NULL;
            PathList* temp = PathList_tail;
            PathList_tail = PathList_tail->prior;
            delete temp;  // ?
            curSt.curDicName = PathList_tail->name;
        }
    } else {
        // forward
        curSt.curDicName = (char*)d->DIR_Name;
        PathList_tail->next = new PathList;
        PathList_tail->next->prior = PathList_tail;
        strcpy(PathList_tail->next->name, curSt.curDicName);
        PathList_tail = PathList_tail->next;
    }
}
int open1(DPTR disk, char name[]) {
    dir* d = get_dir_by_name(disk, name);
    if (d == NULL)
        return -1;
    if (d->DIR_Attr[0] & ATTR_DIC) {
        printf(RED "open:[Error] %s is not a file\n" RESET, name);
        return -1;
    }
    int fd = get_free_fd();
    if (fd == FD_FULL) {
        printf(RED "open:[Error] fd are full\n" RESET, name);
        return -1;
    }
    curFd[fd].fd_active = 1;
    curFd[fd].fd_dir = d;
    return fd;
}

void write1(DPTR disk, int fd, void* buff, u32 count, u32 offset = 0) {
    if (fd == -1) {
        printf(RED "write:[Error] fd == -1\n" RESET);
        return;
    }

    dir* d = curFd[fd].fd_dir;
    BlkID now_blk = d->DIR_FstBlock;
    u32 size1 = max(offset + count, d->DIR_FileSize);
    d->DIR_FileSize = size1;
    int blkNum = size1 / BLOCK_SIZE + size1 % BLOCK_SIZE ? 1 : 0;
    u32 cur = offset;

    for (int i = 0; i < blkNum; i++) {
        if (cur >= BLOCK_NUM) {
            if (get_next_blk(disk, now_blk) == -1) {
                // 没块了
                BlkID fb = get_free_block();
                Wrblk2bm(bm, fb, 1);
                set_FAT(disk, fb, FAT_END);
                set_FAT(disk, now_blk, fb);
                cur -= BLOCK_NUM;
                now_blk = fb;
            } else {
                now_blk = get_next_blk(disk, now_blk);
                cur -= BLOCK_NUM;
            }
        } else {
            // 就从这个块开始
            block* b = &disk[now_blk];

            while (cur < BLOCK_NUM && count > 0) {
                b->a[cur++] = *(u8*)buff;
                buff = (u8*)buff + 1;
                count--;
            }
            if (count == 0)
                return;
            cur = 0;
            if (get_next_blk(disk, now_blk) == -1) {
                // 没块了
                BlkID fb = get_free_block();
                Wrblk2bm(bm, fb, 1);
                set_FAT(disk, fb, FAT_END);
                set_FAT(disk, now_blk, fb);
                now_blk = fb;
            } else {
                now_blk = get_next_blk(disk, now_blk);
            }
        }
    }
}
void read1(DPTR disk, int fd, void* buff, u32 count, u32 offset = 0) {
    if (fd < 0 || fd > FD_SIZE) {
        printf(RED "read:[Error] fd == %d\n" RESET, fd);
        return;
    }

    dir* d = curFd[fd].fd_dir;
    BlkID now_blk = d->DIR_FstBlock;
    if (offset >= d->DIR_FileSize) {
        printf(RED "read:[Error] offset invalid\n" RESET);
        return;
    }
    if (offset + count >= d->DIR_FileSize)
        count = d->DIR_FileSize - offset;

    u32 cur = offset;
    int blkNum =
        (offset + count) / BLOCK_SIZE + (offset + count) % BLOCK_SIZE ? 1 : 0;
    for (int i = 0; i < blkNum; i++) {
        if (cur >= BLOCK_NUM) {
            now_blk = get_next_blk(disk, now_blk);
            cur -= BLOCK_NUM;
        } else {
            // 就从这个块开始
            block* b = &disk[now_blk];

            while (cur < BLOCK_NUM && count > 0) {
                *(u8*)buff = b->a[cur++];
                buff = (u8*)buff + 1;
                count--;
            }
            if (count == 0)
                return;
            cur = 0;
            now_blk = get_next_blk(disk, now_blk);
        }
    }
}
void close1(DPTR disk, int fd) {
    if (fd < 0 || fd >= FD_SIZE) {
        printf(RED "close:[Error] fd %d invalid\n" RESET, fd);
        return;
    }
    curFd[fd].fd_active = 0;
}
int find_usr_by_name(DPTR disk, char usrname[]) {
    user* u = (user*)&disk[fsi->FSI_UserEntBlk];
    for (int i = 0; *(u->USER_NAME); i++) {
        if (!strcmp(usrname, u->USER_NAME))
            return i;
        u++;
    }
    return -1;
}
void login(DPTR disk, char usrname[], char pwd[]) {
    user* u = (user*)&disk[fsi->FSI_UserEntBlk];
    int id = find_usr_by_name(disk, usrname);
    if (id == -1) {
        printf(RED "login:[Error] Could not find user %s\n" RESET, usrname);
        return;
    }
    if (!strcmp(u[id].USER_PWD, pwd)) {
        printf(YEL "Login success!\n" RESET);
        curSt.userID = id;
        curSt.username = u[id].USER_NAME;
    } else {
        printf(YEL "Login fail!\n" RESET);
    }
}
int au_middleware(DPTR disk, u8 op, dir* d) {
    u8 attr = d->DIR_Attr[curSt.userID];
    if (attr & op)
        return 1;
    else {
        printf(RED "Permission denied\n" RESET);
        return 0;
    }
}
void chmod1(DPTR disk, char cmd[], char name[]) {
    dir* d = get_dir_by_name(disk, name);
    if (d == NULL) {
        printf(RED "chmod:[Error] Could not find %s" RESET "\n", name);
        return;
    }
    // r w x
    u8 ans_attr = 0;
    switch (cmd[1]) {
        case 'r':
            ans_attr |= ATTR_READ;
            break;
        case 'w':
            ans_attr |= ATTR_WRITE;
            break;
        case 'x':
            ans_attr |= ATTR_EXEC;
            break;
        default:
            printf(RED "chmod:[Error] Parameter error" RESET "\n", name);
            return;
            break;
    }
    if (cmd[0] == '+') {
        d->DIR_Attr[curSt.userID] |= ans_attr;
    } else if (cmd[0] == '-') {
         d->DIR_Attr[curSt.userID] &= ~ans_attr;
    } else {
        printf(RED "chmod:[Error] Parameter error" RESET "\n", name);
        return;
    }
}

int main() {
    assert(sizeof(struct block) == 512);   // defensive
    assert(sizeof(struct dir) == 32);      // defensive
    assert(sizeof(struct fsInfo) == 512);  // defensive
    assert(sizeof(struct user) == 32);     // defensive

    // 创建一个 文件 来模拟磁盘
    DPTR disk = (BLOCK)malloc(BLOCK_NUM * sizeof(block));  // 创建BLOCK_NUM个块

    // fat init
    init_FSI(disk);
    init_FAT(disk);
    init_BitMap(disk);
    init_rootDir(disk);
    init_User(disk);
    // Wrblk2bm(bm,0,0);
    // mkdir(disk,(char *)"haha");
    PathList_head = new PathList;
    PathList_tail = new PathList;
    PathList_head->next = new PathList;
    PathList_head->next->prior = PathList_head;
    strcpy(PathList_head->next->name, (char*)"/");
    PathList_tail = PathList_head->next;

    curSt.username = "root";
    curSt.curDicBlk = fsi->FSI_RootEntBlk;
    curSt.curDicName = (char*)"/";
    // mkdir(disk, (char*)"haha");
    // for (int i = 1; i <= 50; i++) {
    //     char s[5];
    //     sprintf(s, "hh%d", i);
    //     mkdir(disk, s);
    // }
    // for (int i = 1; i <= 20; i++) {
    //     char s[10];
    //     sprintf(s, "nima%d.c", i);
    //     create(disk, s);
    // }

    // cout << get_fin_blk(disk, 128) << endl;
    // delete1(disk, "hh19");

    // cd(disk, (char*)"nima20.c");
    // mkdir(disk, "hh19");
    // cd(disk, (char*)"hh19");
    // cd(disk, (char*)"..");

    // int t = open1(disk, "nima19.c");
    // write1(disk, t, (void*)"nimasilea", sizeof("nimasilea"));
    // char tt[20] = {};
    // read1(disk, t, tt, 15);
    // printf("%s\n", tt);
    // show_dir(disk);
    create(disk, "1.txt");
    open1(disk, "1.txt");
    write1(disk, 0, (void*)"hahah", 6, 0);
    write1(disk, 0, (void*)"Last Line", 10, 754);
    char ccc[2048] = {};
    read1(disk, 0, ccc, 1000, 0);
    for (int i = 0; i < 1000; i++)
        putchar(ccc[i]);
    puts("");

    while (1) {
        printf("[%s@しろはOS %s]# ", curSt.username, curSt.curDicName);
        char cmd[10];
        scanf("%s", cmd);
        if (!strcmp(cmd, "dir")) {
            show_dir(disk);
        } else if (!strcmp(cmd, "exit")) {
            break;
        } else if (!strcmp(cmd, "cd")) {
            char c[10];
            scanf("%s", c);
            cd(disk, c);
        } else if (!strcmp(cmd, "mkdir")) {
            char c[10];
            scanf("%s", c);
            mkdir(disk, c);
        } else if (!strcmp(cmd, "create")) {
            char c[10];
            scanf("%s", c);
            create(disk, c);
        } else if (!strcmp(cmd, "delete")) {
            char c[10];
            scanf("%s", c);
            delete1(disk, c);
        } else if (!strcmp(cmd, "open")) {
            char c[10];
            scanf("%s", c);
            int fd = open1(disk, c);
            printf("open(%s) = %d\n", c, fd);
        } else if (!strcmp(cmd, "write")) {
            char c[2048] = {};
            int a, b, d;
            scanf("%d%s%d%d", &a, c, &b, &d);
            if (au_middleware(disk, ATTR_WRITE, curFd[a].fd_dir))
                write1(disk, a, c, b, d);
        } else if (!strcmp(cmd, "read")) {
            char c[2048] = {};
            int a, b, d;
            scanf("%d%d%d", &a, &b, &d);
            if (au_middleware(disk, ATTR_READ, curFd[a].fd_dir)) {
                read1(disk, a, c, b, d);
                for (int i = 0; i < b; i++)
                    putchar(c[d + i]);
                puts("");
            }

        } else if (!strcmp(cmd, "close")) {
            int a;
            scanf("%d", &a);
            close1(disk, a);
        } else if (!strcmp(cmd, "bm")) {
            show_bitmap();
        } else if (!strcmp(cmd, "login")) {
            char a[10] = {};
            char b[10] = {};
            scanf("%s%s", a, b);
            login(disk, a, b);
        } else if (!strcmp(cmd, "chmod")) {
            char a[10] = {};
            char b[10] = {};
            scanf("%s%s", a, b);
            chmod1(disk, a, b);
        }else {
            printf(RED "bash:[Error] %s unknown command\n" RESET, cmd);
        }
    }

    /* map memory into file */
    FILE* output = fopen("t", "wb");
    if (output == NULL) {
        cout << " err" << endl;
        exit(0);
    }
    fwrite(disk, BLOCK_NUM * sizeof(block), 1, output);
    fclose(output);

    return 0;
}
/*
    log:
    cd 切换 .. 有点问题
    权限管理做个中间件就ok了
    嘛上面这两个弄完也差不多了
    反正没几行懒得分文件了


*/