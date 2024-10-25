set(component_srcs "src/AndrewsMediaRemote.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"
                       REQUIRES "esp_hid esp_hid_gap nvs_flash")
