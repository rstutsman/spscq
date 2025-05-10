#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/file.h>


//for sleep test
#include <sys/wait.h>


int file_to_write_to(int last_file_id, int file_count){

	//increments to the next file in a ring 
	return (last_file_id + 1) % file_count;
}

int writer_thread_id = -1;
int reader_thread_id = -1;


//command line argument review: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
//string to int: https://www.geeksforgeeks.org/convert-string-to-int-in-c/
int main(int argc, char * argv[]){


	//first argument should be the writer thread ID
	//second argument should be the reader thread ID
	//if(argc < 3){
    //		printf("NEED ARGUMENT FOR WRITER ID AND READER ID\n");
    //		return -1;
	//}
	writer_thread_id = 0;//atoi(argv[1]);
	printf("writer thread id%d\n", writer_thread_id);
	reader_thread_id = 0; //atoi(argv[2]);
	printf("reader thread count%d\n", reader_thread_id);




	int buffer_file;
	int metadata_file;
	char buffer_name[11] = "x_x_buffer\0";
	char  metadata_name[13] = "x_x_metadata\0";
	
	printf("about to make files\n");

    //note: since this isn't actually doing any string
    //stuff, this can only make 10 files. for each writer 
    //reader pair

    //set up the buffer name
    buffer_name[0] = 48 + writer_thread_id;
    buffer_name[2] = 48 + reader_thread_id;

    printf("set up buffer name \n");

    //set up the metadata name
    metadata_name[0] = 48 + writer_thread_id;
    metadata_name[2] = 48 + reader_thread_id;

    buffer_file = open(buffer_name, O_CREAT | O_RDWR, 0666);
    printf("file decriptor is %d error %d \n", buffer_file, errno);
    if(buffer_file == -1){
        printf("failed to create file");
        return -1;
    }

    metadata_file = open(metadata_name, O_CREAT | O_RDWR, 0666);
    printf("file decriptor is %d error %d \n", metadata_file, errno);
    if(metadata_file == -1){
        printf("failed to create file");
        return -1;
    }








	int buffersize = 1000;
	int write_count = 1000000;

	if (1 > 0){

		//write path

		int testbuf[4] = {1234, 5678, 9101, 1121};
		int testmetadata[4] = {0, 1, 2, 3};

		//set metadata to start at 0
		flock(metadata_file, LOCK_EX);
	
		//set up files	
        int test = pwrite(metadata_file, testmetadata, sizeof(int), 0); //read data
        if(test != sizeof(int)){
            printf("failed to write");
            return 0;
        }

        if(test <= -1){
            printf("failed to setup file %d error %d\n", metadata_file, errno);
        }else{
            printf("set up metadata file test 1");
        }
        test = pwrite(metadata_file, testmetadata, sizeof(int), sizeof(int) * 1); //write data
        if(test != sizeof(int)){
            printf("failed to write");
            return 0;
        }
        if(test <= -1){
            printf("failed to setup file %d again error %d\n", metadata_file, errno);
        }else{
            printf("set up metadata file test 2"); 
        }

		flock(metadata_file, LOCK_UN);

		//only write into the first 10 items in the page


		int current_file_id = 0;
        int writeindex = 0;
		for(int i = 0; i < write_count; i++){

			//current_file_id = file_to_write_to(current_file_id, reader_thread_count);
            current_file_id = 0;

			//printf("for loop\n");
			//read from the metadata file		
			int index = 0;	
			while(1){
				flock(metadata_file, LOCK_EX);

				int readtest = pread(metadata_file, testmetadata, sizeof(int), 0);
                if(readtest != sizeof(int)){
                    printf("failed to read");
                    return 0;
                }
				//int writetest = pread(metadata_file, &(testmetadata[1]), sizeof(int), sizeof(int));

				index = testmetadata[0];
				//writeindex = testmetadata[1];
				if(writeindex >= index + (buffersize-1) ){
					//printf("write index is %d, read index is %d", writeindex, index);
					//stop because we have to wait for the reader
					flock(metadata_file, LOCK_UN);
					continue;
				}else{
					break;
				}
			}
			testbuf[0] = i * 10;
			flock(buffer_file, LOCK_EX);

			//write the index into the buffer
			ssize_t write_test = pwrite(buffer_file, testbuf, sizeof(int), (writeindex % buffersize) * sizeof(int));
            if(write_test != sizeof(int)){
                printf("failed to write");
                return 0;
            }

            //printf("locked wrote %d index %d into file %d\n", 10, 10, 10);
			//printf("locked wrote %d index %d into file %d\n", i * 10, (writeindex %buffersize) * sizeof(int), current_file_id);
            //printf("locked wrote %d index %d into file %d\n", 10, 10, 10);
            //printf("hello\n");
			//write and update the write index
            writeindex += 1;
			testmetadata[1] = writeindex;
			//update the metadata path, allowing read path to proceed 
			ssize_t metadata_test = pwrite(metadata_file, &(testmetadata[1]), sizeof(int), sizeof(int));
            if(metadata_test != sizeof(int)){
                printf("failed to write");
                return 0;
            }
		

			flock(metadata_file, LOCK_UN);
			flock(buffer_file, LOCK_UN);

		//sleep(.1);


		}


	//writer can write until the difference between them and the reader
	//is at most the length of the file - 1



	}else{
		printf("exiting");
	}
}

