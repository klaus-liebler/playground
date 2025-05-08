
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "mbedtls/platform.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include <esp_log.h>
#define TAG "RSA"
 #define MBEDTLS_ERROR_CHECK(x) do { int __err_rc = (x); if (__err_rc != 0) { ESP_LOGE(TAG, "Error: %s %d %s", __FILE__, __LINE__, #x); } } while(0)
   
    

const char* privateKeySmartBoard=R"(-----BEGIN RSA PRIVATE KEY-----
MIIEogIBAAKCAQBeE14Ylm0NSwgeKoibwGw73YwsAj6qtYvBbfKCB7HX2BZXY86b
sbn0C8KJlU31HHC67UWG82/Y/GmbqPI5j5Lw2s3fGCYgXS42SyRx8hXyIYqv+Lg3
RuTd8UPg55pWyakOmEg5+8I82ZaFCXfAENujax0nRdidFxj/hwa0seAFQxZ/W3gW
A3ek5op+nidFWpCfCEwzJczCVgd/WF17n70pDlewZF70Euqw/Uo/4Rzyk/ZfbVMi
8hJAYjOXrV7ZcdMwgT4UvFZXNRvPC7YrvkjOJ20nM1lbgpzYxOrKHXNtI0XON/+a
IUd+kQm32kdNQxw1d9FnpUWY8ZDv98/05TfxAgMBAAECggEAV2gRRjEFc5NiIH0O
XgADz/+BTCZtyThSeE7KQgwaLdyErX97KGeRyXDft+sz+mgJSXRI5CDsq6OiD9GB
9P8z3Hq/SufqDKnm1OUuIHx37XdCn0stBZbl8FxI3/7wBHDrlTbh7JXAm77ivgrU
62v+VjCgbqvTnekm/cub7GlUJi5SSGHGfGVD0aOpPcz0z0mNKYArVd0nNQM2fFH6
3RmMQ/gtIrXWYJzTakgYU1Mi0tLUPME2nQf6oqma4Q4xejU925UEA0g8yLd7em+U
RCYWs8hW/66No0DwuiXGG/ewC3Df/mkGX5wnY5nL5VSTE5jg7I0lOdkC/YzvVo11
0ZbOPQKBgQC01AZpLCcAqkrVrLyvqEM5TG4QKapq5PX8yIBel7KWznQaoj6RAdVa
JQNu9shtYbyXlyTc2YrGIRhk4PWwyQlhNxkUoy/CgBnlxH7TfvdYDwF7gPLdy/mb
0RXKGKISviXKR9UtlgJ9C4aAnIaNWjNYTfmv+xZHxROe065pqFQ+qwKBgQCFLwVv
UN2v8luFUprp1mKMexRWubI3wjbL1V5kQh7Ytk/RpEsookZtUsEVkj5zqf4S/GjS
DByX/krfgcK7Cnbh3L0eWOpcbs+NBvOGUDCirtklaFvp+Ah+SDztUZJVLiOfGqtU
6pxovFDjGzLuf1lAyPcopZ8i7osMEL0Qnuyz0wKBgFpzjFljeob8p75GT4PWOPoa
Xxu6m8IokVtGt9+ZJf4Fe1IvQfiq4BvdU+BXlV4Re5KgShImxyHgQCGF3Z/2j7iP
JP6LukIibdHe/kmwwuzLZMRwdgu4Ej180tdqhve1IZYTZHEPDmOaNkdzBGMEcZaE
Opdqk3SHAg96d03mo21NAoGAQNMpRwxKSyig6VzSsDy8TPpfZPMPztoGR+J8Bjq0
vvDGGxlnx8hslSuxdalwImY8l8spzgs9I7IPiYikWtv2D2uc1Zebww/gkBkxEVAY
zCZk5TNNga1LnQPWPAD3Q+SoS/n0KOnk5Ym7qT8E5Fcx/zmSK8OpmJwZK+89x0hD
mCcCgYEAnic4dRrQL9zeQzUyIMh4+KTkMmklo2zMiGKA7m8y/bs3/DNUJ+K8x0cY
tbhf1ePeYBvPWsVjCB7EYibA6eT/aeDUAX8UKKw1WkzFUML+8A+Bhu94/lJ+2ZMu
3UUTIRcl74rYyGFfLLqS9aVqXhjOcm3zZhxlSvZNFq8b0Lfqd58=
-----END RSA PRIVATE KEY-----)";

const char* publicKeySmartboard=R"(-----BEGIN PUBLIC KEY-----
MIIBITANBgkqhkiG9w0BAQEFAAOCAQ4AMIIBCQKCAQBeE14Ylm0NSwgeKoibwGw7
3YwsAj6qtYvBbfKCB7HX2BZXY86bsbn0C8KJlU31HHC67UWG82/Y/GmbqPI5j5Lw
2s3fGCYgXS42SyRx8hXyIYqv+Lg3RuTd8UPg55pWyakOmEg5+8I82ZaFCXfAENuj
ax0nRdidFxj/hwa0seAFQxZ/W3gWA3ek5op+nidFWpCfCEwzJczCVgd/WF17n70p
DlewZF70Euqw/Uo/4Rzyk/ZfbVMi8hJAYjOXrV7ZcdMwgT4UvFZXNRvPC7YrvkjO
J20nM1lbgpzYxOrKHXNtI0XON/+aIUd+kQm32kdNQxw1d9FnpUWY8ZDv98/05Tfx
AgMBAAE=
-----END PUBLIC KEY-----)";

const char* privateKeyCompany=R"(-----BEGIN RSA PRIVATE KEY-----
MIIEogIBAAKCAQB2PLtIBSd79FYhFmhH7QWT4aakxPSRd7H4CaGmgqic694dV/7l
9+dwAZSn1zPdEVJZnCcQ7hnjmnBYRCS8oLZ984a+TtrDmOts7JgXJVfXsxA3DLOu
Mz3wBHxNqP8Tdt8724fz6Aa6u8m20x22UBOpYdoU4yav8zHuKpao4sRiDwczK9fr
kgkKtxXpiDUVyY7A81rkxntWhOGQMY7q0uXGjN4LyhywVs7VNl4M0E7TTgG4UaLG
qAqWukKksYIV1GmQ/1r26k2VdQy80bItp/UB9dtXZ7Y9c5UGW+5P7Rlj/KsLQLwO
lLLsJiOGrCHnOtJTqno5v9EntaUNlDULk/3xAgMBAAECggEAY+ZqYq6gLtZCU5CI
KYV1ef5le8BB2ArnHuicYYDyECwmLmrkHtA4BuX/AVUOKBHzn20VgPN8gldB5wIk
fA/aQU8aoP8lTbCN8ilxU9UkhEZyQ+ECjAQVqA9SK5fHdV2xvfR9yTJZRTtXcpar
+xdrlWjtB0rprC5yGIZ5Z1a6kRJ0rhF9SwzCfXabo6UyjLuMxaXKnx/4vZqKSEGc
QDJOq5UMMbM6YeyjgkZK+NaOVPUbQO+1sll9hFtktlPZBzDNgcWAd5XlcvI5W5nY
BTyXIHtHWbkfBdLAlDaMqFd0S2bNkevR/TvavH7Lo2/6rsNtQ1MJVUbH2Pi7mMid
1Zp6sQKBgQC7rL4EJl8o8P2GsF0RF6AOQd3NX2KClZBpAs7dZV4SiyO6yyhnBxNQ
oKQGlThEiRF0mx4yakYHTomx0NZIeNHkFEQ5SYUBNRhJi0ODhHjLePsFCe59F1WH
zynQkkzoZq28NDU2y/+Kt8seBiOOj8JS5RqH73eR6Yey26VoadzNdQKBgQChSGwG
8lDzjcj1dsqzLZKOmKN6m6OiOAKWBqzqnABGa15NTmI7mVYuu6vlxLX/aMQcigWU
G2vpZJAJ+GjKSDUZvbgaLZhaIsFVf/pIktB8ocpmlUwY8ann66SZ0rT8+FHH7rki
vCjROz3cxnrCymqmtQElmf15wCJ8t6TOkopzDQKBgQClztdOOUyHkmqbDcQbrsDE
9+JraONStasRa96x8bXsMivOA2FdrP2elV51Rm6g9myKleVW61eVawjyJay7+KqP
lGw2JMMyruMXTsfGMUq7kdZMvFqNAAmb5CZwVGFITjceeQ2urQrHVtxIWWoesNoh
fLI0+RetvSIbnj0EA8uFZQKBgEaga0YDePWewkDTm2Ipc8NkDjrTyy1vb5p5I+G9
HNW5kNszlwrYVAg1AMqmgLhvDWod+uFnvLLA6d6j3GewhShXjzZPfycBjz48w8Ji
IFBjnTmc6W0G7stzwmnl/oHE0DNIU+RlF07ArY9vjzSb5WZJX04zB+/39mI9HTuk
qmJFAoGATkDxBvhh4dUehhUuSvyCsSH16SpCaR4jaozITXy+iGuzPiuyZz3VMtTk
XpxSmU4bxRyiKQ7MzBtg/V+72Tt/1JhSKrDqfHKhZExy0PHZrJHvUnPVgW+Tis1v
roCKJhZ2pk4/MPWfm5GqzO9E4pPLHh4h8vasYc/7rhKlyxHSIhY=
-----END RSA PRIVATE KEY-----)";

const char* publicKeyCompany=R"(-----BEGIN PUBLIC KEY-----
MIIBITANBgkqhkiG9w0BAQEFAAOCAQ4AMIIBCQKCAQB2PLtIBSd79FYhFmhH7QWT
4aakxPSRd7H4CaGmgqic694dV/7l9+dwAZSn1zPdEVJZnCcQ7hnjmnBYRCS8oLZ9
84a+TtrDmOts7JgXJVfXsxA3DLOuMz3wBHxNqP8Tdt8724fz6Aa6u8m20x22UBOp
YdoU4yav8zHuKpao4sRiDwczK9frkgkKtxXpiDUVyY7A81rkxntWhOGQMY7q0uXG
jN4LyhywVs7VNl4M0E7TTgG4UaLGqAqWukKksYIV1GmQ/1r26k2VdQy80bItp/UB
9dtXZ7Y9c5UGW+5P7Rlj/KsLQLwOlLLsJiOGrCHnOtJTqno5v9EntaUNlDULk/3x
AgMBAAE=
-----END PUBLIC KEY-----)";


extern "C" void app_main(void)
{
    unsigned char result[16];
    size_t resultLen{0};
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *personalization = "personalization";
    size_t encryptLen{0};
    uint8_t guid[]={0x5F, 0xF2, 0x9D, 0xBD, 0x13, 0xB4, 0x44, 0xE8, 0x8B, 0x31, 0xD6, 0xBD, 0xB2, 0x11, 0x97, 0xF0};
    uint8_t encrypt[4096];
    uint8_t hashSign[32];
    uint8_t hashVerify[32];
    
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy, (const unsigned char *) personalization, strlen(personalization));
    
    ESP_LOGI(TAG,  "Encrypt GUID");
    mbedtls_pk_init( &pk );
    MBEDTLS_ERROR_CHECK(mbedtls_pk_parse_public_key(&pk, (const unsigned char *)publicKeySmartboard, strlen(publicKeySmartboard)+1));
    MBEDTLS_ERROR_CHECK(mbedtls_pk_encrypt( &pk, guid, sizeof(guid), encrypt, &encryptLen, sizeof(encrypt), mbedtls_ctr_drbg_random, &ctr_drbg ));
    ESP_LOGI(TAG, "Encryption successful, encrypted output len = %d.", encryptLen);

    ESP_LOGI(TAG,  "Sign encrypted GUID");
    mbedtls_pk_init(&pk);
    MBEDTLS_ERROR_CHECK(mbedtls_pk_parse_key(&pk, (const unsigned char *)privateKeyCompany, strlen(privateKeyCompany)+1, (const unsigned char *)"", 0));
    MBEDTLS_ERROR_CHECK(mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), encrypt, encryptLen, hashSign));
    size_t sigLen{0};
    mbedtls_pk_sign( &pk, MBEDTLS_MD_SHA256, hashSign, 0, encrypt+encryptLen, &sigLen, mbedtls_ctr_drbg_random, &ctr_drbg );
    ESP_LOGI(TAG,  "Signing successful; signature len = %d", sigLen);

    //encrypt[42]=0x42; // Hahahah, this is the marvellous and very smart attacker ;-)

    ESP_LOGI(TAG,  "Verify signature");
    mbedtls_pk_init(&pk);
    MBEDTLS_ERROR_CHECK(mbedtls_pk_parse_public_key(&pk, (const unsigned char *)publicKeyCompany, strlen(publicKeyCompany)+1));
    MBEDTLS_ERROR_CHECK(mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), encrypt, encryptLen, hashVerify));
    MBEDTLS_ERROR_CHECK(mbedtls_pk_verify( &pk, MBEDTLS_MD_SHA256, hashVerify, 0, encrypt+encryptLen, sigLen));
    ESP_LOGI(TAG, "Signature succcessfully verified");
    
    ESP_LOGI(TAG, "Decrypt" );
    mbedtls_pk_init( &pk );
    MBEDTLS_ERROR_CHECK(mbedtls_pk_parse_key(&pk, (const unsigned char *)privateKeySmartBoard, strlen(privateKeySmartBoard)+1, (const unsigned char *)"test", 4));
    MBEDTLS_ERROR_CHECK(mbedtls_pk_decrypt( &pk, encrypt, encryptLen, result, &resultLen, sizeof(result), mbedtls_ctr_drbg_random, &ctr_drbg ));
    ESP_LOGI(TAG, "Decrypted value is 0x %02x %02x %02x %02x %02x %02x %02x %02x", result[0], result[1],result[2], result[3],result[4], result[5],result[6], result[7]);
    
    mbedtls_pk_free( &pk );
}
