#pragma once
#include <array>

class Error
{
public:
    enum Code
    {
        kSuccess,
        kFull,
        kEmpty,
        kIndexOutOfRange,
        kLastOfCode,
    };

    Error(Code code) : code_{code} {}

    operator bool() const
    {
        return this->code_ != kSuccess;
    }

    const char *Name() const
    {
        return code_names_[static_cast<int>(this->code_)];
    }

private:
    static constexpr std::array<const char *, 4> code_names_ = {
        "kSuccess",
        "kFull",
        "kEmpty",
        "kIndexOutOfRange",
    };

    Code code_;
};

#define MAKE_ERROR(code) Error((code))

template <class T>
struct WithError
{
    T value;
    Error error;
};