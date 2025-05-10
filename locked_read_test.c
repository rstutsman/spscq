#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>

#include <sys/file.h>

#include <stdint.h>

//thread_id is the writer thread id

//copied from fxmark
uint64_t usec(void){
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
/*int file_to_read_from(int last_file_id){

	//increments to the next file in a ring 
	return (last_file_id + 1) % file_count;
}*/

int writer_thread_id = -1;
int reader_thread_id = -1;




//command line argument review: https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/
//string to int: https://www.geeksforgeeks.org/convert-string-to-int-in-c/
int main(int argc, char * argv[]){

	//first argument should be the writer thread ID
	//second argument should be the reader thread ID
	//if(argc < 3){
		//printf("NEED ARGUMENT FOR WRITER ID AND READER ID\n");
		//return -1;
	//}
	writer_thread_id = 0;//atoi(argv[1]);
	printf("writer thread id%d\n", writer_thread_id);
	reader_thread_id = 0;//atoi(argv[2]);
	printf("reader thread id%d\n", reader_thread_id);



	int buffer_file;
	int metadata_file;
	char buffer_name[11] = "x_x_buffer\0";
	char  metadata_name[13] = "x_x_metadata\0";

	//note: since this isn't actually doing any string
	//stuff, this can only make 10 files. for each writer 
	//reader pair

	//set up the buffer name
	buffer_name[0] = 48 + writer_thread_id;
	buffer_name[2] = 48 + reader_thread_id;

	//set up the metadata name
	metadata_name[0] = 48 + writer_thread_id;
	metadata_name[2] = 48 + reader_thread_id;

	
	printf("buffer file %s, metadata file %s\n", buffer_name, metadata_name);

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
		int temp_test = pwrite(metadata_file, testmetadata, sizeof(int), 0);
        if(temp_test != sizeof(int)){
            printf("failed to write");
            return 0;
        }


		flock(metadata_file, LOCK_UN);

		int i = 0;
		uint64_t start_time = usec();
		int prev = -10;


		while(i < write_count){
			int index = 0;
			int writeindex = 0;

			while(1){
				flock(metadata_file, LOCK_EX);
				//read from the metadata file
				//int readtest = pread(metadata_file, testmetadata, sizeof(int), 0);
				int writetest = pread(metadata_file, &(testmetadata[1]), sizeof(int), sizeof(int));
                if(writetest != sizeof(int)){
                    printf("failed to read");
                    return 0;
                }

				index = i; //testmetadata[0];
				writeindex = testmetadata[1];


				if(writeindex <= index){
					//wait for writer
					flock(metadata_file, LOCK_UN);
					continue;
				}else{
					break;
				}
			}
			int readcount = writeindex - index;
			readcount = 1;
			flock(buffer_file, LOCK_EX);

			if(readcount <= 0){
				break;
			}
			//printf("reading %d\n", readcount);
			//printf("write index %d\n", writeindex);
			//printf("read index %d\n", index);
			//
			for(int j = 0; j < readcount; j++){
				//TODO just do one big read instead of breaking it down, unless we cross boundaries
				int readtest = pread(buffer_file, testbuf, sizeof(int), ((index + j) % buffersize) * sizeof(int));
                if(readtest != sizeof(int)){
                    printf("failed to read");
                    return 0;
                }
				//printf("read value %d at index %ld in file \n", testbuf[0], ((index + j) % buffersize) *sizeof(int) );		
				if(testbuf[0] != prev + 10){
					printf("FAILED %d after %d", testbuf[0], prev);	
					flock(buffer_file, LOCK_UN);
					flock(metadata_file, LOCK_UN);

					return -1;
				}
				prev = testbuf[0];
			}


			//update the metadata read amount (allowing writer to proceed)
			i += readcount;
			testmetadata[0] = i;


			//update the metadata path, allowing write path to proceed
			ssize_t metadata_test = pwrite(metadata_file, testmetadata, sizeof(int), 0);
            if(metadata_test != sizeof(int)){
                printf("failed to write");
                return 0;
            }


			flock(buffer_file, LOCK_UN);
			flock(metadata_file, LOCK_UN);


		}
		uint64_t end_time = usec();

		uint64_t time_dif = end_time - start_time;


		printf("elapsed time %lu\n", (unsigned long)time_dif);
		//writer can write until the difference between them and the reader
		//is at most the length of the file - 1



	}else{
		printf("exiting");
	}
}

