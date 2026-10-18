idf_component_register(
    SRCS "zumi_main.c" "zumi/core.c" "zumi/view.c" "zumi/sprites.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer
)
