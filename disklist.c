#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "queue.h"
#include "a3func.h"

struct queue q;


int fileOrSubdirectory(char *disk){
        if('.' != disk[0] && 0xE5 != disk[0] && 0x0F != disk[11] && 0x00 == (disk[11]&0x02) && 0x00 == (disk[11]&0x08)){
                return 1;
        }
        return 0;
}


void printDirectory(char *directoryName){

	printf("%s\n", directoryName);
	printf("==================\n");
}

void printContents(char *disk, char type, char *name){

	int size = (disk[31]<<(8*3)) + (disk[30]<<(8*2)) + (disk[29]<<8) + disk[28];
	int year = ((disk[0x11] & 0xfe)>>1) + 1980;
	int month = ((disk[0x10] & 0xe0)>>5) + ((disk[0x11] & 0x01)<<3);
	int day = disk[0x10]&0x1f;

	int hour = (disk[0x0f]&0xf8)>>3;
	int minute = ((disk[0x0e]&0xe0)>>5) + ((disk[0x0f]&0x07)<<3);
	printf("%c %10d %20s %d/%02d/%02d %02d:%02d\n",type, size, name, year, month, day, hour, minute);
}

/* disk: memory mapped image file
 * flc: first logical cluster
 * name: name of the file or subdirectory
 *
 * prints the name of the directory and files
 * contained inside the directory recursively
 */
void diskList(char *disk, int flc, char *name){

	int directoryIterate = (flc == 19) ? 14*512 : 512;
	printDirectory(name);

	while(flc < 0xff8){

		int offset = (flc == 19) ? 512*19 : (flc+31)*512;
		for(int i = 0; i < directoryIterate && *(disk+offset); i+=32){
			if(fileOrSubdirectory(disk+offset)){
				char *name = getName(disk+offset);
				if((disk+offset)[11] & 0x10){
					int flc = ((disk+offset)[27]<<8) + (disk+offset)[26];
					printContents(disk+offset, 'D', name);
					enqueue(&q, name, flc);
				}
				else{
					printContents(disk+offset, 'F', name);
				}
				free(name);
			}
			offset += 32;
		}
		if(!isEmpty(&q)){
			printf("\n");
			struct node *cur = dequeue(&q);
			int qflc = cur->flc;
			char *qname = cur->name;
			diskList(disk, qflc, qname);
			free(cur);
		}
		flc = (flc == 19) ? 0xff8 : getFatValue(disk,flc);
	}
}

int main(int argc, char **argv){
	initQueue(&q);
	int file = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR);
	struct stat sb;
	if(fstat(file,&sb) == -1){
		printf("Error\n");
		return 0;
	}
	char *disk = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	diskList(disk, 19,"Root");
	munmap(disk, sb.st_size);
	if(!isEmpty(&q)){
		printf("anan\n");
	}
	close(file);
	return 1;
}
