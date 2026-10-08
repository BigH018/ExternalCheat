#pragma once

// Tiny logging helpers that write to stdout (the tool's console).
//
// Every function is noexcept and swallows its own errors: a log line must never take the tool down.
// Everything goes to stdout so info/warn/error lines keep their order. Hot-path callers must rate-limit themselves.
// The namespace is `logger`, not `log`, because `log` clashes with ::log from <cmath>.

#include <cstdio>
#include <format>
#include <string>
#include <utility>

namespace logger
{
namespace detail
{
inline void write(const char* prefix, const std::string& message) noexcept
{
    std::fputs(prefix, stdout);
    std::fputs(message.c_str(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

template <class... Args>
void format_and_write(const char* prefix, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    try
    {
        write(prefix, std::format(fmt, std::forward<Args>(args)...));
    }
    catch (...)
    {
        // Out of memory while formatting a log line: drop the line.
    }
}
} // namespace detail

template <class... Args>
void info(std::format_string<Args...> fmt, Args&&... args) noexcept
{
    detail::format_and_write("[+] ", fmt, std::forward<Args>(args)...);
}

template <class... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) noexcept
{
    detail::format_and_write("[!] ", fmt, std::forward<Args>(args)...);
}

template <class... Args>
void error(std::format_string<Args...> fmt, Args&&... args) noexcept
{
    detail::format_and_write("[x] ", fmt, std::forward<Args>(args)...);
}
} // namespace logger
