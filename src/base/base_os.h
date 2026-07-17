// base_os.h - the platform layer: virtual memory, files, time, threads.
//
// Everything in this file is declared once and implemented three times, one
// #if branch per OS. No other file calls an OS API directly. A new platform
// primitive gets a declaration here and three implementations below.
//
// <windows.h> and <pthread.h> are pulled in only inside the implementation
// section. Including base.h never leaks them into project code, windows.h's
// min and max macros included.
//
// Include base.h, not this file directly. See STYLE.md for conventions.

#ifndef BASE_OS_H
#define BASE_OS_H

#include "base_types.h"
#include "base_string.h"

#ifndef BASE_ARENA_FWD
#define BASE_ARENA_FWD
typedef struct Arena Arena;
#endif

////////////////////////////////
// Platform detection
//
// Exactly one of these is 1. They are always defined, so test them with
// `#if OS_WINDOWS` and never with `#ifdef`.

#if defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_MAC 1
#elif defined(__linux__)
#define OS_LINUX 1
#else
#error "base_os.h: unsupported platform"
#endif

#if !defined(OS_WINDOWS)
#define OS_WINDOWS 0
#endif
#if !defined(OS_MAC)
#define OS_MAC 0
#endif
#if !defined(OS_LINUX)
#define OS_LINUX 0
#endif

#define OS_PATH_MAX 4096

// Opaque platform handle. Zero means invalid.
typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
    u64 v[1];
};

typedef void OS_ThreadFunc(void *ptr);

////////////////////////////////
// Virtual memory
//
// Reserve claims address space without backing it. Commit backs a sub-range of
// a prior reservation with pages. base_arena.h is built on these two.

u64 os_page_size(void);
void *os_reserve(u64 size);
b32 os_commit(void *ptr, u64 size);
void os_decommit(void *ptr, u64 size);
void os_release(void *ptr, u64 size);

////////////////////////////////
// Files
//
// os_file_read pushes contents onto `arena` and null-terminates one byte past
// `out->size`. A failed read pops the arena back and costs nothing.

b32 os_file_read(Arena *arena, Str8 path, Str8 *out);
b32 os_file_write(Str8 path, Str8 data);

////////////////////////////////
// Time

u64 os_time_microseconds(void); // monotonic; epoch is arbitrary
void os_sleep_milliseconds(u32 ms);

////////////////////////////////
// Threads
//
// The thread's bookkeeping is pushed onto `arena`, so the arena must outlive
// the thread. Join before releasing it.

OS_Handle os_thread_launch(Arena *arena, OS_ThreadFunc *func, void *ptr);
b32 os_thread_join(OS_Handle handle);

#ifdef BASE_IMPLEMENTATION

// Included here, not at the top. base_arena.h's declarations depend on this
// file, and the OS implementation below allocates from an arena.
#include "base_arena.h"

// Paths are bounded rather than arena-allocated so os_file_write needs no arena.
static b32 os_path_cstr_(Str8 path, char *buf, u64 buf_size)
{
    if (path.size + 1 > buf_size)
    {
        return 0;
    }
    if (path.size > 0)
    {
        MemoryCopy(buf, path.str, path.size);
    }
    buf[path.size] = 0;
    return 1;
}

////////////////////////////////
#if OS_WINDOWS
////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

u64 os_page_size(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (u64)info.dwPageSize;
}

void *os_reserve(u64 size)
{
    return VirtualAlloc(0, (SIZE_T)size, MEM_RESERVE, PAGE_NOACCESS);
}

b32 os_commit(void *ptr, u64 size)
{
    return VirtualAlloc(ptr, (SIZE_T)size, MEM_COMMIT, PAGE_READWRITE) != 0;
}

void os_decommit(void *ptr, u64 size)
{
    VirtualFree(ptr, (SIZE_T)size, MEM_DECOMMIT);
}

void os_release(void *ptr, u64 size)
{
    Unused(size); // MEM_RELEASE frees the whole original reservation
    VirtualFree(ptr, 0, MEM_RELEASE);
}

b32 os_file_read(Arena *arena, Str8 path, Str8 *out)
{
    out->str  = 0;
    out->size = 0;

    char path_c[OS_PATH_MAX];
    if (!os_path_cstr_(path, path_c, sizeof(path_c)))
    {
        return 0;
    }

    HANDLE file = CreateFileA(path_c, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    b32 result       = 0;
    u64 restore_pos  = arena_pos(arena);
    LARGE_INTEGER sz = {0};
    if (GetFileSizeEx(file, &sz))
    {
        u64 size = (u64)sz.QuadPart;
        u8 *data = (u8 *)arena_push_no_zero(arena, size + 1, 1);
        if (data)
        {
            u64 total = 0;
            b32 ok    = 1;
            while (total < size)
            {
                u64 remaining = size - total;
                DWORD chunk   = (DWORD)Min(remaining, (u64)0x7FFFFFFF);
                DWORD got     = 0;
                if (!ReadFile(file, data + total, chunk, &got, 0) || got == 0)
                {
                    ok = 0;
                    break;
                }
                total += (u64)got;
            }
            if (ok && total == size)
            {
                data[size] = 0;
                out->str   = data;
                out->size  = size;
                result     = 1;
            }
        }
    }
    if (!result)
    {
        arena_pop_to(arena, restore_pos);
    }
    CloseHandle(file);
    return result;
}

b32 os_file_write(Str8 path, Str8 data)
{
    char path_c[OS_PATH_MAX];
    if (!os_path_cstr_(path, path_c, sizeof(path_c)))
    {
        return 0;
    }

    HANDLE file = CreateFileA(path_c, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    b32 result = 1;
    u64 total  = 0;
    while (total < data.size)
    {
        u64 remaining = data.size - total;
        DWORD chunk   = (DWORD)Min(remaining, (u64)0x7FFFFFFF);
        DWORD written = 0;
        if (!WriteFile(file, data.str + total, chunk, &written, 0) || written == 0)
        {
            result = 0;
            break;
        }
        total += (u64)written;
    }
    CloseHandle(file);
    return result;
}

u64 os_time_microseconds(void)
{
    static LARGE_INTEGER freq;
    if (freq.QuadPart == 0)
    {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    // Split to keep the multiply from overflowing on long uptimes.
    u64 ticks     = (u64)now.QuadPart;
    u64 per_sec   = (u64)freq.QuadPart;
    u64 seconds   = ticks / per_sec;
    u64 remainder = ticks % per_sec;
    return seconds * 1000000 + (remainder * 1000000) / per_sec;
}

void os_sleep_milliseconds(u32 ms)
{
    Sleep((DWORD)ms);
}

typedef struct OS_ThreadEntry_ OS_ThreadEntry_;
struct OS_ThreadEntry_
{
    OS_ThreadFunc *func;
    void *ptr;
    HANDLE handle;
};

static DWORD WINAPI os_thread_entry_(LPVOID raw)
{
    OS_ThreadEntry_ *entry = (OS_ThreadEntry_ *)raw;
    entry->func(entry->ptr);
    return 0;
}

OS_Handle os_thread_launch(Arena *arena, OS_ThreadFunc *func, void *ptr)
{
    OS_Handle handle = {0};

    OS_ThreadEntry_ *entry = PushStruct(arena, OS_ThreadEntry_);
    if (entry == 0)
    {
        return handle;
    }
    entry->func = func;
    entry->ptr  = ptr;

    entry->handle = CreateThread(0, 0, os_thread_entry_, entry, 0, 0);
    if (entry->handle == 0)
    {
        return handle;
    }

    handle.v[0] = (u64)(uintptr_t)entry;
    return handle;
}

b32 os_thread_join(OS_Handle handle)
{
    OS_ThreadEntry_ *entry = (OS_ThreadEntry_ *)(uintptr_t)handle.v[0];
    if (entry == 0)
    {
        return 0;
    }
    b32 result = WaitForSingleObject(entry->handle, INFINITE) == WAIT_OBJECT_0;
    CloseHandle(entry->handle);
    entry->handle = 0;
    return result;
}

////////////////////////////////
#else // OS_LINUX || OS_MAC
////////////////////////////////

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

u64 os_page_size(void)
{
    static u64 page_size = 0;
    if (page_size == 0)
    {
        page_size = (u64)sysconf(_SC_PAGESIZE);
    }
    return page_size;
}

void *os_reserve(u64 size)
{
    // PROT_NONE keeps the kernel from backing any of this until os_commit.
    void *result = mmap(0, (size_t)size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    {
        return 0;
    }
    return result;
}

b32 os_commit(void *ptr, u64 size)
{
    return mprotect(ptr, (size_t)size, PROT_READ | PROT_WRITE) == 0;
}

void os_decommit(void *ptr, u64 size)
{
    madvise(ptr, (size_t)size, MADV_DONTNEED);
    mprotect(ptr, (size_t)size, PROT_NONE);
}

void os_release(void *ptr, u64 size)
{
    munmap(ptr, (size_t)size);
}

b32 os_file_read(Arena *arena, Str8 path, Str8 *out)
{
    out->str  = 0;
    out->size = 0;

    char path_c[OS_PATH_MAX];
    if (!os_path_cstr_(path, path_c, sizeof(path_c)))
    {
        return 0;
    }

    int fd = open(path_c, O_RDONLY);
    if (fd < 0)
    {
        return 0;
    }

    struct stat info;
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode))
    {
        close(fd);
        return 0;
    }

    b32 result      = 0;
    u64 restore_pos = arena_pos(arena);
    u64 size        = (u64)info.st_size;
    u8 *data        = (u8 *)arena_push_no_zero(arena, size + 1, 1);
    if (data)
    {
        u64 total = 0;
        while (total < size)
        {
            ssize_t got = read(fd, data + total, (size_t)(size - total));
            if (got < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                break;
            }
            if (got == 0)
            {
                break; // truncated under us
            }
            total += (u64)got;
        }
        if (total == size)
        {
            data[size] = 0;
            out->str   = data;
            out->size  = size;
            result     = 1;
        }
    }
    if (!result)
    {
        arena_pop_to(arena, restore_pos);
    }
    close(fd);
    return result;
}

b32 os_file_write(Str8 path, Str8 data)
{
    char path_c[OS_PATH_MAX];
    if (!os_path_cstr_(path, path_c, sizeof(path_c)))
    {
        return 0;
    }

    int fd = open(path_c, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        return 0;
    }

    b32 result = 1;
    u64 total  = 0;
    while (total < data.size)
    {
        ssize_t written = write(fd, data.str + total, (size_t)(data.size - total));
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            result = 0;
            break;
        }
        total += (u64)written;
    }
    close(fd);
    return result;
}

u64 os_time_microseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000 + (u64)ts.tv_nsec / 1000;
}

void os_sleep_milliseconds(u32 ms)
{
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
    {
        // resume the remaining interval
    }
}

typedef struct OS_ThreadEntry_ OS_ThreadEntry_;
struct OS_ThreadEntry_
{
    OS_ThreadFunc *func;
    void *ptr;
    pthread_t handle;
};

static void *os_thread_entry_(void *raw)
{
    OS_ThreadEntry_ *entry = (OS_ThreadEntry_ *)raw;
    entry->func(entry->ptr);
    return 0;
}

OS_Handle os_thread_launch(Arena *arena, OS_ThreadFunc *func, void *ptr)
{
    OS_Handle handle = {0};

    OS_ThreadEntry_ *entry = PushStruct(arena, OS_ThreadEntry_);
    if (entry == 0)
    {
        return handle;
    }
    entry->func = func;
    entry->ptr  = ptr;

    if (pthread_create(&entry->handle, 0, os_thread_entry_, entry) != 0)
    {
        return handle;
    }

    handle.v[0] = (u64)(uintptr_t)entry;
    return handle;
}

b32 os_thread_join(OS_Handle handle)
{
    OS_ThreadEntry_ *entry = (OS_ThreadEntry_ *)(uintptr_t)handle.v[0];
    if (entry == 0)
    {
        return 0;
    }
    return pthread_join(entry->handle, 0) == 0;
}

////////////////////////////////
#endif // platform branches
////////////////////////////////

#endif // BASE_IMPLEMENTATION
#endif // BASE_OS_H
