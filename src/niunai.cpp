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

    address_ = (T*)(address_head + sizeof(MetaInfo));

    if (!flag_exist) {
        meta_.begin.store(0U);
        meta_.end.store(0U);
        meta_.element_num_.store(0U);
    }

    // 初始化所有的flag位置为northing
    for (size_t idx = 0U; idx < buffer_size; ++ idx) {
        auto flag = static_cast<std::atomic<uint8_t>> ((void*)address_ + idx * sizeof(MetaInfo));
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

template<class T>
T Niunai<T>::front()
{

}

template<class T>
void Niunai<T>::push(const T&t)
{
    auto idx = meta_.end_.fetch_add();
    meta_.element_.fetch_add();

    auto flag = address_ + 
}


/**
 * @brief 安全但是并不可靠，后面估计会删掉
 * 
 * @tparam T tyepe
 * @return true element_num_ == 0
 * @return false element_num_ != 0
 */
template<class T>
bool Niunai<T>::is_empty()
{
    return meta_.element_num_.load() == 0;
}
