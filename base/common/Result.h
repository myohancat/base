/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RESULT_H_
#define __RESULT_H_

#include <string>
#include <variant>

struct Error
{
    int mRCode;
    std::string mMessage;

    Error(int rcode) : mRCode(rcode), mMessage("") {}
    Error(int rcode, const std::string& msg) : mRCode(rcode), mMessage(msg) {}
    Error(int rcode, std::string&& msg) : mRCode(rcode), mMessage(std::move(msg)) {}
};

template <typename T>
class Result
{
public:
    Result(const T& value) : mData(value) {}
    Result(T&& value) : mData(std::move(value)) {}

    Result(const Error& error) : mData(error) {}
    Result(Error&& error) : mData(std::move(error)) {}

    bool is_ok() const
    {
        return std::holds_alternative<T>(mData);
    }

    explicit operator bool() const
    {
        return is_ok();
    }

    const T& value() const
    {
        if (!is_ok())
            throw std::bad_variant_access();

        return std::get<T>(mData);
    }

    const Error& error() const
    {
        if (is_ok())
            throw std::bad_variant_access();

        return std::get<Error>(mData);
    }

private:
    std::variant<T, Error> mData;
};

template <typename T>
inline Result<T> Ok(const T& v)
{
    return Result<T>(v);
}

template <typename T>
inline Result<T> Ok(T&& v)
{
    return Result<T>(std::move(v));
}

template <typename T>
inline Result<T> Err(const Error& e)
{
    return Result<T>(e);
}

template <typename T>
inline Result<T> Err(Error&& e)
{
    return Result<T>(std::move(e));
}

template <typename T>
inline Result<T> Err(int code, std::string msg = "")
{
    return Result<T>(Error(code, std::move(msg)));
}

#endif // __RESULT_H_
