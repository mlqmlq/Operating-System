#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>

#define stat xv6_stat  // avoid clash with host struct stat
#define dirent xv6_dirent  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#undef stat
#undef dirent

int main(int argc, char *argv[]) {
    int fd;
    // Usage is something like <my prog> <fs.img>
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
    } else {
        fprintf(stderr,"%s","Usage: xfsck <file_system_image>\n");
        exit(1);
    }

    if (fd < 0) {
        fprintf(stderr,"%s","image not found.\n");
        exit(1);
    }

    struct stat sbuf;
    fstat(fd, &sbuf);

    // mmap
    void *img_ptr = mmap(NULL, sbuf.st_size, 
        PROT_READ, MAP_PRIVATE, fd, 0);

    // Test 1: Check whether superblock is corrupted.
    struct superblock *sb = (struct superblock *) (img_ptr + BSIZE);
    if (sb->size < sb->nblocks+sb->ninodes/IPB+(sb->size+BPB-1)/BPB+1) {
        fprintf(stderr,"%s","ERROR: superblock is corrupted.\n");
        exit(1);
    }

    // Test 2: Check whether inodes are bad.
    struct dinode *dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    for (int i = 0; i < sb->ninodes; i++) {
        if (dip->type != 0 &&
            dip->type != T_DIR &&
            dip->type != T_FILE &&
            dip->type != T_DEV) {
            fprintf(stderr,"%s","ERROR: bad inode.\n");
            exit(1);
        }
        dip++;
    }

    // Test 3: Check whether address in inode is valid.
    uint first_data_block = BBLOCK(sb->size, sb->ninodes)+1;
    dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    for (int i = 0; i < sb->ninodes; i++){
        for (int j = 0; j < NDIRECT; j++) {
            if (dip->addrs[j] != 0 && (dip->addrs[j] < first_data_block ||
                dip->addrs[j] > first_data_block + sb->nblocks-1)) {
                    fprintf (stderr,"%s","ERROR: bad direct address in inode.\n");
                    exit(1);
            }
        }
        if (dip->addrs[NDIRECT] != 0 && (dip->addrs[NDIRECT] < first_data_block ||
            dip->addrs[NDIRECT] > first_data_block + sb->nblocks-1)) {
                fprintf (stderr,"%s","ERROR: bad indirect address in inode.\n");
                exit(1);
        }
        if (dip->addrs[NDIRECT] != 0) { 
            uint* cat_indirect = (uint*) (img_ptr + BSIZE * dip->addrs[NDIRECT]);
            for (int j = 0; j < BSIZE/sizeof(uint); j++) {
                if (cat_indirect[j] != 0 && (cat_indirect[j] < first_data_block ||
                    cat_indirect[j] > first_data_block + sb->nblocks-1)) {
                        fprintf(stderr,"%s","ERROR: bad indirect address in inode.\n");
                        exit(1);
                }
            }
        }
        dip++;
    }
    
    // Test 4: Check whether directories are properly formatted.
    ushort* inode_inuse = malloc(sizeof(ushort) * sb->ninodes);
    for (int i = 0; i < sb->ninodes; i++)
        inode_inuse[i] = 0;
    dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    for (int i = 0; i < sb->ninodes; i++) {
        if (dip->type == T_DIR){
            int flag = 0;
            for (int j = 0; j < NDIRECT; j++) {
                if (dip->addrs[j] == 0)
                    break;
	        struct xv6_dirent *entry = (struct xv6_dirent *)(img_ptr + 
                        dip->addrs[j] * BSIZE);
                for (int k = 0; k < BSIZE/sizeof(struct xv6_dirent); k++) {
                    if (entry == NULL)
                        break;
                    if (strcmp(entry->name, ".") == 0) {
                        flag ++;
                        if (entry->inum != (ushort)i){
                            fprintf(stderr,"%s","ERROR: directory not properly formatted.\n");
                            exit(1);
                        }
                    } else if (strcmp(entry->name, "..") == 0){
                        flag ++;
                    } else {
                        inode_inuse[entry->inum]++;
                    }
                    entry++;
                }
            }
            if (dip->addrs[NDIRECT] != 0) {
                uint* cat_indirect = (uint*) (img_ptr + BSIZE * dip->addrs[NDIRECT]);
                for (int j = 0; j < BSIZE/sizeof(uint); j++) {
                    if (cat_indirect[j] == 0)
                        break;
	            struct xv6_dirent *entry = (struct xv6_dirent *)(img_ptr + 
                            cat_indirect[j] * BSIZE);
                    for (int k = 0; k < BSIZE/sizeof(struct xv6_dirent); k++) {
                        if (entry == NULL)
                            break;
                        if (strcmp(entry->name, ".") == 0) {
                            flag ++;
                            if (entry->inum != (ushort)i){
                                fprintf(stderr,"%s","ERROR: directory not properly formatted.\n");
                                exit(1);
                            }
                        } else if (strcmp(entry->name, "..") == 0) {
                            flag ++;
                        } else {
                            inode_inuse[entry->inum] ++;
                        }
                        entry++;
                    }
                }
            }

            if (flag != 2) {
                fprintf(stderr,"%s","ERROR: directory not properly formatted.\n");
                exit(1);
            }
        }

        dip++;
    }

    // Test 7: Check whether direct address used more than once.
    // Test 8: Check file size.
    uint* inuse = malloc(sizeof(uint)*sb->size);
    for (int i = 0; i < sb->size; i++)
        inuse[i] = 0;
    dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    for (int i = 0; i < sb->ninodes; i++) {
        if (dip->type >= 1 && dip->type <= 3){
            int num_blocks = 0;
            for (int j = 0; j < NDIRECT+1; j++) {
                if (dip->addrs[j] != 0) {
                    if (j < NDIRECT)
                        num_blocks++;
                    if (inuse[dip->addrs[j]] == 1) {
                        fprintf(stderr,"%s","ERROR: direct address used more than once.\n");
                        exit(1);
                    }   
                    inuse[dip->addrs[j]] = 1;
                } else {
                    break;
                }
            }
            if (dip->addrs[NDIRECT] != 0) { 
                uint* cat_indirect = (uint*) (img_ptr + BSIZE * dip->addrs[NDIRECT]);
                for (int j = 0; j < BSIZE/sizeof(uint); j++) {
                    if (cat_indirect[j] != 0){
                        num_blocks++;
                        inuse[cat_indirect[j]] = 1;
                    } else {
                        break;
                    }
                }
            }
            if (dip->size > num_blocks*BSIZE || (int)dip->size <= (num_blocks-1)*BSIZE) {
                fprintf(stderr,"%s","ERROR: incorrect file size in inode.\n");
                exit(1);
            }
        }
        dip++;
    }
    
    // Test 5: Check whether address used by inode also marked in bitmap.
    // Test 6: Check whether bitmap marks block are actually in use
    char* bitmap_ptr = (char*)(img_ptr + BBLOCK(0,sb->ninodes) * BSIZE);
    for (int i = first_data_block; i < first_data_block+sb->nblocks; i++) {
        if (inuse[i] == 1 && ((bitmap_ptr[i/8] >> i%8) & 1) ^ 1) {
            fprintf (stderr,"%s","ERROR: address used by inode but marked free in bitmap.\n");
            exit(1);
        }
        if (inuse[i] == 0 && ((bitmap_ptr[i/8] >> i%8) & 1)) {
            fprintf(stderr,"%s","ERROR: bitmap marks block in use but it is not in use.\n");
            exit(1);
        }
    }
    
    free(inuse);

    // Test 9: Check inode marked used but not found in a directory.
    // Test 10: Check inode referred to in directory but marked free.
    // Test 11; Check reference counti for file.
    // Test 12: Check whether directory appears more than once in file system.
    inode_inuse[1]++;
    dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    dip++;
    for (int i = 1; i < sb->ninodes; i++) {
        if (dip->type == 0 && inode_inuse[i] >= 1) {
            fprintf (stderr,"%s","ERROR: inode referred to in directory but marked free.\n");
            exit(1);
        }
        if (dip->type >= 1 && dip->type <= 3){
            if (inode_inuse[i] == 0) {
                fprintf (stderr,"%s","ERROR: inode marked used but not found in a directory.\n");
                exit(1);
            }
        }
        if (dip->type == T_FILE) {
            if (inode_inuse[i] != dip->nlink) {
                fprintf (stderr,"%s","ERROR: bad reference count for file.\n");
                exit(1);
            }
        }
        if (dip->type == T_DIR) {
            if (inode_inuse[i] != 1) {
                fprintf (stderr,"%s","ERROR: directory appears more than once in file system.\n");
                exit(1);
            }
        }
        dip++;
    }
    free(inode_inuse);
    // test 13 & 14
    ushort* parent = malloc(sizeof(ushort) * sb->ninodes);
    for (int i = 0; i < sb->ninodes; i++)
        parent[i] = 0;
    dip = (struct dinode *) (img_ptr + 2 * BSIZE);
    // fill in parent array
    for (int i=0; i < sb->ninodes; i++) {
    	if (dip[i].type == 1) {
    		struct xv6_dirent *entry = (struct xv6_dirent *)(img_ptr + dip[i].addrs[0]*BSIZE);
            parent[i] = entry[1].inum;
    	}
    }
    for (int i = 2; i < sb->ninodes; i++) {
        if (dip[i].type == 1) {
            int k = 0;
            for (int l = 0; l < NDIRECT; l++) {
                if (dip[parent[i]].addrs[l] == 0)
					break;
				struct xv6_dirent *entry = (struct xv6_dirent *)(img_ptr + dip[parent[i]].addrs[l]*BSIZE);
                for (int j = 2; j < BSIZE/sizeof(struct xv6_dirent); j++) {
					if (entry[j].inum == i) {
                        k++;
					}
                }
            }
            if (k == 0) {
                fprintf (stderr,"%s","ERROR: parent directory mismatch.\n");
                exit(1);
            }
        }
    }
	for(int i = 2; i < sb->ninodes; i++) {
		if (dip[i].type == 1) {
			int k = 0;
			int index = parent[i];
			while(k < sb->ninodes) {
				if (index == 1)
					break;
				else
					index = parent[index];
				k++;
			}
			if(k == sb->ninodes) {
                fprintf (stderr,"%s","ERROR: inaccessible directory exists.\n");
				exit(1);
			}
		}
	}
    free(parent);
    return 0;
}         
    
    
    

    
    
    

