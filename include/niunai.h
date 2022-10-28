#ifndef BUFFER_H_
#define BUFFER_H_

#include <atomic>
#include <string>
#include <tuple>

#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

using index_t = size_t;
using pointer = void *;
using offset = long long;

#define BASE_PATH "/dev/shm/"

struct alignas(8) test_data
{
    /* data */
};

struct alignas(8) META_INFO
{
    std::atomic<index_t> write_idx;
    std::atomic<index_t> read_idx;
    std::atomic<uint64_t> flag;
};

struct alignas(8) VEC_STAT_ADDRESS
{
    std::atomic<index_t> stat;
    offset offset_;
};

class shm
{
private:
    META_INFO* meta_info;
    pointer head_address;
    pointer stat_address;
    pointer data_address;
    pointer tail_address;

    size_t capacity;
    size_t mask;
    std::string buffer_name;
    int fd_;
public:
    shm(size_t _capacity, std::string _buffer_name);
    ~shm();

    bool init();
    bool deinit();

    index_t get_write_idx();
    void commit_write(index_t idx);
    std::tuple<bool, offset> get_writeable(index_t idx);
    
    index_t get_read_idx();
    std::tuple<bool, offset> get_readable(index_t idx);
    void commit_read(index_t idx);
    std::tuple<bool, offset> get_readable_wait(index_t idx, int timeout_ms);

    void pop();
    bool empty();
    bool full();
    size_t size();
    
    pointer get_base_address() const;
};

#endif // BUFFER_H_
