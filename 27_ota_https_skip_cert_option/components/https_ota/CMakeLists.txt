# set(require_components ${IDF_TARGET} mqtt mdns esp_http_client json freertos spiffs lwip
#     bootloader_support app_update openssl wpa_supplicant spi_flash esp_http_server bt esp32c3 esp32)

set(require_components ${IDF_TARGET} app_update freertos lwip mbedtls lwip nghttp)
    
file(GLOB_RECURSE src src/*.c)

idf_component_register (
    SRCS ${src}
    INCLUDE_DIRS "include"
    REQUIRES ${require_components}
    EMBED_TXTFILES  cert/server_root_cert.pem
)