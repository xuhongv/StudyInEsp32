set(src "my_spi_flash.c")
set(inc "include")


idf_component_register(SRCS ${src}
                    INCLUDE_DIRS ${inc})
