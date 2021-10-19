#include <errno.h>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <tuple>


#include "raw_data_cache.h"
//#include "dali/image/image_factory.h"
//#include "dali/operators/decoder/host/host_decoder.h"
//#include "../reader/loader/commands.h"


using namespace std;

namespace RawDataCache {

const string prefix = "/dev/shm/RawDataCache/";  

/*
 *When the raw data is cached in other nodes, given the node_id, and the fname, read the data from that node 
 *return 1 if successfully read, otherwise, return 0
 *
 */

int ReadFromOtherNode(int node_id, std::string& fname, uint8_t * buf, unsigned long msg_size) {}


/*
 *Use a binary search to find whetehr a file is cached in a particular node
 *cache_lists maintains the files that have been cached in the corresponding node
 *return the index if successfully found, otherwise return -1
 */
int IsCachedInOtherNode(std::vector<std::vector<std::string>> &cache_lists, std::string& sample_name, int node_id){
	
	int num_nodes = cache_lists.size();
	for (int i = 0; i < num_nodes; i++){
		if (i == node_id)
			continue;
		else {
			//do a binary search on cache_list[i]
			bool found = std::binary_search (cache_lists[i].begin(), cache_lists[i].end(), sample_name);
			if (found)
				return i;
		}
	}
	return -1;
}


/*
 *Prefetch the raw images, the input is a list that maintains the cache missed items
 *this function prefetchs a chunk of data
 *
 */
void PrefetchRawData(std::vector<std::string> items_not_in_node, int start_idx, int end_idx, std::string file_root){
	for (int i = start_idx; i < end_idx; i++){
		std::string img = items_not_in_node[i];
		CacheRawDataEntry *crde = new CacheRawDataImageEntry(img);
		int ret = -1;    	
		ret = crde->CreateSegment(); 
		if(ret == -1) {
			std::cerr << "Cache for " << img << " could not be created." << std::endl; 
			return;
		}	
		ret = crde->PutCacheSimple(file_root + "/" + img); 
		if(ret == -1){
			std::cerr << "Cache for " << img << " could not be populated." << std::endl;
			return;
		}

		std::cout << "Cache written for " << img << std::endl;
		delete ce; 
	}
}

/*
void RunImpl(SampleWorkspace &ws){
  
       const auto &input = ws.Input<CPUBackend>(0);
       auto &output = ws.Output<CPUBackend>(0);
       file_name_ = input.GetSourceInfo();

       DALI_ENFORCE(input.ndim() == 1, "Input must be 1D encoded jpeg string.");
       DALI_ENFORCE(IsType<uint8>(input.type()), "Input must be stored as uint8 data.");

       std::unique_ptr<Image> img;

       try{
               img = ImageFactory::CreateImage(input.data<uint8>(), input.size(), output_type_);
               img->SetCropWindowGenerator(GetCropWindowGenerator(ws.data_idx()));
               img->SetUseFastIdct(use_fast_idct_);
               img->Decode();     
       }catch (std::exception &e) {
      
	       DALI_FAIL(e.what() + ". File: " + file_name_);
       }

       const auto decoded = img->GetImage();
       const auto shape = img->GetShape();
       
       output.Resize(shape);
       output.SetLayout("HWC");
       unsigned char *out_data = output.mutable_data<unsigned char>();
       std::memcpy(out_data, decoded.get(), volume(shape));
}


bool FileExist (const char *filename){
       struct stat   buffer;   
       return (stat (filename, &buffer) == 0);
}
*/

string CreateName(string path){
	std::replace(path.begin(), path.end(), '/', '_');	
	return path;
}

string ShmPath(string name, string prefix){
	// assumes prefix ends with '/'
	return prefix + name;
}

int OpenSharedFile(const char* path, int flags, mode_t mode){
	if (!path){
		errno = ENOENT;
	}
	
	flags |= O_NOFOLLOW | O_CLOEXEC;
	/* Disable asynchronous cancellation.  */
	int state;
	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, &state);
	int fd = open (path, flags, mode);
	if (fd == -1){
		cerr << "Cannot open shm segment" << endl;
	}
	pthread_setcancelstate (state, NULL);
	return fd;
}



int TryMkdir(const char* path, int mode){
	typedef struct stat stat_dir;
	stat_dir st;
	int status = 0;
	if (stat(path, &st) != 0) {
		if (mkdir(path, mode) != 0 && errno != EEXIST)
			status = -1;
	} else if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		status = -1;
	}

	return status;
}

/**
** ensures all directories in path exist
** We start working top-down to ensure
** each directory in path exists.
*/
int MkdirPath(const char *path, mode_t mode){
	char *pp;
	char *sp;
	int status;
	char *copypath = strdup(path);

	status = 0;
	pp = copypath;
	// Find all occurences of '/' in path : if /a/c/tmp.jpg
	// we need to mkdir(/a) and mkdir (/a/c)
	while (status == 0 && (sp = strchr(pp, '/')) != 0) {
		if (sp != pp) {
			/* Neither root nor double slash in path */
			*sp = '\0';
			if ((status = TryMkdir(copypath, mode)) < 0){
                              cerr << "Error creating directory " << copypath << endl;
                              return -1;
                        }
			*sp = '/';
		}
		pp = sp + 1;
	}
	
	if (status == 0){
		status = TryMkdir(path, mode);
                if (status < 0){
                      cerr << "Error creating dir " << path << endl;
                      return -1;
                }
        }
    
	free(copypath);
	return (status);
}


int GetFileSize(string filename){
	struct stat st;
	stat(filename.c_str(), &st);
	int size = st.st_size;
	return size;
}

CacheRawDataEntry::CacheRawDataEntry(string path){
	/* If tyhe path conatins prefix,   
	 * then the name of the segment is 
	 * path - prefix 
	 */
	name_ = path; 
	int found = name_.find(prefix); 
	if(found != string::npos){ 
		// Prefix is found in path
		name_.erase(found, prefix.length()); 
	}
	//name_ = path_;
	//std::replace( name_.begin(), name_.end(), '/', '_');
}

int CacheRawDataEntry::CreateSegment(){
	// Get the unique name for the shm segment
	
	string dir_path(name_);
        if (dir_path.find('/') != string::npos){
	     dir_path = dir_path.substr(0, dir_path.rfind("/"));
	     int status = MkdirPath((ShmPath(dir_path, prefix)).c_str(), 0777);
             if (status < 0){
                     cerr << "Error creating path " << ShmPath(dir_path, prefix) << endl;
                     return -1;
             }
        }
	
	int flags = O_CREAT | O_RDWR;
	int mode = 511;
	
	//Get the full shm path and open it
	string shm_path_name_tmp = ShmPath(name_ + "-tmp", prefix);
        
	//cout << "Shm path " << shm_path_name_tmp << endl;
	fd_ = open_shared_file(shm_path_name_tmp.c_str(), flags, mode);
        close(fd_);
	
	return fd_;
}

int CacheRawDataEntry::AttachSegment(){

	// Else, open the file without the O_CREAT
	// flags and return the fd
	
	int flags = O_RDWR;
	int mode = 511;
	string shm_path_name;

	shm_path_name = ShmPath(name_, prefix);

	fd_ = OpenSharedFile(shm_path_name.c_str(), flags, mode);	
	return fd_;
}


int CacheRawDataEntry::PutCacheSimple(string from_file){

        int bytes_to_write = GetFileSize(from_file);
        string shm_path_name = ShmPath(name_, prefix);
        string shm_path_name_tmp = ShmPath(name_+"-tmp", prefix);
        size_ = bytes_to_write;
        FILE *source, *target;
        size_t n, m;
        unsigned char buff[4096];

        //Open the tmp file holding a lock
        if ((source = fopen(from_file.c_str(), "rb")) == NULL){
             cerr << "File open error " << from_file.c_str() << endl;
             return -1;
        }

        if ((target = fopen(shm_path_name_tmp.c_str(), "wb")) == NULL){
             cerr << "File open error " << shm_path_name_tmp << endl;
             return -1;
        }
  
        do {
              n = fread(buff, 1, sizeof buff, source);
              if (n) 
		      m = fwrite(buff, 1, n, target);
              else   
		      m = 0;
        } while ((n > 0) && (n == m));
        
	if (m) {
              //flock(fileno(target), LOCK_UN);
              perror("copy");
              return -1;
        }
  
        fclose(source);
        fclose(target);
        int ret = -1;
        //rename the file

        if ((ret = rename(shm_path_name_tmp.c_str(), shm_path_name.c_str())) < 0){
                if (file_exist(shm_path_name.c_str()))
                         return size_;
                else{
                         cerr << "Caching rename failed : " << strerror(errno) << endl;
                         return -1;
                }
        }

        return size_;
}
      

int CacheRawDataEntry::PutCache(string from_file){
	int bytes_to_write = GetFileSize(from_file);
	size_ = bytes_to_write;
	
	cout << "Write from file " << from_file << " size " << bytes_to_write << endl;
	
	if (fd_ < 0){
		errno = EINVAL;
		cerr << "File " << name_ << " has invalid decriptor" << endl;
		return -1;
	}
	ftruncate(fd_, bytes_to_write);

	//mmap the file to get ptr
	void *ptr = nullptr;
	if ((ptr = mmap(0, bytes_to_write, PROT_WRITE, MAP_SHARED, fd_, 0)) == MAP_FAILED){
		cerr << "mmap error" << endl;  
		return -1;
	} 
	

	// write to memory segment 
	void *ptr_from = nullptr;
	int fd_from = -1;
	if ((fd_from = open(from_file.c_str(), O_RDONLY)) < 0) { 
		cerr << "Open failed" << endl; 
		return -1;
	}
	if ((ptr_from = mmap(0, bytes_to_write, MAP_SHARED, fd_from, 0)) == MAP_FAILED){
		cerr << "mmap error" << endl; 
		return -1;
	}
	std::shared_ptr<void> p_;
	p_ = shared_ptr<void>(ptr_from, [=](void*) {
		munmap(ptr_from, bytes_to_write); 
	});
	
	//Do the memcpy now
	// memcpy(void* dest, const void* src, size)
	memcpy(ptr, p_.get(), bytes_to_write);
	//cout << "memcpy done" << endl;
	int ret = 0;

	// unmap both files
	if ((ret = munmap(ptr, bytes_to_write)) == -1){
		cerr << "Munmap failed" << endl;
		return -1;
	}
	
	if ((ret = munmap(ptr_from, bytes_to_write)) == -1){
		cerr << "Munmap failed" << endl;
		return -1;
	}

	close(fd_from);
        
	//close the tmp file
        close(fd_);

        string shm_path_name = ShmPath(name_, prefix);
        string shm_path_name_tmp = ShmPath(name_+"-tmp", prefix);
  
	//rename the file
        if ((ret = rename(shm_path_name_tmp.c_str(), shm_path_name.c_str())) < 0){
                cerr << "Caching rename failed" << endl;
                return -1;
        }

	return bytes_to_write;
}


void* CacheRawDataEntry::GetCache(){
	string from_file = prefix + name_;
	int bytes_to_read = GetFileSize(from_file);
	size_ = bytes_to_read;
	//cout << "will read from file " << from_file << " size " << bytes_to_read << endl;

	// If the descriptor is invalid, you need to sttach the segment.
	if (fd_ < 0){
		errno = EINVAL;
		cerr << "File " << name_ << " has invalid decriptor" << endl;
		return nullptr;
	}
	//mmap the shm file to get ptr
	void *ptr = nullptr;   
	if ((ptr = mmap(0, bytes_to_read, MAP_SHARED, fd_, 0)) == MAP_FAILED){
		cerr << "mmap error" << endl;
		return nullptr;
	}

	return ptr;
}

string CacheRawDataEntry::GetShmPath(){
	string shm_path_name; 
	shm_path_name = ShmPath(name_, prefix);
	return shm_path_name;
}

int CacheRawDataEntry::CloseSegment(){
	int ret = 0;
	if (fd_ > -1){
		if (( ret = close(fd_)) < 0){
			cerr << "File " << prefix + name_ << " close failed" << endl;
			return -1;
		}
	}
	return 0;
}

int CacheRawDataEntry::RemoveSegment(){
	string shm_path_name;
	shm_path_name = ShmPath(name_, prefix); 
	int result = unlink(shm_path_name.c_str());
	return result;
}

} //end namespace
