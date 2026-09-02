#pragma once
#include <stddef.h>
#include <stdint.h>

// A base que a biblioteca PMS REAL espera encontrar. Só os quatro métodos
// que ela usa — não é o Stream inteiro do Arduino, é o contrato que este
// projeto exerce.
class Stream {
public:
    virtual ~Stream() {}
    virtual int    available() = 0;
    virtual int    read()      = 0;
    virtual int    peek()      = 0;
    virtual size_t write(uint8_t b) = 0;
    virtual size_t write(const uint8_t* dados, size_t n) = 0;
};
