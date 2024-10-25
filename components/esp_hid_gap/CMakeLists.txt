set(component_srcs "src/esp_hid_gap.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       REQUIRES "esp_hid"
                       PRIV_REQUIRES "nvs_flash")
