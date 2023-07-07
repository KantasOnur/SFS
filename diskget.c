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

/* inName: name of the file
 * name: name of the other file
 * if case insensetive comprasion of the 
 * files of the names are the same then 
 * returns 1
 * returns 0 otherwise
 */
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
 * inName: name of the file given in 
 * the input
 * size: size of the file
 * flc: first logical cluster of the file
 * 
 * updates the size and flc of the given file
 * if file does not exist then the size is updated
 * to -1	
 */
void getFileInfo(char *disk, char *inName, int *size, int *flc){


	for(int i = 0; i < 14*512; i+=32){
		char *name = getName(disk+i);
		if(sameFile(inName, name) && !(disk[11] & 0x10)){
			int temp = (disk[31+i]<<(8*3)) + (disk[30+i]<<(8*2)) + (disk[29+i]<<8) + disk[28+i];
			int tempflc = (disk[27+i]<<8) + disk[26+i];
			free(name);
			*size = temp;
			*flc = tempflc;
			return;
		}
		free(name);
	}
	*size = -1;
}

/* disk: memory mapped image file
 * write: memory mapped output file
 * size: size of the file
 * flc: first logical cluster of the file
 */
void diskget(char *disk, char *write, int size, int flc){

	int writingBytes = 0;
	while(flc < 0xff8){
		int offset = 512*(flc+31);
		for(int i = 0; i < 512 && (size != writingBytes); i++){
			write[writingBytes++] = (disk+offset)[i];
		}
		flc = getFatValue(disk,flc);
	}

}

int main(int argc, char **argv){

	if(argc != 3){
		printf("Provide a filename.\n");
		return 0;
	}
	int file;
	if((file = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR)) < 0){
		printf("failed to read the disk.\n");
		close(file);
		return 0;		
	}
	

	struct stat sb;
	if(fstat(file,&sb) == -1){
		printf("Error\n");
		return 0;
	}

	char *disk = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	int size;
	int flc;
	getFileInfo(disk+(19*512), argv[2], &size, &flc);

	if(size > -1){
			
		int outfile = open(argv[2], O_RDWR | O_CREAT, 0666);
		ftruncate(outfile, size);
		char *write = mmap(NULL, size, PROT_WRITE, MAP_SHARED, outfile, 0);
		diskget(disk, write, size, flc);
		munmap(write, size);	
		close(outfile);
		
	}
	else{
		printf("file not found.\n");
	}
	
	munmap(disk, sb.st_size);
	close(file);
	return 1;
}
