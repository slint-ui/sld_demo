
#if defined(KERNEL_USES_SLINT)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <esp_err.h>
#include <slint-esp.h>

#include "slint_string.h"
#include <slint_timer.h>

#include "appwindow.h"

using namespace std::chrono_literals;

extern "C"
{
    #include "board/board.h"
    #include "esp_lcd_panel_ops.h"
    #include "module/version/version.h"
    #include "mcu/sys.h"
    #include "module/comm/dbg.h"
    #include "module/lcd_touch/lcd_touch_esp32.h"

    static void _task_window(void* param);
}

static std::vector<slint::platform::Rgb565Pixel>* buffer;

extern "C" bool app_ui_init(void)
{
    uint32_t width = display_device_get_width(board_lcd->display);
    uint32_t height = display_device_get_height(board_lcd->display);

    DBG_INFO("Initialize %d x %d\n", width, height);

    bool swap_xy, mirror_x, mirror_y;
    struct lcd_touch_flags_s flags;
    display_device_get_swap_xy(board_lcd->display, &swap_xy);
    display_device_get_mirror(board_lcd->display, &mirror_x, &mirror_y);
    lcd_touch_get_flags(board_lcd->touch, &flags);

    DBG_INFO("Display: Swap=%d MirrorX=%d MirrorY=%d\n", swap_xy, mirror_x, mirror_y);
    DBG_INFO("Touch: Swap=%d MirrorX=%d MirrorY=%d\n", flags.swap_xy, flags.mirror_x, flags.mirror_y);
    
    /* Allocate a drawing buffer */
    buffer = new std::vector<slint::platform::Rgb565Pixel>(width * height);

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_touch_handle_t touch_handle = NULL;
    display_get_esp_panel_handle(board_lcd->display, &panel_handle);
    lcd_touch_esp32_create(board_lcd->touch, &touch_handle);

    /* Initialize Slint's ESP platform support*/
    slint_esp_init(SlintPlatformConfiguration{
        .size = slint::PhysicalSize({ width, height }), 
        .panel_handle = panel_handle,
        .touch_handle = touch_handle, 
        .buffer1 = *buffer,
        .color_swap_16 = false
        });

    board_set_backlight(100.0);

    xTaskCreate(_task_window, "DISP", 8192 * 2, NULL, 15, NULL);

    // return eve_found;
    return true;
}

extern "C" 
{
    static void _task_window(void* param)
    {
        auto ui = AppWindow::create();
        /* Show it on the screen and run the event loop */
        ui->set_version(slint::SharedString(version_get_string()));

        slint::Timer timer_update_runtime;
        timer_update_runtime.start(slint::TimerMode::Repeated, 1s, [&ui]() {
            uint32_t seconds = system_get_tick_count() / 1000;
            DBG_VERBOSE((char*)"seconds = %d\n", seconds);
            ui->set_runtime_minutes(slint::SharedString(std::format("{:02d}:{:02d}", seconds / 60, seconds % 60)));
        });
        ui->run();
    }
}

#endif // #if defined(KERNEL_USES_SLINT)