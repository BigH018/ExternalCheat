#include <cstdint>

#include <Windows.h>

#include <doctest.h>

#include "core/process_memory.h"

// ProcessMemory against our OWN process, through the same RPM/WPM path used against cs2.exe.

namespace
{
std::uintptr_t address_of(const void* p)
{
    return reinterpret_cast<std::uintptr_t>(p);
}

// Two pages: the first committed (read/write), the second only reserved, so any access to it fails.
class TwoPageRegion
{
public:
    TwoPageRegion()
    {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        page_size_ = info.dwPageSize;
        base_ = VirtualAlloc(nullptr, page_size_ * 2, MEM_RESERVE, PAGE_NOACCESS);
        if (base_ != nullptr && VirtualAlloc(base_, page_size_, MEM_COMMIT, PAGE_READWRITE) == nullptr)
        {
            VirtualFree(base_, 0, MEM_RELEASE);
            base_ = nullptr;
        }
    }
    ~TwoPageRegion()
    {
        if (base_ != nullptr)
        {
            VirtualFree(base_, 0, MEM_RELEASE);
        }
    }
    TwoPageRegion(const TwoPageRegion&) = delete;
    TwoPageRegion& operator=(const TwoPageRegion&) = delete;

    [[nodiscard]] bool ok() const { return base_ != nullptr; }
    [[nodiscard]] std::uintptr_t reserved() const { return address_of(base_) + page_size_; }

private:
    void* base_ = nullptr;
    std::size_t page_size_ = 0;
};

struct Sample
{
    std::int32_t health;
    float position[3];
    std::uint8_t team;
};
} // namespace

TEST_CASE("ProcessMemory reads a value and a struct")
{
    const core::ProcessMemory memory(GetCurrentProcess());

    const std::int32_t value = 1234;
    CHECK(memory.read<std::int32_t>(address_of(&value)) == 1234);

    const Sample sample{100, {1.0f, 2.0f, 3.0f}, 2};
    Sample out{};
    REQUIRE(memory.safe_read(address_of(&sample), out));
    CHECK(out.health == 100);
    CHECK(out.position[2] == doctest::Approx(3.0f));
    CHECK(out.team == 2);
}

TEST_CASE("ProcessMemory fails cleanly on memory that isn't there")
{
    const TwoPageRegion region;
    REQUIRE(region.ok());
    const core::ProcessMemory memory(GetCurrentProcess());

    std::uint64_t out = 77;
    SUBCASE("reserved but uncommitted page")
    {
        CHECK_FALSE(memory.safe_read(region.reserved(), out));
    }
    SUBCASE("read that runs off the end of a committed page")
    {
        CHECK_FALSE(memory.safe_read(region.reserved() - 4, out));
    }
    CHECK(out == 77);
}

TEST_CASE("ProcessMemory with a null handle fails")
{
    const core::ProcessMemory memory(nullptr);
    const std::int32_t value = 5;
    CHECK_FALSE(memory.read<std::int32_t>(address_of(&value)).has_value());
}

TEST_CASE("ProcessMemory writes a value and fails on an uncommitted page")
{
    core::ProcessMemory memory(GetCurrentProcess());

    std::int32_t target = 1;
    CHECK(memory.safe_write(address_of(&target), std::int32_t{42}));
    CHECK(target == 42);

    const TwoPageRegion region;
    REQUIRE(region.ok());
    CHECK_FALSE(memory.safe_write(region.reserved(), std::int32_t{42}));
}
