#include "./closestCases.h"

static Window *s_main_window;
static TextLayer *s_closest_cases_layer;
static char *s_closest_cases;
static TextLayer *s_title_layer;

void window_load(Window *window){
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_title_layer = text_layer_create(GRect(0,-2,bounds.size.w, 15));
    text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text(s_title_layer, "nearest outbreaks");
    text_layer_set_background_color(s_title_layer, GColorClear);
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_title_layer));


    s_closest_cases_layer = text_layer_create(GRect(5, 10, bounds.size.w, bounds.size.h));
    text_layer_set_font(s_closest_cases_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(s_closest_cases_layer, s_closest_cases);
    text_layer_set_background_color(s_closest_cases_layer, GColorClear);
    text_layer_set_text_alignment(s_closest_cases_layer, GTextAlignmentLeft);
    text_layer_set_overflow_mode(s_closest_cases_layer, GTextOverflowModeTrailingEllipsis);
    layer_add_child(window_layer, text_layer_get_layer(s_closest_cases_layer));

}


void window_unload(Window *window){
    text_layer_destroy(s_closest_cases_layer);
    text_layer_destroy(s_title_layer);
    s_main_window = NULL;
}

void cases_window_push(char *cases){
    s_closest_cases = cases;
    // text_layer_set_text(s_closest_cases_layer, s_closest_cases);
    if(!s_main_window) {
        s_main_window = window_create();
        window_set_background_color(s_main_window, GColorWhite);
        window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
        });
    }
    window_stack_push(s_main_window, true);
}