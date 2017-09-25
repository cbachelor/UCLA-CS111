//NAME: Connor Borden Chris Bachelor
//EMAIL: connorbo97@g.ucla.edu cbachelor@ucla.edu
//ID: 004603469 004608570
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "ext2_fs.h"

const int READ_SIZE = 1025;

struct ext2_super_block block;

int blockSize = -1;
int imagefd = -1;
char *buf = NULL;
int blocksLeft = 0;
int inodesLeft = 0;

void exitFree(int sig)
{
	exit(sig);
}

void errorMessage(char * message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}


void superBlock()
{
	int readSize =  pread(imagefd, (void *) &block, sizeof(struct ext2_super_block), 1024);
	if(readSize != sizeof(struct ext2_super_block))
	{
		errorMessage( "Could not read super block");
	}
	blockSize = 1024 << block.s_log_block_size;
	printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", block.s_blocks_count,block.s_inodes_count, blockSize, block.s_inode_size,  block.s_blocks_per_group, block.s_inodes_per_group, block.s_first_ino);
	blocksLeft = block.s_blocks_count;
	inodesLeft = block.s_inodes_count;
}

void bfree(int offset)
{
	char* bitmap = malloc(sizeof(char) * block.s_inode_size);
	if(bitmap == NULL)
	{
		errorMessage("Failed during malloc");
	}
	int readSize =  pread(imagefd, (void *) bitmap, block.s_inode_size, offset);
	int i;
	int j;
	for(i =0; i < block.s_inode_size; i++)
	{
		for(j=0; j < 8; j++)
		{
			if(!((*(bitmap + i)  >> j) & 1))		//if the bit is 0
			{
				printf("BFREE,%d\n", i*8 + j + 1);	
			}	
		}
	}
	free(bitmap);
}

void ifree(int offset)
{
	char* bitmap = malloc(sizeof(char) * block.s_inode_size);
	if(bitmap == NULL)
	{
		errorMessage("Failed during malloc");
	}
	int readSize =  pread(imagefd, (void *) bitmap, block.s_inode_size, offset);
	int i;
	int j;
	for(i =0; i < block.s_inode_size; i++)
	{
		for(j=0; j < 8; j++)
		{
			if(!((*(bitmap + i)  >> j) & 1))		//if the bit is 0
			{
				printf("IFREE,%d\n", i*8 + j + 1);	
			}	
		}
	}
	free(bitmap);
}

int findNextBlock(__u32 block[], int* blockPosition, int* directBlockOffset, int* indirectBlockOffset, int* doubleIndirectBlockOffset)
{
	int a;
	int b;
	int c;
	int pos = 0;
	int readSize = 0;
	while(*blockPosition < 15)
	{
		while(*blockPosition < 12)
		{
			if(block[*blockPosition] != 0)
			{
				int pos = block[*blockPosition];
				*blockPosition +=1;
				return pos;	
			}
			*blockPosition +=1;
		}
		
		if(*blockPosition == 12)
		{
			if(block[*blockPosition] != 0)
			{
				for(a = *directBlockOffset; a < blockSize/4; a++)
				{
					readSize = pread(imagefd, (void *) &pos, 4, block[*blockPosition] * blockSize + a * 4);
					if(pos != 0)
					{
						*directBlockOffset = a + 1;
						return pos;
					}
				}
				*blockPosition += 1;
			}
			else
			{
				*blockPosition += 1;
			}
		}
		
		if(*blockPosition == 13)
		{
			if(block[*blockPosition] != 0)
			{
				for(a = *indirectBlockOffset; a < blockSize/4; a++)
				{
					readSize = pread(imagefd, (void *) &pos, 4, block[*blockPosition] * blockSize + a * 4);
					if(pos != 0)
					{
						for(b = *directBlockOffset; b < blockSize/4; b++)
						{
							int dPos = 0;
							readSize = pread(imagefd, (void *) &dPos, 4, block[pos] *blockSize + b * 4);
							if(dPos != 0)
							{
								*indirectBlockOffset = a;
								*directBlockOffset = b + 1;
								return dPos;
							}
						}
						*directBlockOffset = 0;
					}
				}
				*indirectBlockOffset = 0;
				*blockPosition += 1;
			}
			else
			{
				*indirectBlockOffset = 0;
				*directBlockOffset = 0;
				*blockPosition +=1;
			}
			
		}
		
		if(*blockPosition == 14)
		{
			if(block[*blockPosition] != 0)
			{
				for(a = *doubleIndirectBlockOffset; a < blockSize/4; a++)
				{
					readSize = pread(imagefd, (void *) &pos, 4, block[*blockPosition] * blockSize + a * 4);
					if(pos != 0)
					{
						for(b = *indirectBlockOffset; b < blockSize/4; b++)
						{
							int sPos = 0;
							readSize = pread(imagefd, (void *) &sPos, 4, block[pos] *blockSize + b * 4);
							if(sPos != 0)
							{
								for(c = *directBlockOffset; c < blockSize/4; c++)
								{
									int dPos = 0;
									readSize = pread(imagefd, (void *) &dPos, 4, block[sPos] *blockSize + c * 4);
									if(dPos != 0)
									{
										*doubleIndirectBlockOffset = a;
										*indirectBlockOffset = b;
										*directBlockOffset = c + 1;
										return dPos;
									}
								}
								*directBlockOffset = 0;
							}
						}
						*indirectBlockOffset = 0;
					}
				}
				*doubleIndirectBlockOffset = 0;
				*blockPosition += 1;
			}
			else
			{
				*doubleIndirectBlockOffset = 0;
				*indirectBlockOffset = 0;
				*directBlockOffset = 0;
				*blockPosition += 1;
			}
			
		}
	}
	return 0;
}

void directoryEntries(__u32 block[], int parent)
{
	int i;
	struct ext2_dir_entry diren;
	int readSize = -1;
	int direnSize = sizeof(struct ext2_dir_entry);
	int blockPosition = 0;
	int directBC = 0;
	int indirectBC = 0;
	int doubleIndirectBC = 0;
	int blockNum = findNextBlock(block, &blockPosition, &directBC, &indirectBC, &doubleIndirectBC);
	readSize = pread(imagefd, (void *) &diren, direnSize, blockNum * blockSize );

	i = 0;
	int j = 0;
	int k;
	int blockStart = blockNum * blockSize;
	int offset = 0;
	char * test = malloc(sizeof(char)*256);
	while(readSize == direnSize && blockNum != 0)
	{
		if(diren.inode != 0)
		{
			strncpy(test, (char*) &(diren.name), diren.name_len);
			test[diren.name_len] = '\0';
			printf("DIRENT,%d,%d,%d,%u,%d,\'%s\'\n", parent, offset + j*blockSize, diren.inode, diren.rec_len, diren.name_len, test );
			offset+= diren.rec_len;
			if(offset == blockSize)
			{
				j++;
				blockNum = findNextBlock(block, &blockPosition, &directBC, &indirectBC, &doubleIndirectBC);
				blockStart = blockNum * blockSize;
				offset = 0;
			}
			readSize = pread(imagefd, (void *) &diren, direnSize, blockNum * blockSize + offset );
		}
		else
		{
			offset+= diren.rec_len;
			if(offset == blockSize)
			{	
				j++;
				blockNum = findNextBlock(block, &blockPosition, &directBC, &indirectBC, &doubleIndirectBC);
				blockStart = blockNum * blockSize;
				offset = 0;
			}
			readSize = pread(imagefd, (void *) &diren, direnSize, blockNum * blockSize + offset );
		}
	}
	free(test);
}

int indirectBlocks(int indirectBlock, int level, int parent, int fileOffset)
{
	int offset = indirectBlock * blockSize;
	int i;
	int totalBlocksFound = 0;
	if(level == 1)
	{
		int blockNum;
		int readSize = pread(imagefd, (void*) &blockNum, 4, offset);
		for(i = 0; i < blockSize / 4 && readSize == 4; i++)
		{
			if(blockNum != 0)
			{
				printf("INDIRECT,%d,%d,%d,%d,%d\n", parent, level, fileOffset + totalBlocksFound, indirectBlock, blockNum);
			}
			totalBlocksFound++;
			readSize = pread(imagefd, (void*) &blockNum, 4, offset + 4*(i + 1));

		}
	}
	else if(level == 2)
	{
		int blockNum;
		int readSize = pread(imagefd, (void*) &blockNum, 4, offset);
		for(i = 0; i < blockSize / 4  && readSize == 4; i++)
		{
			if(blockNum != 0)
			{
				printf("INDIRECT,%d,%d,%d,%d,%d\n", parent, level, fileOffset + totalBlocksFound, indirectBlock, blockNum);
				totalBlocksFound += indirectBlocks(blockNum, 1, parent, fileOffset + totalBlocksFound);
			}
			else
			{
				totalBlocksFound += blockSize/4;
			}
			readSize = pread(imagefd, (void*) &blockNum, 4, offset + 4*(i + 1));
		}
	}
	else if(level == 3)
	{
		int blockNum;
		int readSize = pread(imagefd, (void*) &blockNum, 4, offset);
		for(i = 0; i < blockSize / 4  && readSize == 4; i++)
		{
			if(blockNum != 0)
			{
				printf("INDIRECT,%d,%d,%d,%d,%d\n", parent, level, fileOffset + totalBlocksFound, indirectBlock, blockNum);
				totalBlocksFound += indirectBlocks(blockNum, 2, parent, fileOffset + totalBlocksFound );
			}
			else
			{
				totalBlocksFound += blockSize/4 * blockSize/4;
			}
			readSize = pread(imagefd, (void*) &blockNum, 4, offset + 4*(i + 1));
		}
	}
	else
	{
		errorMessage("Error in indirect block function. Received non 1,2,3 level.");
	}
	return totalBlocksFound;

}

void inodeTable(int offset, int numInodes)
{
	struct ext2_inode inode;
	int i;
	int a = 0;
	for(i =0; i < numInodes; i++)
	{
		int readSize = pread(imagefd, (void *)&inode, block.s_inode_size, offset + block.s_inode_size * i);
		if(inode.i_mode != 0 && inode.i_links_count != 0)
		{
			char type;
	//		printf("\nMODE:%x\n", inode.i_mode);
	//		printf("S:%x F:%x D:%x\n", inode.i_mode & 0xA000, inode.i_mode & 0x8000, inode.i_mode & 0x4000);
			if((inode.i_mode & 0xA000)>>24 == 10)
			{
			    type = 's';
			}
			else if (inode.i_mode & 0x8000)
			{
			    type = 'f';
			}
			else if (inode.i_mode & 0x4000)
			{
			     type = 'd';
			}
			else
			{
			     type = '?';
			}
			printf("INODE,%d,%c,%o,%d,%d,%d,", i+1, type, inode.i_mode&0xFFF, inode.i_uid, inode.i_gid, inode.i_links_count);
			time_t ctime = (time_t) inode.i_ctime;
			struct tm * c = gmtime(&ctime);
			printf("%02d/%02d/%02d %02d:%02d:%02d,", c->tm_mon + 1, c->tm_mday, c->tm_year%100, c->tm_hour, c->tm_min, c->tm_sec);
			time_t mtime = (time_t) inode.i_mtime;
			struct tm * m = gmtime(&mtime);
			printf("%02d/%02d/%02d %02d:%02d:%02d,", m->tm_mon + 1, m->tm_mday, m->tm_year%100, m->tm_hour, m->tm_min, m->tm_sec);
			time_t atime = (time_t) inode.i_atime;
			struct tm * a = gmtime(&atime);
			printf("%02d/%02d/%02d %02d:%02d:%02d,", a->tm_mon + 1, a->tm_mday, a->tm_year%100, a->tm_hour, a->tm_min, a->tm_sec);
			printf("%d,%d,", inode.i_size, inode.i_blocks);
			int j;
			int directBlockCount = 12;
			for(j = 0; j < 14; j++)
			{
				printf("%d,", inode.i_block[j]);
			}
			printf("%d\n", inode.i_block[14]);
			if(type == 'd')
			{
				directoryEntries(inode.i_block, i + 1);
			}
			
			
			int singleIndirectBlockCount = 0;
			int doubleIndirectBlockCount = 0;
			if(inode.i_block[12] != 0)
			{
				singleIndirectBlockCount = indirectBlocks(inode.i_block[12] , 1, i + 1, directBlockCount);
			}
			if(inode.i_block[13] != 0)
			{
				doubleIndirectBlockCount = indirectBlocks(inode.i_block[13] , 2, i + 1, directBlockCount + singleIndirectBlockCount );
			}
			if(inode.i_block[14] != 0)
			{
				indirectBlocks(inode.i_block[14] , 3, i + 1, directBlockCount + singleIndirectBlockCount + doubleIndirectBlockCount );
			}
			
		}
	}
}

void blockGroupDescriptor()
{
	struct ext2_group_desc gd;
	int i;
	int gdSize = sizeof(struct ext2_group_desc);
	for(i = 0; i < blockSize / gdSize; i++)
	{
		int readSize =  pread(imagefd, (void *) &gd, gdSize, blockSize * 2 + 32 * i);
		if(readSize != gdSize)
		{
			errorMessage( "Could not read a group file descriptor");
		}
		if(gd.bg_block_bitmap != 0 && gd.bg_inode_bitmap != 0 && gd.bg_inode_table != 0)
		{
			int blocksInGroup = 0;
			int inodesInGroup = 0;
			if(blocksLeft < block.s_blocks_per_group)
			{
				blocksInGroup = blocksLeft;
				blocksLeft = 0;
			}
			else
			{
				blocksInGroup = block.s_blocks_per_group;
				blocksLeft -= block.s_blocks_per_group;
				
			}
			if(inodesLeft < block.s_inodes_per_group)
			{
				inodesInGroup = inodesLeft;
				inodesLeft = 0;
			}
			else
			{
				inodesInGroup = block.s_inodes_per_group;
				inodesLeft -= block.s_inodes_per_group;
				
			}
			printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",i, blocksInGroup, inodesInGroup,
			       gd.bg_free_blocks_count, gd.bg_free_inodes_count, gd.bg_block_bitmap,
				gd.bg_inode_bitmap, gd.bg_inode_table);

			bfree(block.s_blocks_per_group * i + blockSize * gd.bg_block_bitmap);
			ifree(block.s_blocks_per_group * i + blockSize * gd.bg_inode_bitmap);
			inodeTable(block.s_blocks_per_group * i + blockSize * gd.bg_inode_table, inodesInGroup);
		}
	}


}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "No image name given or too many arguments.\n");
		exit(1);
	}
	imagefd = open(argv[1], O_RDONLY);
	if(imagefd == -1)
	{
		fprintf(stderr, "Could not open given image\n");
		exit(2);
	}
	
	buf = malloc(sizeof(char) * 1025);
	if(buf == NULL)
	{
		fprintf(stderr, "Malloc failed\n");
		exit(1);
	}
	superBlock();
	blockGroupDescriptor();


//	handleOpt(argc, argv);
	
//	handleFlags();

	
	/*time.tv_nsec = 0;
	time.tv_sec = 0;
	if ( clock_getres( CLOCK_MONOTONIC, &time ) != 0 )
    	{
      		fprintf(stderr, "Could not get resolution of clock");
		exitFree(1);
    	}
	if ( clock_gettime( CLOCK_MONOTONIC, &time ) != 0 )
    	{
      		fprintf(stderr, "Could not get start time");
		exitFree(1);
    	}*/
	exitFree(0);
	return 0;
	
}

