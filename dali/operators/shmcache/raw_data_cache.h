#ifndef RAW_DATA_CACHE_H_
#define RAW_DATA_CACHE_H_

#include <errno.h>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>


namespace RawDataDache{

extern const std::string prefix;  

/* 
 * Reads one sample from a remote node, given by the node_id.
 * Returns the number of bytes read 
 */
int ReadFromOtherNode(int node_id, std::string fname, uint8_t * buf, unsigned long msg_size);


/* Reads a particular file from other nodes */
int ReadFileFromOtherNode(int node_id, std::string fname, uint8_t * buf, unsigned long msg_size);


/* 
 * Returns the node where this sample is cached in memory
 * If not cached anywhere and needs to be fetched from one's own
 * disk, then returns -1
 */
int IsCachedInOtherNode(std::vector<std::vector<std::string>> &cache_lists, std::string sample_name, int node_id);


void PrefetchRawData(std::vector<std::string> items_not_in_node, int start_idx, int end_idx, std::string file_root);


//void RunImpl(SampleWorkspace &ws) override;


class CachedRawDataEntry {
    public:

	/* 
	 * Constructor : initialize segment using path of disk file, and populate name. 
	 * This does not open the file until create or attach segment is called.
	 */
	CachedRawDataEntry(std::string path);


	/* 
	 * Given a path to the current file that has to be cached in memory.
	 * This function returns the descriptor for the corresponding memory segment. 
	 * If the segment already exists, it returns the fd. If not, it creates and then returns the handle.
	 */
        int CreateSegment();

	/* 
	 * This function must be used when we know the memory segment exists and we want a handle to it. 
	 * If the segment does not exist, the function returns error.
	 */
	int AttachSegment();
	
	/* 
	 * Given the path to the memory segment or a name,this function returns the pointer to the
	 * contents of the file after mmap in its address space.
	 */
	void* GetCache();

	/* 
	 * Write to the memory segment that is already created and open by mmap into its address space
	 * unmaps on exit.
	 */
	int PutCache(std::string from_file);
	int PutCacheSimple(std::string from_file);

	/* 
	 * This function returns the memory path to this segment.
	 */
	std::string GetCachePath();

	/* 
	 * Returns the file size
	 */
	int GetSize() { return size_;}

	/* 
	 * This function must be called after the AttachSegment or CreateSegment
	 * It closes the opened segment, but does not delete it.
	 */
	int CloseSegment();

	/* 
	 * This function must be called to delete the cache.
	 * This must be called at the end of experiment when you want to clear off all entries
	 */
	int RemoveSegment();

	/* 
	 * name_ is the identifier of the memory segment in the path indicated by prefix
	 * To access the segment use path: prefix + name
	 */
	std::string name_;

    private:

        //fd of the open file - tmp if create-segment
	int fd_ = -1;

	int size_ = 0;

	//DALIImageType output_type_;

	bool use_fast_idct_ = false;

	std::string file_name_;
	
};

} // end namespace 

#endif
