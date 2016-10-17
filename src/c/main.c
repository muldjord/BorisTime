#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_time_shadow_layer;
static TextLayer *s_date_layer;
static TextLayer *s_date_shadow_layer;
static GFont s_time_font;
static BitmapLayer *borisLayer;
static TextLayer *s_weather_layer;
static TextLayer *s_weather_shadow_layer;
static GFont s_weather_font;
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];
static int s_battery_level;
static Layer *s_battery_layer;
static AppTimer *frameTimer;
static AppTimer *behavTimer;
static uint32_t state;
static int borisX;
static int borisY;
static int borisSize;
static GBitmap *borisBitmap;
static GBitmapSequence *curBehav;

static GBitmapSequence *bhStanding;
static GBitmapSequence *bhSleeping;
static GBitmapSequence *bhWalkLeft;
static GBitmapSequence *bhWalkRight;
static GBitmapSequence *bhWalkDown;
static GBitmapSequence *bhWalkUp;
static GBitmapSequence *bhShredding;
static GBitmapSequence *bhEating;

#define NOOFBEHAVS 8

#define STANDING 0
#define SLEEPING 1
#define WALKLEFT 2
#define WALKRIGHT 3
#define WALKUP 4
#define WALKDOWN 5
#define SHREDDING 6
#define EATING 7

static void loadBehavs() {
  bhStanding = gbitmap_sequence_create_with_resource(RESOURCE_ID_STANDING);
  bhSleeping = gbitmap_sequence_create_with_resource(RESOURCE_ID_SLEEPING);
  bhWalkLeft = gbitmap_sequence_create_with_resource(RESOURCE_ID_WALKLEFT);
  bhWalkRight = gbitmap_sequence_create_with_resource(RESOURCE_ID_WALKRIGHT);
  bhWalkDown = gbitmap_sequence_create_with_resource(RESOURCE_ID_WALKDOWN);
  bhWalkUp = gbitmap_sequence_create_with_resource(RESOURCE_ID_WALKUP);
  bhShredding = gbitmap_sequence_create_with_resource(RESOURCE_ID_SHREDDING);
  bhEating = gbitmap_sequence_create_with_resource(RESOURCE_ID_EATING);
}

static void nextFrame() {
  uint32_t nextDelay;

  // Advance to the next APNG frame, and get the delay for this frame
  if(gbitmap_sequence_update_bitmap_next_frame(curBehav, borisBitmap, &nextDelay)) {
    bitmap_layer_set_bitmap(borisLayer, borisBitmap);
    layer_mark_dirty(bitmap_layer_get_layer(borisLayer));
  }

  // Move Boris if current animation is a walking animation
  if(state == WALKLEFT) {
    borisX = borisX - 2;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(borisX, borisY, borisSize, borisSize));
  } else if(state == WALKRIGHT) {
    borisX = borisX + 2;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(borisX, borisY, borisSize, borisSize));
  } else if(state == WALKDOWN) {
    borisY = borisY + 1;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(borisX, borisY, borisSize, borisSize));
  } else if(state == WALKUP) {
    borisY = borisY - 1;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(borisX, borisY, borisSize, borisSize));
  }
  
  // Pebble Time resolution is 144 x 168
  // Make sure Boris doesn't get too far out of bounds
  int min = -borisSize;
  int maxX = 168;
  int maxY = 144;
  if(borisX < -min || borisX > maxX || borisY < -min || borisY > maxY) {
    if(borisX < -min) {
      borisX = -min;
    }
    if(borisX > maxX) {
      borisX = maxX;
    }
    if(borisY < -min) {
      borisY = -min;
    }
    if(borisY > maxY) {
      borisY = maxY;
    }
    app_timer_reschedule(behavTimer, 0);
    return;
  }

  // Timer for that frame's delay
  frameTimer = app_timer_register(nextDelay, nextFrame, NULL);
}

static void changeBehaviour() {
  // Stop the current frame timer
  app_timer_cancel(frameTimer);
  
  // Choose a random behaviour unless one is specified
  state = rand() % NOOFBEHAVS;
  switch(state) {
    case STANDING:
      curBehav = bhStanding;
    break;
    case SLEEPING:
      curBehav = bhSleeping;
    break;
    case WALKLEFT:
      curBehav = bhWalkLeft;
    break;
    case WALKRIGHT:
      curBehav = bhWalkRight;
    break;
    case WALKDOWN:
      curBehav = bhWalkDown;
    break;
    case WALKUP:
      curBehav = bhWalkUp;
    break;
    case SHREDDING:
      curBehav = bhShredding;
    break;
    case EATING:
      curBehav = bhEating;
    break;
  }
  // Make sure we start the animation from the beginning
  gbitmap_sequence_restart(curBehav);
  
  // Set timeout for next behaviour change
  behavTimer = app_timer_register((rand() % 3000) + 3000, changeBehaviour, NULL);

  // Start the animation again
  nextFrame();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char time_buffer[8];
  static char date_buffer[8];
  strftime(time_buffer, sizeof(time_buffer), (clock_is_24h_style() ? "%H:%M" : "%I:%M"), tick_time);
  strftime(date_buffer, sizeof(date_buffer), "%d/%m", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, time_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
  text_layer_set_text(s_time_shadow_layer, time_buffer);
  text_layer_set_text(s_date_shadow_layer, date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Calculate the relevant width of the bar
  int width = (int)(((float)s_battery_level / 100.0F) * 144.0F);
  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
   // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Load all behaviours
  loadBehavs();

  // Create blank GBitmap using APNG frame size
  GSize frame_size = gbitmap_sequence_get_bitmap_size(bhStanding);
  borisBitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);

  // Create BitmapLayer to display the GBitmap
  borisLayer = bitmap_layer_create(GRect(borisX, borisY, borisSize, borisSize));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(borisLayer, borisBitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(borisLayer));

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_48));
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_20));

  s_time_layer = text_layer_create(GRect(0, 10, bounds.size.w, 50));
  s_date_layer = text_layer_create(GRect(0, 75, bounds.size.w, 50));
  s_time_shadow_layer = text_layer_create(GRect(0, 14, bounds.size.w, 50));
  s_date_shadow_layer = text_layer_create(GRect(0, 79, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_weather_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Repeat for shadow layers
  text_layer_set_background_color(s_time_shadow_layer, GColorClear);
  text_layer_set_text_color(s_time_shadow_layer, GColorBlack);
  text_layer_set_font(s_time_shadow_layer, s_time_font);
  text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentCenter);

  text_layer_set_background_color(s_date_shadow_layer, GColorClear);
  text_layer_set_text_color(s_date_shadow_layer, GColorBlack);
  text_layer_set_font(s_date_shadow_layer, s_weather_font);
  text_layer_set_text_alignment(s_date_shadow_layer, GTextAlignmentCenter);
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(
  GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
  s_weather_shadow_layer = text_layer_create(
  GRect(0, PBL_IF_ROUND_ELSE(129, 124), bounds.size.w, 25));

  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // Style the shadow
  text_layer_set_background_color(s_weather_shadow_layer, GColorClear);
  text_layer_set_text_color(s_weather_shadow_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_shadow_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_shadow_layer, "Loading...");

  // Create second custom font, apply it and add to Window
  text_layer_set_font(s_weather_layer, s_weather_font);
  text_layer_set_font(s_weather_shadow_layer, s_weather_font);
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(0, 150, 144, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add shadow layers
  layer_add_child(window_layer, text_layer_get_layer(s_time_shadow_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_shadow_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_shadow_layer));
  
  // Add front font layers
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_time_shadow_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_date_shadow_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_weather_shadow_layer);
  gbitmap_destroy(borisBitmap);
  bitmap_layer_destroy(borisLayer);
  fonts_unload_custom_font(s_weather_font);
  fonts_unload_custom_font(s_time_font);
  layer_destroy(s_battery_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
    text_layer_set_text(s_weather_shadow_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void init() {
  state = STANDING;
  borisX = 10;
  borisY = 72;
  borisSize = 32;
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  window_set_background_color(s_main_window, GColorBlack);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  battery_state_service_subscribe(battery_callback);

  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // Initialize Boris ai to get it going
  changeBehaviour();
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}