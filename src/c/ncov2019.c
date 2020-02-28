#include <pebble.h>

#include "./ncov2019.h"
#include "./windows/closestCases.h"


static Window *s_window;
static TextLayer *s_corona_layer, *s_you_layer, *s_distance_layer, *s_closest_outbreak_layer, *s_closest_location_layer, *s_closest_location_cases_layer;
static GFont s_big_font, s_small_font, s_medium_font;
static char s_closest_cases[512];


static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  cases_window_push(s_closest_cases);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_big_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);


  s_distance_layer = text_layer_create(GRect(0, (bounds.size.h/2)-20, bounds.size.w, 34));
  text_layer_set_font(s_distance_layer, s_big_font);
  text_layer_set_text(s_distance_layer, "loading");
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));

  s_closest_outbreak_layer = text_layer_create(GRect(0, (bounds.size.h/2)-30, bounds.size.w, 20));
  text_layer_set_font(s_closest_outbreak_layer, s_small_font);
  text_layer_set_text(s_closest_outbreak_layer, "closest outbreak");
  text_layer_set_background_color(s_closest_outbreak_layer, GColorClear);
  text_layer_set_text_alignment(s_closest_outbreak_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_closest_outbreak_layer));

  s_closest_location_layer = text_layer_create(GRect(0, (bounds.size.h/2)+10, bounds.size.w, 20));
  text_layer_set_font(s_closest_location_layer, s_small_font);
  text_layer_set_text(s_closest_location_layer, "");
  text_layer_set_background_color(s_closest_location_layer, GColorClear);
  text_layer_set_text_alignment(s_closest_location_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_closest_location_layer));

  s_closest_location_cases_layer = text_layer_create(GRect(0, (bounds.size.h/2)+22, bounds.size.w, 20));
  text_layer_set_font(s_closest_location_cases_layer, s_small_font);
  text_layer_set_text(s_closest_location_cases_layer, "");
  text_layer_set_background_color(s_closest_location_cases_layer, GColorClear);
  text_layer_set_text_alignment(s_closest_location_cases_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_closest_location_cases_layer));

  s_corona_layer = text_layer_create(GRect(0, 0, bounds.size.w, 30));
  text_layer_set_font(s_corona_layer, s_medium_font);
  text_layer_set_text(s_corona_layer, "2019-nCoV");
  text_layer_set_background_color(s_corona_layer, GColorClear);
  text_layer_set_text_alignment(s_corona_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_corona_layer));

  s_you_layer = text_layer_create(GRect(0, bounds.size.h-30, bounds.size.w, 30));
  text_layer_set_font(s_you_layer, s_medium_font);
  text_layer_set_text(s_you_layer, "");
  text_layer_set_background_color(s_you_layer, GColorClear);
  text_layer_set_text_alignment(s_you_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_you_layer));

}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_distance_layer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *distance_tuple = dict_find (iter, Distance);
  if (distance_tuple){
    text_layer_set_text(s_distance_layer, distance_tuple->value->cstring);
  }
  Tuple *location_tuple = dict_find (iter, Location);
  if (location_tuple){
    text_layer_set_text(s_closest_location_layer, location_tuple->value->cstring);
  }
  Tuple *location_cases_tuple = dict_find (iter, LocationCases);
  if (location_cases_tuple){
    text_layer_set_text(s_closest_location_cases_layer, location_cases_tuple->value->cstring);
  }
  Tuple *closest_cases_tuple = dict_find (iter, ClosestCases);
  if (closest_cases_tuple){
    strncpy(s_closest_cases, closest_cases_tuple->value->cstring, sizeof(s_closest_cases));
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context){
  //handle failed message
}

static void prv_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  //instantiate appmessages
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
