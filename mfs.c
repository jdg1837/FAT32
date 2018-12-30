/*
	Group Member 0: Juan Diego Gonzalez German
	ID: 1001401837
	Group Member 1: Chelsea May
	ID: 1001140094	
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define WHITESPACE " \t\n"	//separators to parse cmd line
#define SLASH "/"			//separator to parse directory path

#define MAX_COMMAND_SIZE 255	// The maximum command-line size

#define MAX_NUM_ARGUMENTS 4	//The max number of arguments in a command line
#define MAX_LENGTH_PATH 15	//the max length of a directory path

FILE *fp;			//file pointer for the image
int is_open = 0;	//bool-like variable to track if there is an image open 

//Variables to hold the FAT32 system's information
char 	 VolumeName[11];
uint16_t BPB_BytesPerSec;
uint8_t  BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t	 BPB_NumFATS;
uint32_t BPB_FATSz32;

uint32_t root_offset;	//address to root directory
int current_offset;		//address to track the pwd

//structure to hold the data for a directory entry
struct __attribute__((__packed__)) DirectoryEntry
{
	char 		DIR_Name[11];
	uint8_t 	DIR_Attr;
	uint8_t		Unused1[8];
	uint16_t	DIR_FirstClusterHigh;
	uint8_t		Unused2[4];
	uint16_t	DIR_FirstClusterLow;
	uint32_t	DIR_FileSize;
};

//collection of files in a directory
struct DirectoryEntry dir[16];

void parse_input(char**, char*, int);		//foo to parse input (cmd or path) and tokenize it
void get_directory( int directory_LBA );	//fill array with information for the current directory
int16_t NextLB(uint32_t sector);			//find next block of a file
int LBAToOffset( int32_t sector );			//find the offset to arrive to an address
int compare_names(char*);					//finds directory entry with a matching name
void to_uppercase(char*);					//transform user input to an uppercase string
void format_name(int, char*);				//formats a filename to match input format

int main()
{
	//universal counters
	int i, j;
	
	//we allocate memory to hold the user input string
	char* cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

	//we loop for commands until user logs out
	while(1)
	{
		// Print out the prompt
		printf ("mfs> ");

		// Read the command from the commandline.
		// Maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		//make sure string is not empty
		//if it is, we loop again
		int is_valid = 0;

		for(i = 0; i < strlen(cmd_str) -1; i++)
			if(cmd_str[i] != ' ' && cmd_str[i] != '\t' && cmd_str[i] != '\n')
				is_valid = 1;

		if(is_valid == 0)
			continue;

		//define array for tokenized input
		char* token[MAX_NUM_ARGUMENTS];                       
	
		//call function to tokenize input string		                                   
		parse_input(token, cmd_str, 0);

		//if commands is to exit program
		//we close the file, if one is open, and exit
		if((strcmp(token[0], "exit") == 0) || strcmp(token[0], "quit") == 0)
		{
			if(is_open != 0)
			{
				fclose(fp);
			}

			return 0;
		}

		else if(strcmp(token[0], "open") == 0)
		{
			//we can only open one file at a time
			//if one is already open, we just loop for another input
			if(is_open == 1)
			{
				printf("Error: File system image already open.\n");
				continue;
			}

			//we open the image with a 'read' flag
			fp = fopen(token[1], "r");

			//the the file could not be open, we print an error message
			if(fp == NULL)
			{
				printf("Error: File system image not found\n");
			}

			//if the file was opened, we get the system's base data
			//with the offsets and sizes established by the FAT specs
			else
			{
				is_open = 1;

				fseek(fp, 71, SEEK_SET);
				fread(&VolumeName, 11, 1, fp);

				fseek(fp, 11, SEEK_SET);
				fread(&BPB_BytesPerSec, 2, 1, fp);


				fseek(fp, 13,  SEEK_SET);
				fread(&BPB_SecPerClus, 1, 1, fp);


				fseek(fp, 14,  SEEK_SET);
				fread(&BPB_RsvdSecCnt, 2, 1, fp);


				fseek(fp, 16,  SEEK_SET);
				fread(&BPB_NumFATS, 1, 1, fp);


				fseek(fp, 36,  SEEK_SET);
				fread(&BPB_FATSz32, 4, 1, fp);

				//we calculate the offset to the root directory, and move there
				root_offset = (BPB_RsvdSecCnt*BPB_BytesPerSec) + (BPB_NumFATS*BPB_FATSz32*BPB_BytesPerSec);

				get_directory(root_offset);

				//we save this root address as the pwd
				current_offset = root_offset;
			}

			continue;
		}

		//if there is no file open, the only operations we allowed
		//are open, and exit
		if(is_open == 0)
		{
			printf("Error: File system image must be opened first.\n");
			continue;
		}

		if(strcmp(token[0], "close") == 0)
		{
			//if there is no file open, we print an error message
			if(is_open == 0)
			{
				printf("Error: File system not open\n");
			}

			//We try to close file. If we fail, we print an error message
			else if(fclose(fp) != 0)
			{
				printf("Error: File system not closed\n");
			}

			//if we succed in closing file, we change var to show no file is now open
			else
			{
				is_open = 0;
			}		
		}

		//if user asks for file information
		//we print the values gotten when we opened it
		//in both decimal and hexadecimal form
		else if(strcmp(token[0], "info") == 0)
		{
			printf("BPB_BytesPerSec = %d, %x\n", BPB_BytesPerSec, BPB_BytesPerSec);
			printf("BPB_SecPerClus = %d, %x\n", BPB_SecPerClus, BPB_SecPerClus);
			printf("BPB_RsvdSecCnt = %d, %x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
			printf("BPB_NumFATS = %d, %x\n", BPB_NumFATS, BPB_NumFATS);
			printf("BPB_FATSz32 = %d, %x\n", BPB_FATSz32, BPB_FATSz32);
		}

		else if(strcmp(token[0], "stat") == 0)
		{
			//if the user tries to get specific info but does not name a file
			//we print warning and ask for input again
			if(token[1] == NULL)
			{
				printf("Error: Filename needed\n");
				continue;			
			}

			//we find what file in the directory array 
			//matches the user-give filename
			int filename_pos = compare_names(token[1]);

			//if none match, the file does not exist in the directory
			//and we print error message
			if(filename_pos == -1)
					printf("Error: File not found\n");
			
			//if the file is found, we print its Attribute flag, size, and cluster info
			else
			{	
					printf("\nAttributes:\t%x", dir[filename_pos].DIR_Attr);
					printf("\nSize:\t\t%d", dir[filename_pos].DIR_FileSize);
					printf("\nCluster\t\t%d\n", dir[filename_pos].DIR_FirstClusterLow);
			}
		}

		else if(strcmp(token[0], "read") == 0)
		{
			//if command does not match required format
			//we remind user of format and ask for input again
			if(token[1] == NULL || token[2] == NULL || token[3] == NULL)
			{
				printf("Error: Make sure command is of form: read <filename> <position> <number of bytes> \n");
				continue;			
			}

			//if the user flags are negative numbers
			//we print an error message and do not proceed
			if(token[2] < 0 || token[3] < 0)
			{
				printf("Error: offset and number of bytes must be positive");
				continue;
			}

			//we save the user flags for easier use
			int offset_in = atoi(token[2]);
			int remaining = atoi(token[3]);

			//we find the file that matches the user-given filename
			int pos = compare_names(token[1]);

			//if the file is not found, we print an error message and continue;
			if(pos == -1)
			{
				printf("Error: File not found\n");
				continue;
			}

			//if we do find the file, we check it's size and compare to user request			
			int file_size = dir[pos].DIR_FileSize;

			//if the user flags ask for info that goes off the bounds of the file
			//we print an error message and do not proceed
			if(remaining > (file_size - offset_in))
			{
				printf("Error: Values given exceed file size\n");
				continue;
			}

			//we move the filepointer to the position specified by user
			int LowCluster = dir[pos].DIR_FirstClusterLow;

			int offset = LBAToOffset(LowCluster) + offset_in;

			fseek(fp, offset, SEEK_SET);

			//if we have more than a block left to read, we read this whole block
			//print the values after buffering them
			//and find address for next block
			while(remaining >= 512)
			{
				//we buffer data and then output it to screen
            	char * ptr = (char*)malloc( 513 );
            	fread( ptr, 1, 512, fp );
				ptr[512] = '\0';
				printf("%s", ptr);
            	free( ptr );
				
				remaining -= 512;

				LowCluster = NextLB(LowCluster);
				
				//when we reach end of file, we break off loop
				if(LowCluster == -1)
					break;

				offset = LBAToOffset(LowCluster);
				fseek(fp, offset, SEEK_SET);
			}

			//if we have less than a block-worth of data to read
			//we just buffer and output the remaing bytes of data
			if(remaining > 0) 
            {
            	char * ptr = (char*)malloc( remaining + 1 );
            	fread( ptr, 1, remaining, fp );
				ptr[remaining] = '\0';
				printf("%s", ptr);
            	free( ptr );
            }				
		}

		else if(strcmp(token[0], "get") == 0)
		{
			//if the user does not specify what file to get
			//we print an error message and move one
			if(token[1] == NULL)
			{
				printf("Error: Filename needed\n");
				continue;			
			}

			//we find the file that matches the user-given filename
			int pos = compare_names(token[1]);

			//if the file is not found, we print error message and loop again
			if(pos == -1)
			{
				printf("Error: File not found\n");
				continue;
			}
			
			//we open a file, using the name the user gave
			FILE *new_fp;
			new_fp = fopen(token[1], "w");
			
			//we seek to the beginning of the file
			//and save the value of the size
			int file_size = dir[pos].DIR_FileSize;
			int LowCluster = dir[pos].DIR_FirstClusterLow;

			int offset = LBAToOffset(LowCluster);

			fseek(fp, offset, SEEK_SET);

			//while there is more than a block-worth of data left to get
			//we get a whole block
			//find address for next block
			//and track how much data is still unread
			while(file_size >= 512)
			{
				//we buffer data and then copy it to the new file
            	char * ptr = (char*)malloc( 512 );
            	fread( ptr, 1, 512, fp );
				fwrite(ptr, 1, 512, new_fp);
            	free( ptr );
				
				file_size -= 512;

				LowCluster = NextLB(LowCluster);

				//when we reach end of file, we break off loop
				if(LowCluster == -1)
					break;

				offset = LBAToOffset(LowCluster);
				fseek(fp, offset, SEEK_SET);
			}

			//if we have less than a block to get
			//we get the rest of the data
			if(file_size > 0) 
            {
            	char * ptr = (char*)malloc( file_size );
            	fread( ptr, 1, file_size, fp );
				fwrite(ptr, file_size, 1, new_fp);
            	free( ptr );
            }

			fclose(new_fp);									
		}

		else if(strcmp(token[0], "cd") == 0)
		{
			//if directory not specified, we move to root
			//using the stored offset
			if(token[1] == NULL)
			{
				get_directory(root_offset);
				current_offset = root_offset;
				continue;
			}

			//define array for tokenized input
			char* path[MAX_LENGTH_PATH];

            int length = strlen(token[1]);

			//if path does not end with the delimitator
			//we copy to a new, bigger string, and attach it
			//as the parsing function will need it
            if ( token[1][length-1] != '/')
			{
				char* path_buffer = (char*)malloc( MAX_COMMAND_SIZE );
				memcpy(path_buffer, token[1], length);
				path_buffer[length] = '/';

				parse_input(path, path_buffer, 1);

				free(path_buffer);
            }

			//if the user did put the '/'
			//we parse the given string, as is
			else
			{
				parse_input(path, token[1], 1);
			}	                                   


			//we keep track of current offset
			//should the process fail
			int buffer_offset = current_offset;

			int failed = 0;

			i = 0;

			//we try to cd to every token in the path
            while(path[i] != NULL)
            {
				const char* tilde = "~";

                if(strcmp(path[i], tilde) == 0 )
                {
					//if the path starts with "~" we have a global path
					//so we move to root and start from there
					if(i == 0)
					{
						current_offset = root_offset;
						get_directory(root_offset);
						i++;
						continue;
					}
                }

				//find the file specified in the current directory
			    int filename_pos = compare_names(path[i]);

				//if it does not exist, we mark the process as failing
				//and exit loop
				if(filename_pos == -1)
				{
					failed = 1;
					break;
				}

				else	
				{
					//if the file is found, and it is a directory
					//we move there using the cluster
					//if the cluster is 0, meaning the file points to root
					//we manually make the correction necesary (cluster = 2)
					if(dir[filename_pos].DIR_FileSize == 0) 
					{	
						int cluster = dir[filename_pos].DIR_FirstClusterLow;
						if( cluster == 0 ) 
						{
							cluster = 2;
						}
						current_offset = LBAToOffset(cluster);
						get_directory(current_offset);
						i++;
					}

					//if the file exists but it is not a directory
					//we mark the process as having failed and break out of loop
					else
					{
						failed = 1;
						break;
					}
				}
			}

			//if the process failed, we print an error message
			//and return to the directory we were initially in
			if(failed == 1)
			{
				printf("Error: Path not found\n");
				get_directory(buffer_offset);
			}
	
		}

		else if(strcmp(token[0], "ls") == 0)
		{
			int dir_changed = 0;

			//if we have a flag for the ls command
			if(token[1] != NULL)
			{
				//if the flag is ".." we try to list files in the parent directory
				//to do this, we changed to that directory and proceed
				//if there is no parent directory, we just move on
				if(strcmp(token[1], "..") == 0)
				{
					int filename_pos = compare_names(token[1]);
					if(filename_pos == -1) continue;

					int cluster = dir[filename_pos].DIR_FirstClusterLow;
					if( cluster == 0 ) cluster = 2;
					get_directory(LBAToOffset(cluster));

					dir_changed = 1;
				}

				//if the flag is "." we just print contents of current directory
				//is it is something else, we print an error message
				//and move on
				else if(strcmp(token[1], ".") != 0)
				{
					printf("Error: %s not a valid ls command\n", cmd_str);
					continue;
				}
			}

			//we check every file in the directory
			for(i = 0; i < 16; i ++)
			{
				//if they have been deleted, we ignore them
				if(((dir[i].DIR_Name[0] == 0xffffffe5) || 
                    (dir[i].DIR_Name[0] == 0xffffff05) ||  
                    (dir[i].DIR_Name[0] == 0xffffff00)))
                {
                    continue;
                }

				//if they have an attribute that makes them showable
				//we print the names, after NULL-terminating them
				if((dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20))
				{
					char name_buffer[12];
					memcpy(name_buffer, dir[i].DIR_Name, 11);
					name_buffer[11] = '\0';
					printf("%s\n", name_buffer);
				}
			}

			//if the directory was changed to print contents of parent directory
			//we simply change back to the pwd using the buffer variable
			if(dir_changed)
			{
				get_directory(current_offset);
			}
		}

		//If the user ask for the volume label, gotten when we oppened
		//we simply print it
		else if(strcmp(token[0], "volume") == 0)
		{
			char name_buffer[12];
			memcpy(name_buffer, VolumeName, 11);
			name_buffer[11] = '\0';
			printf("Volume Label: %s\n", name_buffer);
		}
	}
}

//This function takes the command string
//tokenizes it, using a parameter
//and puts the tokens in the token array
//the parameter to use is defined by del
//0 = use WHITESPACE
//1 = use SLASH
void parse_input(char** token, char* cmd_str, int del)
{
	int token_count = 0;

	// Pointer to point to the token
	// parsed by strsep
	char* arg_ptr;                                         
			                                   
	char* working_str  = strdup( cmd_str );                

	// we are going to move the working_str pointer so
	// keep track of its original value so we can deallocate
	// the correct amount at the end
	char* working_root = working_str;

	// Tokenize the input strings with the specified parameter
    if(del == 0)
		while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
				(token_count < MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
		
			if( strlen( token[token_count] ) == 0 )
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

    else if(del == 1)
		while ( ( (arg_ptr = strsep(&working_str, SLASH ) ) != NULL) && 
				(token_count < MAX_LENGTH_PATH))
		{
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
		
			if( strlen( token[token_count] ) == 0 )
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

	free( working_root );

	return;
}

//this function takes a filename and compares int
//after formatting to all filenames in the directory
//if there is a file with that name, its position in
//the dir array is returned
//if there is none, we return -1
int compare_names(char* filename)
{
	if(filename[0] == '\0')
		return -1;
	
	to_uppercase(filename);

	int i;

	for(i = 0; i <16; i++)
	{
		char name_buffer[13];

		format_name(i, name_buffer);

		if(strcmp(name_buffer, filename) == 0)
			return i;
	}

	return -1;
}

//this function takes user input
//and changes all lowercase characters
//to uppercase, using ASCII values
void to_uppercase(char* filename)
{
	int i;

	int filename_length = strlen(filename);

	for(i = 0;  i < filename_length; i++)
		if(filename[i] > 96 && filename[i] < 123)
			filename[i] -= 32;
}

//this functions changes filenames to match
//the user friendly format of file.ext
void format_name(int i, char name_buffer[13])
{
	int j;

	bzero(name_buffer, 13);

	for(j = 0;;  j++)
	{
		if(dir[i].DIR_Name[j] == ' ' || j == 8)
			break;					
	
		name_buffer[j] = dir[i].DIR_Name[j];
	}

	if(dir[i].DIR_Name[8] != ' ')
	{
		name_buffer[j] = '.';
		name_buffer[++j] = dir[i].DIR_Name[8];
		name_buffer[++j] = dir[i].DIR_Name[9];
		name_buffer[++j] = dir[i].DIR_Name[10];
	}

}

//given a sector, we find the next logical block that follows it
int16_t NextLB(uint32_t sector)
{
	uint32_t FAT_address = (BPB_BytesPerSec*BPB_RsvdSecCnt) + (sector * 4);
	int16_t next;
	fseek(fp, FAT_address, SEEK_SET);
	fread(&next, 2, 1, fp);
	return next;
}

//given a sector address, we transform it to an offset
//we can use to move the file pointer
int LBAToOffset( int32_t sector)
{
	return (( sector - 2 ) * BPB_BytesPerSec ) + ( BPB_BytesPerSec * BPB_RsvdSecCnt ) +
             ( BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec );
}

//given a directory address, we seek to it
//and get all the information on the directory entries
void get_directory( int directory_LBA )
{
	int i;

	fseek(fp, directory_LBA, SEEK_SET);

	for(i = 0; i < 16; i ++)
	{
		fread(&dir[i], sizeof(struct DirectoryEntry), 1, fp);
	}
}
