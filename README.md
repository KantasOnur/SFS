# SFS
-----------------------------------
To compile the programs type "make".
-----------------------------------

Description:
4 simple programs that display information about the disk, list the files in the disk about various information about them, copy files in the root directory of the disk to the current directory of the machine, and finally copy any file from the machine to the disk given a path.

Important:
These programs only work for FAT12 file systems with 2880 sectors and 512 bytes for sector, they have not been tested for other types of image files.

FAT12 Disk Organization:
Disk-Sectors		Name-of-section
0			Boot Sector
1~18			FAT tables
19~32			Root Directory
33~2879			Data Area

To run the program:
	part I:
		./diskinfo <image file> | ex: ./diskinfo foo.IMA
	part II:
		./disklist <image file> | ex: ./disklist foo.IMA
	part III:
		./diskget <image file> <file to be copied> | ex: ./diskget foo.IMA foo.txt
	part IV:
		./diskput <image file> <path> <file to be copied> | ex: ./diskput foo.IMA subdir1/subdir2/ foo.pdf or
		omitting the path will search for and copy the file in the root directory | ex: ./diskput foo.IMA foo.pdf 

	Important: 
		-For running part IV there must be a space between the path and the file.
