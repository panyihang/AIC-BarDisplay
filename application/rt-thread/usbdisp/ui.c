/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * Idle/settings widgets from main/app_lcd_ui.c, upstream
 * 17b774453ef106ab58d5ed9b885e759e0d67879f. OS, persistence and display
 * allocation are adapted to Luban-Lite's 8 MiB PSRAM.
 */
#include <string.h>
#include <stdio.h>
#include "usbdisp.h"
LV_FONT_DECLARE(simhei_50);
LV_FONT_DECLARE(simhei_30);
#define LCD_UI_PANEL_OPA 150
#define LCD_UI_MAIN_PANEL_W 320
#define LCD_UI_MAIN_PANEL_H 320
#define LCD_UI_USB_PANEL_W 320
#define LCD_UI_USB_PANEL_H 80
#define LCD_UI_PANEL_GAP 10
#define LCD_UI_SETTINGS_BTN_SIZE 44
#define LCD_UI_FLOATING_SETTINGS_BTN_SIZE 56
#define LCD_UI_COUNTDOWN_MS UDISP_COUNTDOWN_MS
#define LCD_UI_NVS_KEY_PAGE_HIDE "page_hide"
#define LCD_UI_NVS_KEY_HID_TOUCH "hid_touch"
static struct {
    lv_obj_t *usb_panel, *main_panel, *usb_label, *status_label;
    lv_obj_t *countdown_label, *countdown_arc, *spinner, *settings_icon;
    lv_obj_t *floating_settings_icon, *settings_page, *back_icon;
    lv_obj_t *hide_switch, *hid_switch;
    bool settings_open, page_hidden, hid_touch_enabled, stream_detected;
    char last_status[64];
} s_ui_ctx;
static bool s_countdown_anim_started;
static struct aicfb_layer_data ui_layer;
static uint8_t *ui_buffer;
static uint8_t *ui_buffers[2];
static lv_display_t *ui_display;
static bool lv_started;
#define UI_BUFFER_SIZE (UDISP_UI_WIDTH * UDISP_UI_HEIGHT * 4U)
static volatile bool frame_ready;
static uint32_t countdown_start;
static bool counting, was_stream;
static bool ui_active;
static void app_lcd_ui_update_waiting_status_locked(void);
static void app_lcd_ui_start_intro_anim(lv_obj_t *obj, uint32_t delay);
static void app_lcd_ui_anim_set_opa(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void app_lcd_ui_anim_set_translate_y(void *obj, int32_t value)
{
    lv_obj_set_style_translate_y((lv_obj_t *)obj, value, 0);
}

static void app_lcd_ui_anim_set_arc_value(void *obj, int32_t value)
{
    lv_arc_set_value((lv_obj_t *)obj, value);
}

static void app_lcd_ui_style_panel(lv_obj_t *panel, int32_t width, int32_t height)
{
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, width, height);
    lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(panel, LCD_UI_PANEL_OPA, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_white(), 0);
    lv_obj_set_style_border_opa(panel, 70, 0);
    lv_obj_set_style_radius(panel, 18, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_shadow_color(panel, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(panel, 70, 0);
    lv_obj_set_style_shadow_width(panel, 24, 0);
    lv_obj_set_style_shadow_offset_y(panel, 8, 0);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
}

static void app_lcd_ui_set_hidden(lv_obj_t *obj, bool hidden)
{
    if (obj == NULL) {
        return;
    }

    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void app_lcd_ui_style_icon_button(lv_obj_t *btn, int32_t size, lv_opa_t bg_opa)
{
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, size / 2, 0);
    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(btn, bg_opa, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(btn, 70, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(btn, 55, 0);
    lv_obj_set_style_shadow_width(btn, 14, 0);
    lv_obj_set_style_shadow_offset_y(btn, 4, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
}

static lv_obj_t *app_lcd_ui_create_icon_button(lv_obj_t *parent, const char *symbol, int32_t size, lv_opa_t bg_opa)
{
    lv_obj_t *btn = lv_button_create(parent);
    app_lcd_ui_style_icon_button(btn, size, bg_opa);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, lv_color_hex(0x203040), 0);
    lv_obj_center(label);
    return btn;
}

static void app_lcd_ui_settings_save_bool(const char *key, bool value)
{
    (void)key; (void)value;
    udisp_page_hidden = s_ui_ctx.page_hidden;
    udisp_hid_enabled = s_ui_ctx.hid_touch_enabled;
    udisp_settings_save();
}

static void app_lcd_ui_apply_visibility_locked(void)
{
    if (s_ui_ctx.settings_open) {
        app_lcd_ui_set_hidden(s_ui_ctx.main_panel, false);
        app_lcd_ui_set_hidden(s_ui_ctx.usb_panel, true);
        app_lcd_ui_set_hidden(s_ui_ctx.floating_settings_icon, true);
        app_lcd_ui_set_hidden(s_ui_ctx.spinner, true);
        app_lcd_ui_set_hidden(s_ui_ctx.countdown_arc, true);
        app_lcd_ui_set_hidden(s_ui_ctx.countdown_label, true);
        app_lcd_ui_set_hidden(s_ui_ctx.settings_icon, true);
        app_lcd_ui_set_hidden(s_ui_ctx.settings_page, false);
        return;
    }

    app_lcd_ui_set_hidden(s_ui_ctx.settings_page, true);
    app_lcd_ui_set_hidden(s_ui_ctx.settings_icon, false);

    if (s_ui_ctx.page_hidden) {
        app_lcd_ui_set_hidden(s_ui_ctx.main_panel, true);
        app_lcd_ui_set_hidden(s_ui_ctx.usb_panel, true);
        app_lcd_ui_set_hidden(s_ui_ctx.floating_settings_icon, false);
    } else {
        app_lcd_ui_set_hidden(s_ui_ctx.main_panel, false);
        app_lcd_ui_set_hidden(s_ui_ctx.usb_panel, false);
        app_lcd_ui_set_hidden(s_ui_ctx.floating_settings_icon, true);
    }
}

static void app_lcd_ui_open_settings_locked(void)
{
    s_ui_ctx.settings_open = true;
    app_lcd_ui_apply_visibility_locked();
    app_lcd_ui_start_intro_anim(s_ui_ctx.settings_page, 0);
}

static void app_lcd_ui_close_settings_locked(void)
{
    s_ui_ctx.settings_open = false;
    app_lcd_ui_apply_visibility_locked();
    if (!s_ui_ctx.page_hidden && !s_ui_ctx.stream_detected) {
        app_lcd_ui_update_waiting_status_locked();
    }
}

static void app_lcd_ui_settings_icon_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        app_lcd_ui_open_settings_locked();
    }
}

static void app_lcd_ui_back_icon_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        app_lcd_ui_close_settings_locked();
    }
}

static void app_lcd_ui_switch_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    lv_obj_t *target = lv_event_get_target_obj(event);
    const bool enabled = lv_obj_has_state(target, LV_STATE_CHECKED);
    if (target == s_ui_ctx.hide_switch) {
        s_ui_ctx.page_hidden = enabled;
        app_lcd_ui_settings_save_bool(LCD_UI_NVS_KEY_PAGE_HIDE, enabled);
    } else if (target == s_ui_ctx.hid_switch) {
        s_ui_ctx.hid_touch_enabled = enabled;
        app_lcd_ui_settings_save_bool(LCD_UI_NVS_KEY_HID_TOUCH, enabled);

    }
}

static lv_obj_t *app_lcd_ui_create_setting_label(lv_obj_t *parent, const char *text, int32_t y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_width(label, 170);
    lv_obj_set_style_text_font(label, &simhei_30, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x16202C), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(label, 34, y);
    return label;
}

static lv_obj_t *app_lcd_ui_create_setting_switch(lv_obj_t *parent, bool checked, int32_t y)
{
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 64, 34);
    lv_obj_set_pos(sw, 220, y - 2);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xD7DFE8), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw, 170, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xF28C28), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_white(), LV_PART_KNOB);
    if (checked) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw, app_lcd_ui_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return sw;
}

static void app_lcd_ui_create_settings_page_locked(void)
{
    s_ui_ctx.settings_page = lv_obj_create(s_ui_ctx.main_panel);
    lv_obj_remove_style_all(s_ui_ctx.settings_page);
    lv_obj_set_size(s_ui_ctx.settings_page, LCD_UI_MAIN_PANEL_W, LCD_UI_MAIN_PANEL_H);
    lv_obj_set_pos(s_ui_ctx.settings_page, 0, 0);
    lv_obj_set_style_bg_opa(s_ui_ctx.settings_page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(s_ui_ctx.settings_page, 0, 0);
    lv_obj_set_scrollbar_mode(s_ui_ctx.settings_page, LV_SCROLLBAR_MODE_OFF);

    s_ui_ctx.back_icon = app_lcd_ui_create_icon_button(s_ui_ctx.settings_page, LV_SYMBOL_LEFT, LCD_UI_SETTINGS_BTN_SIZE, 110);
    lv_obj_set_pos(s_ui_ctx.back_icon, 14, 14);
    lv_obj_add_event_cb(s_ui_ctx.back_icon, app_lcd_ui_back_icon_event_cb, LV_EVENT_CLICKED, NULL);

    app_lcd_ui_create_setting_label(s_ui_ctx.settings_page, "页面隐藏", 105);
    s_ui_ctx.hide_switch = app_lcd_ui_create_setting_switch(s_ui_ctx.settings_page, s_ui_ctx.page_hidden, 104);

    app_lcd_ui_create_setting_label(s_ui_ctx.settings_page, "HID触摸", 174);
    s_ui_ctx.hid_switch = app_lcd_ui_create_setting_switch(s_ui_ctx.settings_page, s_ui_ctx.hid_touch_enabled, 173);

    lv_obj_t *version_label = lv_label_create(s_ui_ctx.settings_page);
    lv_label_set_text(version_label, "version v1.0.1");
    lv_obj_set_width(version_label, LCD_UI_MAIN_PANEL_W - 24);
    lv_obj_set_style_text_align(version_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(version_label, &simhei_30, 0);
    lv_obj_set_style_text_color(version_label, lv_color_hex(0x8A94A6), 0);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_obj_add_flag(s_ui_ctx.settings_page, LV_OBJ_FLAG_HIDDEN);
}

static void app_lcd_ui_start_intro_anim(lv_obj_t *obj, uint32_t delay)
{
    lv_obj_set_style_opa(obj, 0, 0);
    lv_obj_set_style_translate_y(obj, 12, 0);

    lv_anim_t fade;
    lv_anim_init(&fade);
    lv_anim_set_var(&fade, obj);
    lv_anim_set_values(&fade, 0, LV_OPA_COVER);
    lv_anim_set_duration(&fade, 420);
    lv_anim_set_delay(&fade, delay);
    lv_anim_set_path_cb(&fade, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&fade, app_lcd_ui_anim_set_opa);
    lv_anim_start(&fade);

    lv_anim_t slide;
    lv_anim_init(&slide);
    lv_anim_set_var(&slide, obj);
    lv_anim_set_values(&slide, 12, 0);
    lv_anim_set_duration(&slide, 520);
    lv_anim_set_delay(&slide, delay);
    lv_anim_set_path_cb(&slide, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&slide, app_lcd_ui_anim_set_translate_y);
    lv_anim_start(&slide);
}

static void app_lcd_ui_start_countdown_anim_locked(void)
{
    if (s_countdown_anim_started || s_ui_ctx.countdown_arc == NULL) {
        return;
    }

    s_countdown_anim_started = true;
    lv_arc_set_value(s_ui_ctx.countdown_arc, 0);

    lv_anim_t arc_anim;
    lv_anim_init(&arc_anim);
    lv_anim_set_var(&arc_anim, s_ui_ctx.countdown_arc);
    lv_anim_set_values(&arc_anim, 0, 100);
    lv_anim_set_duration(&arc_anim, LCD_UI_COUNTDOWN_MS);
    lv_anim_set_path_cb(&arc_anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&arc_anim, app_lcd_ui_anim_set_arc_value);
    lv_anim_start(&arc_anim);
}

static void app_lcd_ui_build_screen_locked(void)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0x203040), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    /* Background video is scanned out by the hardware YUV layer. */

    s_ui_ctx.main_panel = lv_obj_create(screen);
    app_lcd_ui_style_panel(s_ui_ctx.main_panel, LCD_UI_MAIN_PANEL_W, LCD_UI_MAIN_PANEL_H);
    lv_obj_align(s_ui_ctx.main_panel, LV_ALIGN_CENTER, 0, 40);

    s_ui_ctx.usb_panel = lv_obj_create(screen);
    app_lcd_ui_style_panel(s_ui_ctx.usb_panel, LCD_UI_USB_PANEL_W, LCD_UI_USB_PANEL_H);
    lv_obj_align_to(s_ui_ctx.usb_panel, s_ui_ctx.main_panel, LV_ALIGN_OUT_TOP_MID, 0, -LCD_UI_PANEL_GAP);

    s_ui_ctx.usb_label = lv_label_create(s_ui_ctx.usb_panel);
    lv_obj_set_width(s_ui_ctx.usb_label, LCD_UI_USB_PANEL_W - 32);
    lv_obj_set_style_text_align(s_ui_ctx.usb_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_ui_ctx.usb_label, &simhei_50, 0);
    lv_obj_set_style_text_color(s_ui_ctx.usb_label, lv_color_hex(0x102033), 0);
    lv_obj_set_style_text_opa(s_ui_ctx.usb_label, LV_OPA_COVER, 0);
    lv_obj_center(s_ui_ctx.usb_label);

    s_ui_ctx.settings_icon = app_lcd_ui_create_icon_button(s_ui_ctx.main_panel, LV_SYMBOL_SETTINGS, LCD_UI_SETTINGS_BTN_SIZE, 150);
    lv_obj_align(s_ui_ctx.settings_icon, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_add_event_cb(s_ui_ctx.settings_icon, app_lcd_ui_settings_icon_event_cb, LV_EVENT_CLICKED, NULL);

    s_ui_ctx.floating_settings_icon = app_lcd_ui_create_icon_button(screen, LV_SYMBOL_SETTINGS, LCD_UI_FLOATING_SETTINGS_BTN_SIZE, 150);
    lv_obj_align(s_ui_ctx.floating_settings_icon, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_add_event_cb(s_ui_ctx.floating_settings_icon, app_lcd_ui_settings_icon_event_cb, LV_EVENT_CLICKED, NULL);

    const int32_t spinner_size = (int32_t)(udisp_screen.width / 2U);
    s_ui_ctx.spinner = lv_spinner_create(s_ui_ctx.main_panel);
    // lv_spinner_set_anim_duration(s_ui_ctx.spinner, 900);
    // lv_spinner_set_arc_sweep(s_ui_ctx.spinner, 110);
    lv_obj_set_size(s_ui_ctx.spinner, spinner_size, spinner_size);
    lv_obj_set_style_bg_opa(s_ui_ctx.spinner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(s_ui_ctx.spinner, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ui_ctx.spinner, lv_color_hex(0xF28C28), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_ui_ctx.spinner, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_ui_ctx.spinner, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ui_ctx.spinner, lv_color_hex(0xF9D5AE), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(s_ui_ctx.spinner, 190, LV_PART_MAIN);
    lv_obj_align(s_ui_ctx.spinner, LV_ALIGN_CENTER, 0, 0);

    s_ui_ctx.status_label = lv_label_create(s_ui_ctx.main_panel);
    lv_obj_add_flag(s_ui_ctx.status_label, LV_OBJ_FLAG_HIDDEN);

    s_ui_ctx.countdown_arc = lv_arc_create(s_ui_ctx.main_panel);
    lv_obj_set_size(s_ui_ctx.countdown_arc, 152, 152);
    lv_arc_set_range(s_ui_ctx.countdown_arc, 0, 100);
    lv_arc_set_value(s_ui_ctx.countdown_arc, 0);
    lv_arc_set_rotation(s_ui_ctx.countdown_arc, 270);
    lv_arc_set_bg_angles(s_ui_ctx.countdown_arc, 0, 360);
    lv_obj_remove_flag(s_ui_ctx.countdown_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ui_ctx.countdown_arc, 11, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ui_ctx.countdown_arc, lv_color_hex(0xD7E4F2), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(s_ui_ctx.countdown_arc, 190, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ui_ctx.countdown_arc, 11, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ui_ctx.countdown_arc, lv_color_hex(0x2677E8), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(s_ui_ctx.countdown_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_align(s_ui_ctx.countdown_arc, LV_ALIGN_CENTER, 0, -24);
    lv_obj_add_flag(s_ui_ctx.countdown_arc, LV_OBJ_FLAG_HIDDEN);

    s_ui_ctx.countdown_label = lv_label_create(s_ui_ctx.main_panel);
    lv_obj_set_width(s_ui_ctx.countdown_label, LCD_UI_MAIN_PANEL_W - 40);
    lv_obj_set_style_text_align(s_ui_ctx.countdown_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_ui_ctx.countdown_label, &simhei_50, 0);
    lv_obj_set_style_text_color(s_ui_ctx.countdown_label, lv_color_hex(0x102033), 0);
    lv_obj_align(s_ui_ctx.countdown_label, LV_ALIGN_CENTER, 0, -24);
    lv_label_set_text(s_ui_ctx.countdown_label, "");
    lv_obj_add_flag(s_ui_ctx.countdown_label, LV_OBJ_FLAG_HIDDEN);

    app_lcd_ui_create_settings_page_locked();

    app_lcd_ui_start_intro_anim(s_ui_ctx.usb_panel, 80);
    app_lcd_ui_start_intro_anim(s_ui_ctx.main_panel, 180);

    s_ui_ctx.last_status[0] = '\0';
    s_countdown_anim_started = false;
    app_lcd_ui_update_waiting_status_locked();
    app_lcd_ui_apply_visibility_locked();
}

static void app_lcd_ui_update_waiting_status_locked(void)
{
    if (s_ui_ctx.settings_open) {
        return;
    }

    const bool usb_ready = udisp_host_alive();
    const char *status = usb_ready ? "USB已连接" : "USB未连接";

    if (s_ui_ctx.spinner) {
        lv_obj_remove_flag(s_ui_ctx.spinner, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_ui_ctx.countdown_arc) {
        lv_obj_add_flag(s_ui_ctx.countdown_arc, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_ui_ctx.countdown_label) {
        lv_obj_add_flag(s_ui_ctx.countdown_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(s_ui_ctx.countdown_label, "");
    }

    if (strncmp(s_ui_ctx.last_status, status, sizeof(s_ui_ctx.last_status)) == 0)
    {
        return;
    }

    snprintf(s_ui_ctx.last_status, sizeof(s_ui_ctx.last_status), "%s", status);
    lv_label_set_text(s_ui_ctx.usb_label, status);
    app_lcd_ui_apply_visibility_locked();
}

static void app_lcd_ui_update_countdown_locked(int seconds_left)
{
    if (s_ui_ctx.settings_open) {
        return;
    }

    lv_label_set_text(s_ui_ctx.usb_label, "USB已连接");
    lv_obj_add_flag(s_ui_ctx.spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ui_ctx.countdown_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_ui_ctx.countdown_label, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(s_ui_ctx.countdown_label, "%d", seconds_left);
    app_lcd_ui_start_countdown_anim_locked();
    snprintf(s_ui_ctx.last_status, sizeof(s_ui_ctx.last_status), "%s", "USB已连接");
    app_lcd_ui_apply_visibility_locked();
}


static void ui_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixels)
{
    (void)area;
    if (lv_display_flush_is_last(disp)) {
        aicos_dcache_clean_range(pixels, UI_BUFFER_SIZE);
        ui_layer.buf.phy_addr[0] = (uintptr_t)pixels;
        aicos_mutex_take(udisp_display_lock, AICOS_WAIT_FOREVER);
        mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_LAYER_CONFIG, &ui_layer);
        mpp_fb_ioctl(udisp_fb, AICFB_WAIT_FOR_VSYNC, NULL);
        aicos_mutex_give(udisp_display_lock);
    }
    lv_display_flush_ready(disp);
}
void udisp_ui_visibility(bool enable)
{
    if (!ui_active) return;
    ui_layer.enable = enable;
    aicos_mutex_take(udisp_display_lock, AICOS_WAIT_FOREVER);
    mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_LAYER_CONFIG, &ui_layer);
    aicos_mutex_give(udisp_display_lock);
}
void udisp_ui_frame_ready(void) { frame_ready = true; }
void udisp_ui_init(void)
{
    /* ARGB8888 already carries the LVGL opacity channel. Color-keying zero
     * would turn transparent pixels into a second, conflicting blend rule. */
    struct aicfb_ck_config ck = {AICFB_LAYER_TYPE_UI, 0, 0};
    struct aicfb_alpha_config alpha = {AICFB_LAYER_TYPE_UI, 1, AICFB_PIXEL_ALPHA_MODE, 255};
    lv_display_t *disp;
    if (ui_active) return;
    if (!lv_started) {
        lv_init();
        lv_tick_set_cb(rt_tick_get_millisecond);
        lv_started = true;
    }
    /* All rectangles must be disabled before DE changes the layer format.
     * The framebuffer format is selected by AICFB_RGB888 in usbdisp builds. */
    aicos_mutex_take(udisp_display_lock, AICOS_WAIT_FOREVER);
    for (unsigned rect = 0; rect < 4; rect++) {
        struct aicfb_layer_data off = {0};
        off.layer_id = AICFB_LAYER_TYPE_UI;
        off.rect_id = rect;
        mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_LAYER_CONFIG, &off);
    }
    aicos_mutex_give(udisp_display_lock);
    if (udisp_screen.smem_len < UI_BUFFER_SIZE * 2) {
        printf("usbdisp: framebuffer too small for UI windows\n");
        return;
    }
    /* Reuse the driver's full-screen allocation for two unscaled windows.
     * Full rendering swaps only at VSYNC and never draws into scanout. */
    ui_buffer = udisp_screen.framebuffer;
    ui_buffers[0] = ui_buffer;
    ui_buffers[1] = ui_buffer + UI_BUFFER_SIZE;
    disp = ui_display;
    if (!disp)
        disp = ui_display = lv_display_create(UDISP_UI_WIDTH, UDISP_UI_HEIGHT);
    /* Keep LVGL's per-pixel opacity in the ARGB8888 UI layer. The base
     * framebuffer is RGB888; the UI layer has its own four-byte pixels. */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_flush_cb(disp, ui_flush);
    memset(ui_buffer, 0, UI_BUFFER_SIZE * 2);
    aicos_dcache_clean_range(ui_buffer, UI_BUFFER_SIZE * 2);
    lv_display_set_buffers(disp, ui_buffers[0], ui_buffers[1],
                          UI_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
    ui_layer.layer_id = AICFB_LAYER_TYPE_UI;
    ui_layer.enable = 1;
    ui_layer.pos.x = (udisp_screen.width - UDISP_UI_WIDTH) / 2;
    ui_layer.pos.y = (udisp_screen.height - UDISP_UI_HEIGHT) / 2;
    ui_layer.buf.buf_type = MPP_PHY_ADDR;
    ui_layer.buf.phy_addr[0] = (uintptr_t)ui_buffer;
    ui_layer.buf.stride[0] = UDISP_UI_WIDTH * 4;
    ui_layer.buf.size.width = UDISP_UI_WIDTH;
    ui_layer.buf.size.height = UDISP_UI_HEIGHT;
    ui_layer.buf.format = MPP_FMT_ARGB_8888;
    ui_layer.enable = 0;
    mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_LAYER_CONFIG, &ui_layer);
    mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_CK_CONFIG, &ck);
    mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_ALPHA_CONFIG, &alpha);
    s_ui_ctx.page_hidden = udisp_page_hidden;
    s_ui_ctx.hid_touch_enabled = udisp_hid_enabled;
    app_lcd_ui_build_screen_locked();
    ui_active = true;
    udisp_ui_visibility(true);
}
void udisp_ui_deinit(void)
{
    if (!ui_active) return;
    ui_layer.enable = 0;
    aicos_mutex_take(udisp_display_lock, AICOS_WAIT_FOREVER);
    mpp_fb_ioctl(udisp_fb, AICFB_UPDATE_LAYER_CONFIG, &ui_layer);
    aicos_msleep(UDISP_PRESENT_PERIOD_MS);
    aicos_mutex_give(udisp_display_lock);
    /* Keep the LVGL core/display alive across the USB transition. Clearing
     * the active screen releases all page widgets and animations without
     * destroying the RT-Thread-backed global LVGL state. */
    if (ui_display)
        lv_obj_clean(lv_display_get_screen_active(ui_display));
    memset(&s_ui_ctx, 0, sizeof(s_ui_ctx));
    memset(&ui_layer, 0, sizeof(ui_layer));
    ui_buffer = NULL;
    ui_buffers[0] = NULL;
    ui_buffers[1] = NULL;
    frame_ready = false;
    counting = false;
    was_stream = false;
    ui_active = false;
}
void udisp_ui_command(int command, bool value)
{
    if (command == 1) app_lcd_ui_open_settings_locked();
    else if (command == 2) app_lcd_ui_close_settings_locked();
    else if (command == 3 || command == 4) {
        lv_obj_t *sw = command == 3 ? s_ui_ctx.hide_switch : s_ui_ctx.hid_switch;
        if (value) lv_obj_add_state(sw, LV_STATE_CHECKED);
        else lv_obj_remove_state(sw, LV_STATE_CHECKED);
        lv_obj_send_event(sw, LV_EVENT_VALUE_CHANGED, NULL);
        app_lcd_ui_apply_visibility_locked();
    }
}
void udisp_ui_poll(void)
{
    uint32_t now = aic_get_time_ms();
    if (udisp_stream_enabled && ui_active) {
        udisp_ui_deinit();
        was_stream = true;
        return;
    }
    if (!udisp_stream_enabled && !ui_active) {
        udisp_ui_init();
        was_stream = false;
    }
    if (!ui_active) return;
    udisp_settings_poll();
    if (!udisp_host_alive() && !udisp_stream_requested) {
        udisp_stream_requested = false;
        udisp_stream_enabled = false;
        udisp_stream_hint = false;
        counting = false;
    }
    if (was_stream && !udisp_stream_enabled) {
        frame_ready = false;
        s_ui_ctx.stream_detected = false;
        s_countdown_anim_started = false;
        lv_anim_delete(s_ui_ctx.countdown_arc, NULL);
        app_lcd_ui_update_waiting_status_locked();
        udisp_ui_visibility(true);
    }
    if (!udisp_stream_enabled) {
        if (udisp_stream_hint && udisp_host_alive()) {
            s_ui_ctx.stream_detected = true;
            if (!counting) { countdown_start = now; counting = true; }
            uint32_t elapsed = now - countdown_start;
            int left = elapsed < UDISP_COUNTDOWN_MS ?
                       (UDISP_COUNTDOWN_MS - elapsed + 999) / 1000 : 0;
            app_lcd_ui_update_countdown_locked(left);
            if (!left) udisp_stream_requested = true;
        } else {
            s_ui_ctx.stream_detected = false;
            s_countdown_anim_started = false;
            app_lcd_ui_update_waiting_status_locked();
        }
    } else if (frame_ready) {
        /* Hide the waiting overlay only when a native USB frame is visible. */
        udisp_ui_visibility(false);
    }
    was_stream = udisp_stream_enabled;
    lv_timer_handler();
}
