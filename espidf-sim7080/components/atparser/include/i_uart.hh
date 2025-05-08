#pragma once
class iUartHal{
public:
    virtual void writeChars(const char* data, size_t data_len_bytes)=0;
};