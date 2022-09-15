/**
 * @copyright 
 * 
 * @brief 
 * @author yaokun Zhai (zhaiyaokun@lixiang.com)
 * @date 2022-08-30
 */

#include <atomic>
#include <filesystem>
#include <fstream>
#include <fcntl.h>

#include <stdio.h>

#include <sys/shm.h>
#include <sys/mman.h>

#include "../include/niunai.h"


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
    const bool flag_exist = std::filesystem::exists(std::filesystem::path(DEFAULT+buffer_name_));
    
    auto fd = shm_open(buffer_name_.c_str(),  O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        printf("shm_open error code:%d\n", errno);
        return false;
    }

    // 只有master 节点对buffer 进行初始化
    if (master_) {
        auto ret = ftruncate(fd, 2 * sizeof(std::atomic<uint64_t>) + buffer_size_ * (sizeof(T) + sizeof(std::atomic<uint8_t>)));
        if (ret < 0) {
            printf("ftruncate error code:%d\n", errno);
            return false;
        }
    }

    struct stat shm_file;
    auto ret = stat(PATH+buffer_name_, &shm_file);
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
        meta_->begin_.store(0U);
        meta_->end_.store(0U);
        meta_->element_num_.store(0U);
    }

    element_size = sizeof(std::atomic<uint8_t>) + sizeof(T);

    // 初始化所有的flag位置为northing
    for (size_t idx = 0U; idx < buffer_size; ++ idx) {
        auto flag = static_cast<std::atomic<uint8_t*>> ((void*)address_ + idx * element_size);
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
    T ret;
    auto idx = meta->begin_.fetch_add();
    meta_->element_.fetch_sub();
    idx %= buffer_size_;
    {
        auto flag = static_cast<std::atomic<uint8_t*>> ((void*)address_ + idx * element_size);
        while ((*flag).store() != WRITING);
        memcpy((T*)(flag + 1), &T, sizeof(T));
        (*flag).store() != READFINISH;
    }
}

template<class T>
T Niunai<T>::front()
{
    T ret;
    auto idx = meta->begin_.load();

    meta_->element_.fetch_sub();
    idx %= buffer_size_;
    {
        auto flag = static_cast<std::atomic<uint8_t*>> ((void*)address_ + idx * element_size);
        while ((*flag).store() != WRITING);
        memcpy((T*)(flag + 1), &T, sizeof(T));
        (*flag).store() != READFINISH;
    }
    return ret;
}

template<class T>
void Niunai<T>::write_flagfile()
{
    if (meta_->element_num_ % 4000 == 0)
    {
        try {
            std::fstream op;
            op.open(PATH + buffer_name_ + FLAG);
            if (!op.is_open()) {
                printf("flag file open error!\n");
                return;
            }
            op << "1";
            op.close();
        } catch(e) {
            printf("%s", e.what());
        }
    }
    return ;
}



template<class T>
void Niunai<T>::push(const T&t)
{
    auto idx = meta_->end_.fetch_add();
    meta_->element_.fetch_add();
    idx %= buffer_size_;

    {
        auto flag = static_cast<std::atomic<uint8_t>> ((void*)address_ + idx * element_size);
        while ((*flag).store() != READING);
        memcpy((T*)(flag + 1), &T, sizeof(T));
        (*flag).store() != WRITEFINISH;
    }

    if (meta_->element_num_.load() >= buffer_size_) {
        meta_->element_num_.store(buffer_size_);
        meta_->begin_.store(meta_.end_.load() + 1);
    }

    write_flagfile();
    return ;
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
    return meta_->element_num_.load() == 0;
}

template<class T>
uint64_t Niunai<T>::element_num()
{
    return meta_->element_num_;
}