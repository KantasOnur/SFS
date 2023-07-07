#include <stdlib.h>
#include <stdio.h>
#include "a3func.h"

/* disk: memory mapped image file
 * entry: index of the FAT table
 * returns the FAT value on a given entry
 */
int getFatValue(char *disk, int entry){

        disk += 512;

        int byte;
        int nibble;
        int entryValue;
        int index = (int)((3*entry)/2);
        if(entry%2 == 0){
                nibble = disk[1+index] & 0x0F;
                byte = disk[index] & 0xFF;
                entryValue = (nibble<<8)+byte;
        }
        else{
                nibble = disk[index] & 0xF0;
                byte = disk[1+index] & 0xFF;
                entryValue = (byte<<4) + (nibble>>4);
        }
        return entryValue;
}

/* disk: memory mapped image file
 * returns the free size of the disk
 */
int getFreeDiskSize(char *disk){

        int count = 0;
        for(int i = 2; i <= 2848; i++){
                if(getFatValue(disk, i) == 0x000){
                        count++;
                }

        }
        return count*512;
}

/* disk: memory mapped image file
 * returns the name of the file subdirectory
 */
char *getName(char *disk){
        char *name = (char*)malloc(sizeof(char)*13);
        int i = 0;
        int j = 0;
        for(; i<8 && disk[i] != ' '; i++){
                name[i] = disk[i];
        }
        if(*(disk+8) != ' '){
                name[i] = '.';
                i++;
                for(; j<3 && (disk+8)[j] != ' '; j++){
                        name[i+j] = (disk+8)[j];
                }
        }
        name[i+j] = '\0';
        return name;
}

