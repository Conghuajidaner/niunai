/**
 * @copyright 
 * 
 * @brief 
 * @author yaokun Zhai (zhaiyaokun@lixiang.com)
 * @date 2022-08-30
 */

#include <atomic>
#include <filesystem>
#include <fcntl.h>

#include <sys/shm.h>
#include <sys/mman.h>

#include "../include/niunai.h"

#define PATH "/dev/shm/"

#define DEFAULT "test"

template<class T>
NiuNai<T>::NiuNai(std::string name, bool master, size_t buffer_size)
{
    buffer_name_ = name;
    buffer_size_ = buffer_size;
    master_ = master;
    init_();
}

template<class T>
NiuNai<T>::~NiuNai()
{
    deinit_();
}

template<class T>
bool NiuNai<T>::init_()
{
    // 判断是否第一次创建共享内存
    const bool flag_exist = std::filesystem::exists(std::filesystem::path(DEFAULT name));
    
    auto fd = shm_open(buffer_name_,  O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        printf("shm_open error code:%d\n", errno);
        return false;
    }

    // 只有master 节点对buffer 进行初始化
    if (master) {
        auto ret = ftruncate(fd, 2 * sizeof(std::atomic<uint64_t>) + buffer_size_ * (sizeof(T) + sizeof(std::atomic<uint8_t>)));
        if (ret < 0) {
            printf("ftruncate error code:%d\n", errno);
            return false;
        }
    }

    struct stat shm_file;
    auto ret = stat(PATH name, &shm_file);
    if (ret < 0)
    {
         printf("shm_file open error code:%d\n", errno);
         return false;
    }

    auto address_head = mmap(nullptr, shm_file.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address_head == nullptr) {
         printf("mmap error code:%d\n", errno);
         return false;
    }

    address_ = (T*)(address_head + 2 * sizeof(std::atomic<uint64_t>));

    if (flag_exist) {
        begin_ = (std::atomic<uint64_t>*);
        end_ =  (std::atomic<uint64_t>*) + 1;
    } else {
        begin_ = (std::atomic<uint64_t>*);
        end_ =  (std::atomic<uint64_t>*) + 1;
        begin_->store(0U);
        end_->store(0U);
    }

    for (size_t idx = 0U; idx < buffer_size; ++ idx) {
        auto flag = static_cast<std::atomic<uint8_t>> address_ + idx * (sizeof(T) + sizeof(std::atomic<uint8_t>));
        *flag = NORTHING;
    }
    return true;
}

template<class T>
void Niunai<T>::deinit_()
{

}

template<class T>
T Niunai<T>::pop_front()
{
    
}
