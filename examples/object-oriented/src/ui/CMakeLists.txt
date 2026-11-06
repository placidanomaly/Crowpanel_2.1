SET(SOURCES screens/ui_timerScreen.c
    screens/ui_stopwatchScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_sevenSegments48.c
    fonts/ui_font_sevenSegments96.c)

add_library(ui ${SOURCES})
