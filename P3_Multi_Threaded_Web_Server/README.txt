/* CSci4061 F2018 Project 3 Group 80
* login: jungx457@umn.edu
* date: 12/12/18
* name: Chan Jung, Khoa Tran, Sara Nelson
* id: 5108030(jungx457),5411431(tran0707),5163126(nels8907) */

How to Compile and Run Code:
Go to correct location that program is at
Make program.
write ./web_server port path num_dispatch num_workers dynamic_flag qlen cache_entries
	in terminal with desired values of inputs noted.

How program works:
The program is executed in the terminal with the same inputs as noted in the handout. From the main function, all worker threads and dispatcher threads are created as well as a request buffer. If dispatcher thread is available, will put request in queue by locking and unlocking queue. Worker threads will then request lock for queue to look for “work.” If queue is empty, wait for dispatcher. If not empty, check cache. If file is in cache—>done and print to terminal, info in cache, check file disk, then put in cache, then print to terminal. Error checking used in code.

Cache:
Uses struct, cache_entry to keep track of cache. Initialize memory for cache using malloc. Uses function getCacheIndex to find index of request. When adding into cache,  use addIntoCache function which creates an entry at the appropriate size and frees remaining memory if replacing entry. If there is a MISS and the cache is full. Cache entry replacement is implemented using LFU method. The number of getIndex requests are kept track of using an additional variable in the struct for a cache entry. When looking for an entry to replace, program chooses entry with least number of requests.

Worker thread pool size:
The number of worker threads is an input from terminal when initially executing the program. Worker threads are created in main function using a for loop. Memory is allocated using malloc before the for loop is executed. 
