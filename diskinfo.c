#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "a3func.h"

#define SECTOR_SIZE 512
#define ROOT_SECTOR 19
#define NEXT_SUBDIRECTORY 32
#define VOLUME_LABEL 11

/* returns the os name of the disk
 * disk: memory mapped image file
 */
char *getOs(char *disk){

	char *os = (char*)malloc(sizeof(char)*9);
	disk += 3;
	for(int i = 0; i < 8; i++){
		os[i] = disk[i];	
	}
	os[8] = '\0';
	return os;
}

/* returns the label of the disk
 * disk: memory mapped image file
 */
char *getLabel(char *disk){

	char *label = (char*)malloc(sizeof(char)*12);
	disk += (SECTOR_SIZE * ROOT_SECTOR);
	while(disk[11] != 0x08){
		disk += NEXT_SUBDIRECTORY;
	}
	for(int i = 0; i < VOLUME_LABEL; i++){
		label[i] = disk[i];
	}

	label[VOLUME_LABEL] = '\0';	
	return label;
}

/* returns 1 if the given entry is a file or a subdirectory
 *         0 otherwise
 */
int fileOrSubdirectory(char *disk){
	if('.' != disk[0] && 0xE5 != disk[0] && 0x0F != disk[11] && 0x00 == (disk[11]&0x02) && 0x00 == (disk[11]&0x08)){
		return 1;
	}
	return 0;
}

/* returns the amount of files in the disk
 * disk: memory mapped image file
 * flc: first logical cluster
 */
int getFileCount(char *disk, int flc){
	int directoryIterate = (flc == 19) ? 14*512 : 512;
	int count = 0;
	while(flc < 0xff8){
       		int offset = (flc == 19) ? 512*19 : (flc+31)*512;
		for(int i = 0; i < directoryIterate && *(disk+offset); i+=32){
			if(fileOrSubdirectory(disk+offset)){
				if((disk+offset)[11] & 0x10){
					int next = ((disk+offset)[27]<<8) + (disk+offset)[26];
					count += getFileCount(disk,next);
				}
				else{
					count++;
				}
			}
			offset += 32;
		}
		flc = (flc == 19) ? 0xff8: getFatValue(disk,flc);
	}
	return count;
}

/* prints the info
 * os: os name of the disk
 * label: label of the disk
 * diskSize: total size of the disk
 * freeDiskSize: free size of the disk
 * fileCount: amount of the files in the disk
 * fatCount: amount of FAT in the disk
 * sectorsPerFat: amount of sectors per FAT
 */
void printInfo(char *os, char *label, int diskSize, int freeDiskSize, int fileCount, int fatCount, int sectorsPerFat){
	printf("OS Name: %s\n", os);
	printf("Label of the disk: %s\n", label);
	printf("Total size of the disk: %d bytes.\n", diskSize);
	printf("Free size of the disk: %d bytes.\n", freeDiskSize);
	printf("The number of files in the disk: %d\n", fileCount);
	printf("Number of FAT copies: %d\n", fatCount);
	printf("Sectors per FAT: %d\n", sectorsPerFat);
}

int main(int argc, char **argv){


	int file = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR);
	struct stat sb;
	if(fstat(file,&sb) == -1){
		printf("Error\n");
		return 0;
	}
	char *disk = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	

	char *os = getOs(disk);
	char *label = getLabel(disk);

	int sectors = (disk[20]*256) + disk[19];
	int bytesPerSector = (disk[12]*256) + disk[11];

	int diskSize = sectors * bytesPerSector;
	int freeDiskSize = getFreeDiskSize(disk);
	int fileCount = getFileCount(disk, 19);//getFileCount(disk, 512*19, 19);
	int fatCount = disk[16];
	int sectorsPerFat = (disk[23]<<8) + disk[22];
	
	printInfo(os, label, diskSize, freeDiskSize, fileCount, fatCount, sectorsPerFat);

	free(os);
	free(label);
	munmap(disk, sb.st_size);
	close(file);
}
