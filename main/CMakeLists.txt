idf_component_register(
    SRCS "zumi_main.c" "zumi/core.c" "zumi/talk_model.c" "zumi/talk_wire.c" "zumi/talk_device.c" "demo_radio.c" "zumi/view.c" "zumi/sprites.c" "zumi/energy_icons.c" "zumi/wake_cards.c" "zumi/sound.c" "zumi/sounds.c" "zumi/audio.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer esp_wifi esp_netif nvs_flash tcp_transport json
    EMBED_FILES "../assets/images/zumi-pocket/wake-cards/wake_cards.rgb565"
)
