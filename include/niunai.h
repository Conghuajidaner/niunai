/**
 * @copyright  
 * 
 * @brief head file
 * @author yaokun Zhai (zhaiyaokun@lixiang.com)
 * @date 2022-08-30
 */
#ifndef NIUNAI_H_
#define NIUNAI_H_

#include <string>


uint8_t NORTHING = 1 << 0;
uint8_t READING = 1 << 1;
uint8_t READFINISH = 1 << 2;
uint8_t WRITING = 1 << 3;
uint8_t WRITEFINISH = 1 << 4;

struct MetaInfo {
    std::atomic<uint16_t> element_num_;
    std::atomic<uint64_t> begin_;
    std::atomic<uint64_t> end_;
}

template<class T>
class NiuNai
{
private:
    /* data */
    bool init_();
    void deinit_();

private:
    T* address_;
    MetaInfo* meta_;
    int fd_;
    std::string buffer_name_;
    size_t buffer_size_;
    bool master_;
public:
    NiuNai() = delete;
    NiuNai(std::string name = DEFAULT, bool master = false, size_t buffer_size = 8192);
    ~NiuNai();

public:
    T pop_front();
    T front();
    void push(const T&);
    bool is_empty();
};

#endif //NIUNAI_H_
