idf_component_register(
    SRCS "zumi_main.c" "zumi/core.c" "zumi/view.c" "zumi/sprites.c" "zumi/wake_cards.c" "zumi/sound.c" "zumi/sounds.c" "zumi/audio.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer
    EMBED_FILES "../assets/images/zumi-pocket/wake-cards/wake_cards.rgb565"
)
