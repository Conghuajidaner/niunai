#include "shm_ring_buffer.h"

#include <chrono>
#include <thread>

shm::shm(size_t _capacity, std::string _buffer_name): capacity(_capacity), buffer_name(_buffer_name)
{
    mask = capacity - 1;
    init();
}

shm::~shm()
{
    deinit();
}

bool shm::init()
{
    fd_ = shm_open(buffer_name.c_str(), O_CREAT | O_RDWR, S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR | S_IWOTH | S_IROTH);
    if (fd_ == -1)
    {
        printf("shm_open errno code: %d", errno);
        return false;
    }

    const size_t BUFFER_SIZE = sizeof(META_INFO) + capacity * (sizeof(VEC_STAT_ADDRESS) + sizeof(test_data));
    auto ret = ftruncate(fd_, BUFFER_SIZE);
    if (ret < 0)
    {
        printf("ftruncate error code%d:\n", errno);
        return false;
    }

    head_address = mmap(nullptr, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (head_address == MAP_FAILED)
    {
        printf("mmap errno code%d:\n", errno);
        return false;
    }
    tail_address = head_address + BUFFER_SIZE;
    stat_address = head_address + sizeof(META_INFO);
    data_address = stat_address + capacity * sizeof(VEC_STAT_ADDRESS);
    meta_info =  static_cast<META_INFO*>(head_address);

    if (meta_info->flag.load() != 2333333)
    {
        printf("init value\n");
        meta_info->read_idx.store(0U);
        meta_info->write_idx.store(0U);
        meta_info->flag.store(2333333U);

        for (size_t idx = 0U; idx < capacity; ++idx)
        {
            auto status = static_cast<VEC_STAT_ADDRESS *>(stat_address + idx * sizeof(VEC_STAT_ADDRESS));
            status->stat.store(idx);
            status->offset_ = idx * sizeof(test_data);
        }
    }
    return true;
}

bool shm::deinit()
{
    // northing to do
    return true;
}

index_t shm::get_write_idx()
{
    return meta_info->write_idx.fetch_add(1U, std::memory_order_relaxed);
}

void shm::commit_write(index_t idx)
{
    auto status = static_cast<VEC_STAT_ADDRESS*>(stat_address + (idx & mask) * sizeof(VEC_STAT_ADDRESS));
    status->stat.store(static_cast<index_t>(~idx), std::memory_order_release);
    // printf("status->stat.load: %zu", status->stat.load());
}

std::tuple<bool, offset> shm::get_writeable(index_t idx)
{
    auto status = static_cast<VEC_STAT_ADDRESS *>(stat_address + (idx & mask) * sizeof(VEC_STAT_ADDRESS));
    if (status->stat.load() != idx) return {false, 0};
    return {true, status->offset_};
}

index_t shm::get_read_idx()
{
    return meta_info->read_idx.fetch_add(1U, std::memory_order_relaxed);
}

std::tuple<bool, offset> shm::get_readable(index_t idx)
{
    auto status = static_cast<VEC_STAT_ADDRESS *>(stat_address + (idx & mask) * sizeof(VEC_STAT_ADDRESS));
    if (status->stat.load() != ~idx)
    {
        return {false, 0};
    }
    return {true, status->offset_};
}

void shm::commit_read(index_t idx)
{
    auto status = static_cast<VEC_STAT_ADDRESS *>(stat_address + (idx & mask) * sizeof(VEC_STAT_ADDRESS));
    status->stat.store(static_cast<index_t>(idx + capacity), std::memory_order_release);
}

std::tuple<bool, offset> shm::get_readable_wait(index_t idx, int timeout_ms)
{
    auto ret = get_readable(idx);
    if (!std::get<0>(ret)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        ret = get_readable(idx);
    }
    return ret;
}

void shm::pop()
{
    const int64_t idx = static_cast<int64_t>(get_read_idx());
    commit_read(static_cast<uint64_t>(idx));
    printf("pop: %d\n", idx);
}

bool shm::empty()
{
    return size() <= 0;
}

bool shm::full()
{
    return size() >= capacity;
}

size_t shm::size()
{
    auto w_id = meta_info->write_idx.load(std::memory_order_relaxed);
    auto r_id = meta_info->read_idx.load(std::memory_order_relaxed);
    if (w_id < r_id) {
      return 0;
    }
    return w_id - r_id;
}

pointer shm::get_base_address() const
{
    return data_address;   
}
