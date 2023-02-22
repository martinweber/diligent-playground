#pragma once
#include <string>
#include <exception>

namespace cgrebel {

struct error_message
{
    std::string message = "";

    error_message() noexcept = default;
    explicit error_message(std::string_view message_) noexcept
        : message(message_)
    {}

    // empty message return true as success
    operator bool() const noexcept { return message.empty(); }
    operator const char*() const noexcept { return message.c_str(); }
};

template<typename T>
struct log_error final : public std::exception
{
    T value;
    using type = T;
    int line = 0;
    std::string file = "";
    std::string function = "";

    log_error() noexcept = delete;
    log_error(const log_error& other) noexcept = default;
    log_error(log_error&& other) noexcept = default;
    log_error& operator=(const log_error& other) noexcept = default;
    log_error& operator=(log_error&& other) noexcept = default;

    explicit log_error(const std::string_view file_, int line_, std::string_view function_) noexcept
        : file(file_)
        , line(line_)
        , function(function_)
    {}
    explicit log_error(const T payload_, const std::string_view file_, int line_, std::string_view function_) noexcept
        : value(payload_)
        , file(file_)
        , line(line_)
        , function(function_)
    {}

    constexpr operator bool() const noexcept { return static_cast<bool>(value); }
    const char* what() const noexcept override { return static_cast<const char*>(value); }
};

using Error = log_error<error_message>;

#define CGR_SUCCESS cgrebel::Error(__FILE__, __LINE__, __FUNCTION__)
#define CGR_FAIL(payload) cgrebel::Error(cgrebel::error_message(payload), __FILE__, __LINE__, __FUNCTION__)
} // namespace cgrebel
