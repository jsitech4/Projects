#pragma once
#include <stddef.h>
// Certificate validation is outside the native persistence adapter's test scope.
struct mbedtls_x509_crt {
    bool ca_istrue = false;
};
inline void mbedtls_x509_crt_init(mbedtls_x509_crt *) {}
inline int mbedtls_x509_crt_parse(mbedtls_x509_crt *, const unsigned char *, size_t) {
    return -1;
}
inline void mbedtls_x509_crt_free(mbedtls_x509_crt *) {}
