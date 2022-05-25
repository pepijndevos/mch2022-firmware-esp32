idf_component_register(
    SRCS "main.c"
         "appfs_wrapper.c"
         "fpga_test.c"
         "graphics_wrapper.c"
         "menu.c"
         "pax_keyboard.c"
         "rp2040_updater.c"
         "settings.c"
         "system_wrapper.c"
         "wifi_connection.c"
         "wifi_ota.c"
         "fpga_download.c"
         "audio.c"
    INCLUDE_DIRS "."
                 "include"
    EMBED_TXTFILES ${project_dir}/resources/isrgrootx1.pem
    EMBED_FILES ${project_dir}/resources/wallpaper.png
                ${project_dir}/resources/fpga_selftest.bin
                ${project_dir}/resources/rp2040_firmware.bin
                ${project_dir}/resources/boot.snd
                ${project_dir}/resources/mch2022_logo.png
                #${project_dir}/resources/icons/help32.png
                #${project_dir}/resources/icons/executablefile32.png
                #${project_dir}/resources/icons/erasefile32.png
                #${project_dir}/resources/icons/copy32.png
                #${project_dir}/resources/icons/infrared32.png
                #${project_dir}/resources/icons/fpgafile32.png
                #${project_dir}/resources/icons/audio32.png
                #${project_dir}/resources/icons/directory32.png
)
