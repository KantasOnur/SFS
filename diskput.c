#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "a3func.h"
#include <time.h>
#include <sys/time.h>

/* destination: path of the file to be put
 * dirCount: amount of subdirectories the path has
 * returns an array of char* which consists of subdirectories
 */
char **getDirectories(char *destination, int *dirCount){

	char *token = strtok(destination, "/");
	int count = 0;
	char **directories = (char**)malloc(sizeof(char*)*1000);
	while(token != NULL){	
		directories[count] = token;
		token = strtok(NULL, "/");
		count++;
	}
	*dirCount = count;
	return directories;
}

int fileOrSubdirectory(char *disk){
        if('.' != disk[0] && 0xE5 != disk[0] && 0x0F != disk[11] && 0x00 == (disk[11]&0x02) && 0x00 == (disk[11]&0x08)){
                return 1;
        }
        return 0;
}

int sameFile(char *inName, char *name){
        if(!(strlen(inName) == strlen(name))){
                return 0;
        }
        else{
                for(int i = 0; i < strlen(name); i++){
                        if(toupper(inName[i]) != toupper(name[i])){
                                return 0;
                        }
                }
        }
        return 1;
}

/* disk: memory mapped image file
 * directories: array of char* consits of subdirectories
 * dirCount: amount of subdirectories the path has
 * flc: first logical cluster of the file
 * returns the phiysical location of the path 
 * returns -1 if the path does not exist
 */
int goToDir(char *disk, char **directories, int dirCount, int flc) {
	int count = 0;
	int offset;
	int address;
	int directoryIterate;
	while (count < dirCount) {
		int found = 0;
		while (flc < 0xFF8 && !found) {
			offset = (flc == 19) ? 512 * 19 : 512 * (flc + 31);
			directoryIterate = (flc == 19) ? (32-19)*512 : 512;

			for (int i = 0; i < directoryIterate && !found; i += 32) {
				if (fileOrSubdirectory(disk + offset)) {
					char *name = getName(disk + offset);

					if (((disk + offset)[11] & 0x10) && sameFile(name, directories[count])) {
						flc = ((disk + offset)[27] << 8) + (disk + offset)[26];
						address = (flc + 31) * 512;
						count++;
						found = 1;
					}
					free(name);
				}
				offset += 32;
			}
			
			if (!found) flc = (flc == 19) ? 0xFF8 : getFatValue(disk, flc);
		}
		if (!found) {
			return -1;
		}
	}
    return address;
}
/* disk: memory mapped image file
 * returns the first unused sector encountered
 */
int getFatIndex(char *disk){
	int i = 2;
	for(; getFatValue(disk, i); i++);
	return i;
}
/* disk: memory mapped image file
 * fileName: name of the file to be put
 * flc: first logical cluster of the file
 * fileSize: size of the file
 * updates the contents of the directory entry
 */
void updateDirEntry(char *disk, char *fileName, int flc, int fileSize){

	int i = 0;
	for(; i < 8 && fileName[i] != '.'; i++){
		disk[i] = toupper(fileName[i]);
	}
	disk[i] = ' ';

	char *ext = strchr(fileName, '.');
	ext++;
	for(int j = 0; j < 3 && ext[j]; j++){
		disk[8+j] = toupper(ext[j]);
	}

	disk[11] = 0x00;
	time_t t = time(NULL);
	struct tm *now = localtime(&t);

	// Calculate years since 1980
	int year_since_1980 = now->tm_year - 80;

	// Set the date and time fields in the disk structure
	disk[14] = 0;
	disk[15] = 0;
	disk[16] = 0;
	disk[17] = 0;

	// Set year
	disk[17] |= (year_since_1980 << 1);

	// Set month
	int month = now->tm_mon + 1;
	disk[17] |= (month >> 3);
	disk[16] |= (month << 5);

	// Set day
	int day = now->tm_mday;
	disk[16] |= (day & 0b00011111);

	// Set hour
	int hour = now->tm_hour;
	disk[15] |= (hour << 3);

	// Set minute
	int minute = now->tm_min;
	disk[15] |= (minute >> 3);
	disk[14] |= (minute << 5);
	
	
	disk[26] = flc & 0x00ff;
	disk[27] = ((flc & 0xff00)>>8);

	disk[28] = (fileSize & 0x000000ff);
	disk[29] = (fileSize & 0x0000ff00) >> 8;
	disk[30] = (fileSize & 0x00ff0000) >> 16;
	disk[31] = (fileSize & 0xff000000) >> 24;


}

/* disk: memory mapped image file
 * entry: FAT index to be updated
 * value: the updated value
 * updates the FAT value on a given entry and value
 */
void setFatValue(char *disk, int entry, int value) {
	disk += 512;

	int index = (int)((3*entry)/2);
	if(entry%2 == 0){
		disk[index] = value & 0xFF; // Write the low byte
		disk[1+index] = ((value >> 8) & 0x0F) | (disk[1+index] & 0xF0); // Write the high nibble
	}
	else{
		disk[index] = (disk[index] & 0x0F) | ((value << 4) & 0xF0); // Write the high nibble
		disk[1+index] = (value >> 4) & 0xFF; // Write the low byte
	}
}

/* disk: memory mapped image file
 * file: memory output file
 * directories: array of subdirectories
 * dirCount: amount of subdirectories
 * fileName: name of output file
 * filSize: size of the file
 * directoryIterate: bytes to be iterated
 * copies the file in the disk to the current directory of the machine
 */
void copyFile(char *disk, char *file, char **directories, int dirCount, char *fileName, int fileSize, int directoryIterate){
	
	int offset = (directoryIterate == 512) ? goToDir(disk,directories, dirCount, 19) : 512*19;

	
	if(offset == -1){
		printf("Directory not found.\n");
		return;
	}
	int current;
	int foundSpace = 0;
	int flc = (directoryIterate==512) ? (offset/512)-31 : 19;
	while(flc < 0xff8 && !foundSpace){
		if (flc != 19) offset = (flc+31)*512;
		for(int i = 0; i < directoryIterate; i+=32){
			if(!disk[offset]){
				foundSpace = 1;
				break;
			}
			offset+=32;
		}
		current = flc;
		flc = (flc == 19) ? 0xff8: getFatValue(disk,flc);
	}

	if(!foundSpace){
		printf("There is no empty directory entry available. This case is not handeled :(\n");
		return;
	}

	int bytesWritten = 0;
	current = getFatIndex(disk);
	updateDirEntry(disk+offset, fileName, current, fileSize);

	
	while (bytesWritten < fileSize) {
    		offset = 512 * (31 + current);

    		for (int i = 0; i < 512 && bytesWritten < fileSize; i++) {
        		disk[offset + i] = file[bytesWritten++];
    		}		

    		if (bytesWritten == fileSize) {
        		setFatValue(disk, current, 0xFFF);
        		break;
    		}	 
    		else{
			setFatValue(disk,current,0x31);
        		int next = getFatIndex(disk);
        		setFatValue(disk, current, next);
        		current = next;
    		}
	}
	
}

/* disk: memory mapped image file
 * fileName: name of the file to be copied
 * flc: first logical cluster
 * returns 1 if the file name exists anywhere in the disk
 * returns 0 otherwise
 */
int fileExists(char *disk, char *fileName, int flc) {
	int directoryIterate = (flc == 19) ? (32 - 19) * 512 : 512;
	int offset;
	int firstIteration = 1;
	int found = 0;

	while (flc < 0xFF8 && (flc != 19 || firstIteration) && !found) {
		offset = (flc == 19) ? 512 * 19 : 512 * (flc + 31);

		for (int i = 0; i < directoryIterate && *(disk + offset) && !found; i += 32) {
			if (fileOrSubdirectory(disk + offset)) {
				char *name = getName(disk + offset);
				int isSubdirectory = (disk + offset)[11] & 0x10;
				if (isSubdirectory) {
					int nextCluster = ((disk + offset)[27] << 8) + (disk + offset)[26];
					found = fileExists(disk, fileName, nextCluster);
				}
				else if (sameFile(name, fileName)) {
					free(name);
					found = 1;
					return found;
				}
				free(name);
			}
			offset += 32;
		}
		flc = (flc != 19) ? getFatValue(disk, flc) : 0xff8;
		firstIteration = 0;
	}
	return found;
}

int main(int argc, char **argv){

	struct stat st;
	int dirCount = 0;
	char **directories = NULL;
	int directoryIterate = 512;
	char *fileName;

	if(argc == 3){
		stat(argv[2], &st); //get file size
		directoryIterate = 14*512;
		fileName = argv[2];
	}
	else{
		stat(argv[3], &st); //get file size
		directories = getDirectories(argv[2], &dirCount);
		fileName = argv[3];
	}
	

	int fileSize = st.st_size;
	int diskfile = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
	fstat(diskfile, &st);
	char *disk = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, diskfile, 0);
	
	if(fileExists(disk, fileName, 19)){
		printf("There is a file of the same name in the disk.\n");
		munmap(disk, st.st_size);
		if(directories) free(directories);
		return 0;
	}
	struct stat st2;
	int fd;
	if((fd = open(fileName, O_RDONLY, S_IRUSR | S_IWUSR)) == -1){
		printf("File not found.\n");
		munmap(disk, st.st_size);
		if(directories) free(directories);
		return 0;
	}
	fstat(fd, &st2);
	char *file = mmap(NULL, st2.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(st2.st_size > getFreeDiskSize(disk)){
		printf("Not enough space on the disk.\n");
		munmap(file, st2.st_size);
		munmap(disk, st.st_size);

		if(directories) free(directories);
		return 0;
	}
	copyFile(disk, file, directories, dirCount, fileName, fileSize, directoryIterate);

	if(directories){
		free(directories);
	}
	munmap(file, st2.st_size);
	munmap(disk, st.st_size);
	close(diskfile);
	return 0;
}
