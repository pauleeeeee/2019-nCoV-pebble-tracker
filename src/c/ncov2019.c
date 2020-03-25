#include <pebble.h>

#include "./ncov2019.h"
#include "./windows/closestCases.h"
#include <pdc-transform/pdc-transform.h>

static Window *s_window;
static Layer *s_stats_layer;
static TextLayer *s_cases_layer, *s_deaths_layer, *s_deaths_caption_layer, *s_recovered_layer, *s_recovered_caption_layer, *s_your_location_layer, *s_cases_caption_layer, *s_loading_layer;
static GFont s_big_font, s_small_font, s_small_font_bold, s_medium_font;
static char s_closest_cases[512];
static PropertyAnimation *s_stats_layer_animation, *property_animation_fg_drift_up, *property_animation_bg_drift_up, *property_animation_fg_drift_down, *property_animation_bg_drift_down;

//for animation
static Layer *s_background_layer, *s_foreground_layer;
static GDrawCommandImage *s_corona_cell_large;
static GDrawCommandImage *s_corona_cells[8];
static Layer *s_corona_cell_layers[8];
static bool s_should_draw = true;
static int s_scale;
static int s_scale_increment;
static int s_angle;
static AppTimer *s_animation_timer;
static bool is_animating = false;

//for displayed data
static LocationStats s_location_stats[3];

// static char *s_locations[3];
// static char *s_cases[3];
// static char *s_deaths[3];
// static char *s_recovered[3];

static int s_selected_index = 0;

static int getRandomNumber(int lower, int upper) { 
  int num = (rand() % (upper - lower + 1)) + lower; 
  return num;
} 

static void clear_animating(){
  is_animating = false;
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void prv_long_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(s_loading_layer, "Loading...");
    layer_set_hidden(s_stats_layer, true);
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_cstring(iter, RequestUpdate, "update");
    app_message_outbox_send();
}


static void update_text(){
  text_layer_set_text(s_your_location_layer, s_location_stats[s_selected_index].location);
  text_layer_set_text(s_cases_layer, s_location_stats[s_selected_index].cases);
  text_layer_set_text(s_deaths_layer, s_location_stats[s_selected_index].deaths);
  text_layer_set_text(s_recovered_layer, s_location_stats[s_selected_index].recovered);

}

static void drift_up(){
  GRect window_bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect fg_frame = layer_get_frame(s_foreground_layer);
  GRect fg_bounds = layer_get_bounds(s_foreground_layer);
  property_animation_fg_drift_up = property_animation_create_layer_frame(s_foreground_layer, NULL, &GRect(fg_frame.origin.x, fg_frame.origin.y-window_bounds.size.h, fg_bounds.size.w, fg_bounds.size.h));
  Animation *animation_fg_drift_up = property_animation_get_animation(property_animation_fg_drift_up); 
  animation_set_duration(animation_fg_drift_up, 1000);
  animation_set_curve(animation_fg_drift_up, AnimationCurveEaseOut);
  animation_schedule(animation_fg_drift_up);
  
  GRect bg_frame = layer_get_frame(s_background_layer);
  GRect bg_bounds = layer_get_bounds(s_background_layer);
  property_animation_bg_drift_up = property_animation_create_layer_frame(s_background_layer, NULL, &GRect(bg_frame.origin.x, bg_frame.origin.y-window_bounds.size.h/2, bg_bounds.size.w, bg_bounds.size.h));
  Animation *animation_bg_drift_up = property_animation_get_animation(property_animation_bg_drift_up); 
  animation_set_duration(animation_bg_drift_up, 1000);
  animation_set_curve(animation_bg_drift_up, AnimationCurveEaseOut);
  animation_schedule(animation_bg_drift_up);

}

static void drift_down(){
  GRect window_bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect fg_frame = layer_get_frame(s_foreground_layer);
  GRect fg_bounds = layer_get_bounds(s_foreground_layer);
  property_animation_fg_drift_down = property_animation_create_layer_frame(s_foreground_layer, NULL, &GRect(fg_frame.origin.x, fg_frame.origin.y+window_bounds.size.h, fg_bounds.size.w, fg_bounds.size.h));
  Animation *animation_fg_drift_down = property_animation_get_animation(property_animation_fg_drift_down); 
  animation_set_duration(animation_fg_drift_down, 1000);
  animation_set_curve(animation_fg_drift_down, AnimationCurveEaseOut);
  animation_schedule(animation_fg_drift_down);

  GRect bg_frame = layer_get_frame(s_background_layer);
  GRect bg_bounds = layer_get_bounds(s_background_layer);
  property_animation_bg_drift_down = property_animation_create_layer_frame(s_background_layer, NULL, &GRect(bg_frame.origin.x, bg_frame.origin.y+window_bounds.size.h/2, bg_bounds.size.w, bg_bounds.size.h));
  Animation *animation_bg_drift_down = property_animation_get_animation(property_animation_bg_drift_down); 
  animation_set_duration(animation_bg_drift_down, 1000);
  animation_set_curve(animation_bg_drift_down, AnimationCurveEaseOut);
  animation_schedule(animation_bg_drift_down);

}

static void bottom_up(){
  update_text();
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  layer_set_frame(s_stats_layer, GRect(bounds.origin.x, bounds.size.h+bounds.size.h, bounds.size.w, bounds.size.h));
  s_stats_layer_animation = property_animation_create_layer_frame(s_stats_layer, NULL, &bounds);
  Animation *animation = property_animation_get_animation(s_stats_layer_animation);
  animation_set_duration(animation, 200);
  animation_set_curve(animation, AnimationCurveEaseOut);
  animation_schedule(animation);

}

static void top_down(){
  update_text();
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  layer_set_frame(s_stats_layer, GRect(bounds.origin.x, -bounds.size.h, bounds.size.w, bounds.size.h));
  s_stats_layer_animation = property_animation_create_layer_frame(s_stats_layer, NULL, &bounds);
  Animation *animation = property_animation_get_animation(s_stats_layer_animation);
  animation_set_duration(animation, 200);
  animation_set_curve(animation, AnimationCurveEaseOut);
  animation_schedule(animation);

}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_selected_index > 0 && (!is_animating)){
    is_animating = true;
    s_selected_index--;
    GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
    s_stats_layer_animation = property_animation_create_layer_frame(s_stats_layer, NULL, &GRect(bounds.origin.x, bounds.size.h, bounds.size.w, bounds.size.h));
    Animation *animation = property_animation_get_animation(s_stats_layer_animation);
    animation_set_duration(animation, 200);
    animation_set_curve(animation, AnimationCurveEaseOut);
    animation_schedule(animation);
    app_timer_register(200, top_down, NULL);
    drift_down();
    app_timer_register(1200, clear_animating, NULL);
  }

}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_selected_index < 2 && (!is_animating)) {
    is_animating = true;
    s_selected_index++;
    GRect bounds = layer_get_bounds(s_stats_layer);
    s_stats_layer_animation = property_animation_create_layer_frame(s_stats_layer, NULL, &GRect(bounds.origin.x, -bounds.size.h, bounds.size.w, bounds.size.h));
    Animation *animation = property_animation_get_animation(s_stats_layer_animation);
    animation_set_duration(animation, 200);
    animation_set_curve(animation, AnimationCurveEaseOut);
    animation_schedule(animation);
    app_timer_register(200, bottom_up, NULL);
    drift_up();
    app_timer_register(1200, clear_animating, NULL);
  }

  // cases_window_push(s_closest_cases);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, prv_long_select_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void s_foreground_layer_update_proc (Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  if (s_should_draw){
  // gdraw_command_image_draw(ctx, s_corona_cells[0], GPoint());
  gdraw_command_image_draw(ctx, s_corona_cells[1], GPoint((bounds.size.w/2)+getRandomNumber(-45,0), (bounds.size.h/2)+getRandomNumber(-45,0)));
  gdraw_command_image_draw(ctx, s_corona_cells[2], GPoint((bounds.size.w/2)+getRandomNumber(-75,0), (bounds.size.h/2)+getRandomNumber(-75,0)));
  s_should_draw = false;    
  }
}

static void animation_callback(void* data) {
  s_scale+=s_scale_increment;
  if(s_scale>30 || s_scale<10)
    s_scale_increment*=0;
  s_angle = (s_angle+1)%360;
  s_animation_timer = app_timer_register(300,animation_callback,NULL);
  layer_mark_dirty(s_background_layer);
}

static void s_background_layer_update_proc(Layer *layer, GContext *ctx) {
  // Place image in the center of the Window
  // GSize img_size = {.w=80,.h=80};
  GRect bounds = layer_get_bounds(layer);

  // Make sure it is in the middle of the frame
  // img_size.h = img_size.h * s_scale / 10;
  // img_size.w = img_size.w * s_scale / 10;
  // const GEdgeInsets frame_insets = {
  //   .top = (bounds.size.h - img_size.h) / 2 ,
  //   .left = (bounds.size.w - img_size.w) / 2
  // };
  graphics_context_set_fill_color(ctx,GColorWhite);
  // If the image was loaded successfully...
  if (s_corona_cell_large) {
    // Draw it
    pdc_transform_gdraw_command_image_draw_transformed(ctx, s_corona_cell_large, PBL_IF_ROUND_ELSE(GPoint(20,110),GPoint(0,105)),s_scale,s_angle,GColorWhite,GColorLightGray,0);
    // pdc_transform_gdraw_command_image_draw_transformed(ctx, s_corona_cell_large, GPoint((bounds.size.w/2)-50,(bounds.size.h/2)-25),s_scale,s_angle,GColorWhite,GColorLightGray,0);
  }
}

// static void wiggle_animate(){
//   uint16_t duration = 200;
//   GRect bounds = layer_get_frame(s_corona_cell_layers[5]);
//   PropertyAnimation *prop_animation = property_animation_create_layer_frame(s_corona_cell_layers[5], &bounds, &GRect(bounds.origin.x + 3, bounds.origin.y+6, bounds.size.w, bounds.size.h));
//   Animation *animation = property_animation_get_animation(prop_animation);
//   animation_set_duration(animation, duration);
//   animation_set_curve(animation, AnimationCurveEaseOut);

//   PropertyAnimation *prop_animation2 = property_animation_create_layer_frame(s_corona_cell_layers[5], &GRect(bounds.origin.x + 3, bounds.origin.y - 6, bounds.size.w, bounds.size.h), &GRect(bounds.origin.x + 6, bounds.origin.y, bounds.size.w, bounds.size.h));
//   Animation *animation2 = property_animation_get_animation(prop_animation2);
//   animation_set_duration(animation2, duration);
//   animation_set_curve(animation2, AnimationCurveEaseOut);

//   PropertyAnimation *prop_animation3 = property_animation_create_layer_frame(s_corona_cell_layers[5], &GRect(bounds.origin.x + 6, bounds.origin.y, bounds.size.w, bounds.size.h), &bounds);
//   Animation *animation3 = property_animation_get_animation(prop_animation3);
//   animation_set_duration(animation3, duration);
//   animation_set_curve(animation3, AnimationCurveEaseOut);

//   Animation *sequence = animation_sequence_create(animation, animation2, animation3);
//   animation_schedule(sequence);

//   app_timer_register(605, wiggle_animate, NULL);


// }

static PropertyAnimation *property_animations_up[8]; 
static PropertyAnimation *property_animations_down[8]; 
static Animation *animation_up[8];
static Animation *animation_down[8];
static Animation *sequences[8];

static void float_animation(){

  for ( int i=0; i<8; i++){
    GRect cell_bounds = layer_get_frame(s_corona_cell_layers[i]);
    int offset = getRandomNumber(8,12);
    property_animations_up[i] = property_animation_create_layer_frame(s_corona_cell_layers[i], &cell_bounds, &GRect(cell_bounds.origin.x, cell_bounds.origin.y+offset, cell_bounds.size.w, cell_bounds.size.h));
    animation_up[i] = property_animation_get_animation(property_animations_up[i]); 
    animation_set_duration(animation_up[i], 2500);
    animation_set_curve(animation_up[i], AnimationCurveEaseInOut);

    property_animations_down[i] = property_animation_create_layer_frame(s_corona_cell_layers[i], &GRect(cell_bounds.origin.x, cell_bounds.origin.y+offset, cell_bounds.size.w, cell_bounds.size.h), &cell_bounds);
    animation_down[i] = property_animation_get_animation(property_animations_down[i]); 
    animation_set_duration(animation_down[i], 2500);
    animation_set_curve(animation_down[i], AnimationCurveEaseInOut);

    sequences[i] = animation_sequence_create(animation_up[i], animation_down[i], NULL);
    animation_schedule(sequences[i]);

  }

    // for ( int i=0; i<8; i++ ){
    //   animation_schedule(sequences[i]);
    // }

  app_timer_register(5000, float_animation, NULL);

}

static void s_c_c_l_u_p_0(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[0], GPoint(0,0));
}
static void s_c_c_l_u_p_1(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[1], GPoint(0,0));
}
static void s_c_c_l_u_p_2(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[2], GPoint(0,0));
}
static void s_c_c_l_u_p_3(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[3], GPoint(0,0));
}
static void s_c_c_l_u_p_4(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[4], GPoint(0,0));
}
static void s_c_c_l_u_p_5(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[5], GPoint(0,0));
}
static void s_c_c_l_u_p_6(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[6], GPoint(0,0));
}
static void s_c_c_l_u_p_7(Layer *layer, GContext *ctx) {
  gdraw_command_image_draw(ctx, s_corona_cells[7], GPoint(0,0));
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);


  s_corona_cell_large = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_GIANT);
  s_scale= 10;
  s_scale_increment= 0;
  s_angle= 0;
  // Create canvas Layer and set up the update procedure
  s_background_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h*2));
  layer_set_update_proc(s_background_layer, s_background_layer_update_proc);
  layer_add_child(window_layer, s_background_layer);
  s_animation_timer = app_timer_register(300,animation_callback,NULL);

  s_small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_small_font_bold = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_big_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  
  s_stats_layer = layer_create(bounds);
  layer_add_child(window_layer, s_stats_layer);

  s_loading_layer = text_layer_create(GRect(bounds.origin.x, (bounds.size.h/2)-20, bounds.size.w, 40));
  text_layer_set_font(s_loading_layer, s_medium_font);
  text_layer_set_text(s_loading_layer, "Loading...");
  text_layer_set_background_color(s_loading_layer, GColorClear);
  text_layer_set_text_alignment(s_loading_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_loading_layer));

  s_your_location_layer = text_layer_create(GRect(0, 5, bounds.size.w, 30));
  text_layer_set_font(s_your_location_layer, s_medium_font);
  text_layer_set_text(s_your_location_layer, "Loading...");
  text_layer_set_background_color(s_your_location_layer, GColorClear);
  text_layer_set_text_alignment(s_your_location_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_your_location_layer));

  s_cases_layer = text_layer_create(GRect(0, 30, bounds.size.w, 34));
  text_layer_set_font(s_cases_layer, s_big_font);
  text_layer_set_text(s_cases_layer, "000");
  text_layer_set_background_color(s_cases_layer, GColorClear);
  text_layer_set_text_alignment(s_cases_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_cases_layer));

  s_cases_caption_layer = text_layer_create(GRect(0, 58, bounds.size.w, 20));
  text_layer_set_font(s_cases_caption_layer, s_small_font_bold);
  text_layer_set_text(s_cases_caption_layer, "confirmed cases");
  text_layer_set_background_color(s_cases_caption_layer, GColorClear);
  text_layer_set_text_alignment(s_cases_caption_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_cases_caption_layer));

  s_deaths_layer = text_layer_create(GRect(0, 71, bounds.size.w, 34));
  text_layer_set_font(s_deaths_layer, s_big_font);
  text_layer_set_text(s_deaths_layer, "000");
  text_layer_set_background_color(s_deaths_layer, GColorClear);
  text_layer_set_text_alignment(s_deaths_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_deaths_layer));

  s_deaths_caption_layer = text_layer_create(GRect(0, 99, bounds.size.w, 20));
  text_layer_set_font(s_deaths_caption_layer, s_small_font_bold);
  text_layer_set_text(s_deaths_caption_layer, "deaths");
  text_layer_set_background_color(s_deaths_caption_layer, GColorClear);
  text_layer_set_text_alignment(s_deaths_caption_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_deaths_caption_layer));

  s_recovered_layer = text_layer_create(GRect(0, 113, bounds.size.w, 34));
  text_layer_set_font(s_recovered_layer, s_big_font);
  text_layer_set_text(s_recovered_layer, "000");
  text_layer_set_background_color(s_recovered_layer, GColorClear);
  text_layer_set_text_alignment(s_recovered_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_recovered_layer));

  s_recovered_caption_layer = text_layer_create(GRect(0, 142, bounds.size.w, 20));
  text_layer_set_font(s_recovered_caption_layer, s_small_font_bold);
  text_layer_set_text(s_recovered_caption_layer, "recovered");
  text_layer_set_background_color(s_recovered_caption_layer, GColorClear);
  text_layer_set_text_alignment(s_recovered_caption_layer, GTextAlignmentCenter);
  layer_add_child(s_stats_layer, text_layer_get_layer(s_recovered_caption_layer));

  layer_set_hidden(s_stats_layer, true);

  // s_closest_location_cases_layer = text_layer_create(GRect(0, (bounds.size.h/2)+22, bounds.size.w, 20));
  // text_layer_set_font(s_closest_location_cases_layer, s_small_font);
  // text_layer_set_text(s_closest_location_cases_layer, "");
  // text_layer_set_background_color(s_closest_location_cases_layer, GColorClear);
  // text_layer_set_text_alignment(s_closest_location_cases_layer, GTextAlignmentCenter);
  // layer_add_child(window_layer, text_layer_get_layer(s_closest_location_cases_layer));

  // s_corona_layer = text_layer_create(GRect(0, 10, bounds.size.w, 30));
  // text_layer_set_font(s_corona_layer, s_medium_font);
  // text_layer_set_text(s_corona_layer, "2019-nCoV");
  // text_layer_set_background_color(s_corona_layer, GColorClear);
  // text_layer_set_text_alignment(s_corona_layer, GTextAlignmentCenter);
  // layer_add_child(window_layer, text_layer_get_layer(s_corona_layer));

  // s_your_location_layer = text_layer_create(GRect(0, bounds.size.h-30, bounds.size.w, 30));
  // text_layer_set_font(s_your_location_layer, s_medium_font);
  // text_layer_set_text(s_your_location_layer, "");
  // text_layer_set_background_color(s_your_location_layer, GColorClear);
  // text_layer_set_text_alignment(s_your_location_layer, GTextAlignmentCenter);
  // layer_add_child(window_layer, text_layer_get_layer(s_your_location_layer));

  // Create canvas Layer and set up the update procedure
  s_foreground_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h*3));
  // layer_set_update_proc(s_foreground_layer, s_foreground_layer_update_proc);
  layer_add_child(window_layer, s_foreground_layer);


  s_corona_cells[5] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_LARGE);
  s_corona_cell_layers[5] = layer_create(GRect(bounds.origin.x-45, bounds.origin.y-25, 100, 100));
  layer_set_update_proc(s_corona_cell_layers[5],s_c_c_l_u_p_5);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[5]);

  s_corona_cells[2] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_LARGE);
  s_corona_cell_layers[2] = layer_create(GRect(bounds.size.w-35, (bounds.size.h * 2) + (bounds.size.h / 2), 100, 100));
  layer_set_update_proc(s_corona_cell_layers[2],s_c_c_l_u_p_2);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[2]);

  s_corona_cells[1] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_MEDIUM);
  s_corona_cell_layers[1] = layer_create(GRect((bounds.size.w - 35), bounds.size.h/2, 100, 100));
  // s_corona_cell_layers[1] = layer_create(GRect((bounds.size.w/2)+getRandomNumber(-50,0), (bounds.size.h/2)+getRandomNumber(-50,0), 100, 100));
  layer_set_update_proc(s_corona_cell_layers[1],s_c_c_l_u_p_1);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[1]);

  s_corona_cells[4] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_MEDIUM);
  s_corona_cell_layers[4] = layer_create(GRect(bounds.origin.x-25, bounds.size.h+(bounds.size.h/2), 100, 100));
  layer_set_update_proc(s_corona_cell_layers[4],s_c_c_l_u_p_4);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[4]);

  s_corona_cells[0] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_SMALL);
  s_corona_cell_layers[0] = layer_create(GRect((bounds.size.w/2)-20, (bounds.size.h)-20, 100, 100));
  layer_set_update_proc(s_corona_cell_layers[0],s_c_c_l_u_p_0);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[0]);

  s_corona_cells[3] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_SMALL);
  s_corona_cell_layers[3] = layer_create(GRect(bounds.size.w-20, bounds.size.h + (bounds.size.h/2) - 35, 100, 100));
  layer_set_update_proc(s_corona_cell_layers[3],s_c_c_l_u_p_3);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[3]);

  s_corona_cells[6] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_SMALL);
  s_corona_cell_layers[6] = layer_create(GRect(bounds.origin.x-15, (bounds.size.h*2) + (bounds.size.h/2) - 30, 100, 100));
  layer_set_update_proc(s_corona_cell_layers[6],s_c_c_l_u_p_6);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[6]);

  s_corona_cells[7] = gdraw_command_image_create_with_resource(RESOURCE_ID_CORONA_CELL_SMALL);
  s_corona_cell_layers[7] = layer_create(GRect(bounds.origin.x+5, (bounds.size.h*3)-20, 100, 100));
  layer_set_update_proc(s_corona_cell_layers[7],s_c_c_l_u_p_7);
  layer_add_child(s_foreground_layer, s_corona_cell_layers[7]);


  window_set_background_color(s_window, GColorMelon);

  float_animation();

}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_cases_layer);
  // text_layer_destroy(s_closest_location_cases_layer);
  text_layer_destroy(s_cases_caption_layer);
  text_layer_destroy(s_your_location_layer);
  gdraw_command_image_destroy(s_corona_cell_large);
  layer_destroy(s_background_layer);
  layer_destroy(s_stats_layer);

}


static void in_received_handler(DictionaryIterator *iter, void *context) {
  
  int index = 0;

  Tuple *scope_index_tuple = dict_find(iter, Scope);
  if (scope_index_tuple){
    index = scope_index_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "got scope tuple %d", index);
  }

  Tuple *location_tuple = dict_find (iter, Location);
  if (location_tuple){
    strcpy(s_location_stats[index].location, location_tuple->value->cstring);    
    // s_locations[index] = location_tuple->value->cstring;
    // text_layer_set_text(s_your_location_layer, location_tuple->value->cstring);
  }

  Tuple *cases_tuple = dict_find (iter, Cases);
  if (cases_tuple){
    strcpy(s_location_stats[index].cases, cases_tuple->value->cstring);    
    // s_cases[index] = cases_tuple->value->cstring;
    // text_layer_set_text(s_cases_layer, cases_tuple->value->cstring);
  }

  Tuple *deaths_tuple = dict_find (iter, Deaths);
  if (deaths_tuple){
    strcpy(s_location_stats[index].deaths, deaths_tuple->value->cstring);    
    // s_deaths[index] = deaths_tuple->value->cstring;
    // text_layer_set_text(s_deaths_layer, deaths_tuple->value->cstring);
  }

  Tuple *recovered_tuple = dict_find (iter, Recovered);
  if (recovered_tuple){
    strcpy(s_location_stats[index].recovered, recovered_tuple->value->cstring);    
    // s_recovered[index] = recovered_tuple->value->cstring;
    // text_layer_set_text(s_recovered_layer, recovered_tuple->value->cstring);
  }

  Tuple *ready_tuple = dict_find (iter, Ready);
  if (ready_tuple){
    if (ready_tuple->value->int32 == 1) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "got ready tuple %d", 1);
      text_layer_set_text(s_loading_layer, "");
      layer_set_hidden(s_stats_layer, false);
      update_text();
    }
    // text_layer_set_text(s_cases_layer, recovered_tuple->value->cstring);
  }

  // Tuple *closest_cases_tuple = dict_find (iter, ClosestCases);
  // if (closest_cases_tuple){
  //   strncpy(s_closest_cases, closest_cases_tuple->value->cstring, sizeof(s_closest_cases));
  // }
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
