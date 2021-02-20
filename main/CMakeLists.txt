set(srcs "spi_master_example_main.c"
    "axp202.c" "ft6336.c" "uearth.c" "state.c" "sun.c"
    )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES WORLD.DAT)

