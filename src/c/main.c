#include <pebble.h>

static Window *mainWindow;
static TextLayer *timeLayer;
static TextLayer *timeShadowLayer;
static TextLayer *dateLayer;
static TextLayer *dateShadowLayer;
static GFont timeFont;
static BitmapLayer *borisLayer;
static BitmapLayer *weatherIconLayer;
static TextLayer *weatherTextLayer;
static TextLayer *weatherTextShadowLayer;
static char temperatureBuffer[4];
static GFont weatherFont;
static int batteryLevel;
static Layer *batteryLayer;
static AppTimer *frameTimer;
static AppTimer *behavTimer;
static GBitmap *borisBitmap;
static GBitmap *weatherBitmap;
static GBitmapSequence *curBehav;

static GBitmap *wIcon01d;
static GBitmap *wIcon01n;
static GBitmap *wIcon02d;
static GBitmap *wIcon02n;
static GBitmap *wIcon03d;
static GBitmap *wIcon03n;
static GBitmap *wIcon04d;
static GBitmap *wIcon04n;
static GBitmap *wIcon09d;
static GBitmap *wIcon09n;
static GBitmap *wIcon10d;
static GBitmap *wIcon10n;
static GBitmap *wIcon11d;
static GBitmap *wIcon11n;
static GBitmap *wIcon13d;
static GBitmap *wIcon13n;
static GBitmap *wIcon50d;
static GBitmap *wIcon50n;

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

static void nextFrame();
static void changeBehaviour(int newState);

// Persistent storage key
#define SETTINGS_KEY 1

// Define our settings struct
typedef struct AppSettings {
  GColor bgColor;
  uint32_t state;
  int borisX;
  int borisY;
  int borisSize;
} AppSettings;

static AppSettings settings;

static void defaultSettings() {
  settings.bgColor = GColorBlack;
  settings.state = STANDING;
  settings.borisX = 10;
  settings.borisY = 72;
  settings.borisSize = 32;
}

// Read settings from persistent storage
static void loadSettings() {
  // Load the default settings
  defaultSettings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void saveSettings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

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

static void pickRandomBehav() {
  changeBehaviour(666);
}

static void changeBehaviour(int newState) {
  // Stop the current frame timer
  app_timer_cancel(frameTimer);
  app_timer_cancel(behavTimer);
  
  // Choose a random behaviour unless one is specified
  if(newState != 666) {
    settings.state = newState;
  } else {
    settings.state = rand() % NOOFBEHAVS;
  }
  //settings.state = WALKLEFT; // Uncomment to test / force behaviour
  switch(settings.state) {
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
  behavTimer = app_timer_register((rand() % 3000) + 3000, pickRandomBehav, NULL);

  // Start the animation again
  nextFrame();
}

static void nextFrame() {
  uint32_t nextDelay;

  // Advance to the next APNG frame, and get the delay for this frame
  if(gbitmap_sequence_update_bitmap_next_frame(curBehav, borisBitmap, &nextDelay)) {
    bitmap_layer_set_bitmap(borisLayer, borisBitmap);
    layer_mark_dirty(bitmap_layer_get_layer(borisLayer));
  }

  // Move Boris if current animation is a walking animation
  if(settings.state == WALKLEFT) {
    settings.borisX = settings.borisX - 2;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(settings.borisX, settings.borisY,
                                                              settings.borisSize, settings.borisSize));
  } else if(settings.state == WALKRIGHT) {
    settings.borisX = settings.borisX + 2;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(settings.borisX, settings.borisY,
                                                              settings.borisSize, settings.borisSize));
  } else if(settings.state == WALKDOWN) {
    settings.borisY = settings.borisY + 1;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(settings.borisX, settings.borisY,
                                                              settings.borisSize, settings.borisSize));
  } else if(settings.state == WALKUP) {
    settings.borisY = settings.borisY - 1;
    layer_set_frame(bitmap_layer_get_layer(borisLayer), GRect(settings.borisX, settings.borisY,
                                                              settings.borisSize, settings.borisSize));
  }
  
  // Pebble Time resolution is 144 x 168
  // Make sure Boris doesn't get too far out of bounds
  int min = settings.borisSize;
  int maxX = 144;
  int maxY = 168;
  if(settings.borisX < -min || settings.borisX > maxX ||
     settings.borisY < -min || settings.borisY > maxY) {
    if(settings.borisX < -min) {
      settings.borisX = maxX;
    }
    if(settings.borisX > maxX) {
      settings.borisX = -min;
    }
    if(settings.borisY < -min) {
      settings.borisY = maxY;
    }
    if(settings.borisY > maxY) {
      settings.borisY = -min;
    }
    return;
  }

  // Timer for that frame's delay
  frameTimer = app_timer_register(nextDelay, nextFrame, NULL);
}

static void loadWeatherIcons() {
  wIcon01d = gbitmap_create_with_resource(RESOURCE_ID_01D);
  wIcon01n = gbitmap_create_with_resource(RESOURCE_ID_01N);
  wIcon02d = gbitmap_create_with_resource(RESOURCE_ID_02D);
  wIcon02n = gbitmap_create_with_resource(RESOURCE_ID_02N);
  wIcon03d = gbitmap_create_with_resource(RESOURCE_ID_03D);
  wIcon03n = gbitmap_create_with_resource(RESOURCE_ID_03N);
  wIcon04d = gbitmap_create_with_resource(RESOURCE_ID_04D);
  wIcon04n = gbitmap_create_with_resource(RESOURCE_ID_04N);
  wIcon09d = gbitmap_create_with_resource(RESOURCE_ID_09D);
  wIcon09n = gbitmap_create_with_resource(RESOURCE_ID_09N);
  wIcon10d = gbitmap_create_with_resource(RESOURCE_ID_10D);
  wIcon10n = gbitmap_create_with_resource(RESOURCE_ID_10N);
  wIcon11d = gbitmap_create_with_resource(RESOURCE_ID_11D);
  wIcon11n = gbitmap_create_with_resource(RESOURCE_ID_11N);
  wIcon13d = gbitmap_create_with_resource(RESOURCE_ID_13D);
  wIcon13n = gbitmap_create_with_resource(RESOURCE_ID_13N);
  wIcon50d = gbitmap_create_with_resource(RESOURCE_ID_50D);
  wIcon50n = gbitmap_create_with_resource(RESOURCE_ID_50N);
}

static void updateTime() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tickTime = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char timeBuffer[8];
  static char dateBuffer[16];
  //strftime(timeBuffer, sizeof(timeBuffer), (clock_is_24h_style() ? "%H:%M" : "%I:%M"), tickTime);
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", tickTime);
  strftime(dateBuffer, sizeof(dateBuffer), "%d %B", tickTime);
  // Display this time on the TextLayer
  text_layer_set_text(timeLayer, timeBuffer);
  text_layer_set_text(dateLayer, dateBuffer);
  text_layer_set_text(timeShadowLayer, timeBuffer);
  text_layer_set_text(dateShadowLayer, dateBuffer);
}

static void batteryUpdateProc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Calculate the relevant width of the bar
  int width = (int)(((float)batteryLevel / 100.0F) * 144.0F);
  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void tickHandler(struct tm *tickTime, TimeUnits unitsChanged) {
  updateTime();
   // Get weather update every 30 minutes
  if(tickTime->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void mainWindowLoad(Window *window) {
  // Get information about the Window
  Layer *windowLayer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(windowLayer);

  // Load all behaviours
  loadBehavs();

  // load all weather icons
  loadWeatherIcons();
  
  // Create blank GBitmap using APNG frame size
  GSize frameSize = gbitmap_sequence_get_bitmap_size(bhStanding);
  borisBitmap = gbitmap_create_blank(frameSize, GBitmapFormat8Bit);

  // Create BitmapLayer to display the GBitmap
  borisLayer = bitmap_layer_create(GRect(settings.borisX, settings.borisY, settings.borisSize, settings.borisSize));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_compositing_mode(borisLayer, GCompOpSet);
  bitmap_layer_set_bitmap(borisLayer, borisBitmap);
  layer_add_child(windowLayer, bitmap_layer_get_layer(borisLayer));

  // Create blank GBitmap using APNG frame size
  weatherBitmap = gbitmap_create_with_resource(RESOURCE_ID_50D);

  // Create BitmapLayer to display the GBitmap
  weatherIconLayer = bitmap_layer_create(GRect(13, 98, 32, 32));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_compositing_mode(weatherIconLayer, GCompOpSet);
  bitmap_layer_set_bitmap(weatherIconLayer, weatherBitmap);
  layer_add_child(windowLayer, bitmap_layer_get_layer(weatherIconLayer));

  // Create GFont
  timeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_48));
  weatherFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_20));

  timeLayer = text_layer_create(GRect(13, 10, bounds.size.w, 50));
  dateLayer = text_layer_create(GRect(13, 60, bounds.size.w, 50));
  timeShadowLayer = text_layer_create(GRect(13, 14, bounds.size.w, 50));
  dateShadowLayer = text_layer_create(GRect(13, 64, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(timeLayer, GColorClear);
  text_layer_set_text_color(timeLayer, GColorWhite);
  text_layer_set_font(timeLayer, timeFont);
  text_layer_set_text_alignment(timeLayer, GTextAlignmentLeft);

  text_layer_set_background_color(dateLayer, GColorClear);
  text_layer_set_text_color(dateLayer, GColorWhite);
  text_layer_set_font(dateLayer, weatherFont);
  text_layer_set_text_alignment(dateLayer, GTextAlignmentLeft);

  // Repeat for shadow layers
  text_layer_set_background_color(timeShadowLayer, GColorClear);
  text_layer_set_text_color(timeShadowLayer, GColorBlack);
  text_layer_set_font(timeShadowLayer, timeFont);
  text_layer_set_text_alignment(timeShadowLayer, GTextAlignmentLeft);

  text_layer_set_background_color(dateShadowLayer, GColorClear);
  text_layer_set_text_color(dateShadowLayer, GColorBlack);
  text_layer_set_font(dateShadowLayer, weatherFont);
  text_layer_set_text_alignment(dateShadowLayer, GTextAlignmentLeft);
  
  // Create temperature Layer
  weatherTextLayer = text_layer_create(GRect(13, 115, bounds.size.w, 25));
  weatherTextShadowLayer = text_layer_create(GRect(13, 119, bounds.size.w, 25));

  // Style the text
  text_layer_set_background_color(weatherTextLayer, GColorClear);
  text_layer_set_text_color(weatherTextLayer, GColorWhite);
  text_layer_set_text_alignment(weatherTextLayer, GTextAlignmentLeft);
  text_layer_set_text(weatherTextLayer, "Loading...");

  // Style the shadow
  text_layer_set_background_color(weatherTextShadowLayer, GColorClear);
  text_layer_set_text_color(weatherTextShadowLayer, GColorBlack);
  text_layer_set_text_alignment(weatherTextShadowLayer, GTextAlignmentLeft);
  text_layer_set_text(weatherTextShadowLayer, "Loading...");

  // Create second custom font, apply it and add to Window
  text_layer_set_font(weatherTextLayer, weatherFont);
  text_layer_set_font(weatherTextShadowLayer, weatherFont);
  
  // Create battery meter Layer
  batteryLayer = layer_create(GRect(0, 150, 144, 2));
  layer_set_update_proc(batteryLayer, batteryUpdateProc);

  // Add shadow layers
  layer_add_child(windowLayer, text_layer_get_layer(timeShadowLayer));
  layer_add_child(windowLayer, text_layer_get_layer(dateShadowLayer));
  layer_add_child(windowLayer, text_layer_get_layer(weatherTextShadowLayer));
  
  // Add front font layers
  layer_add_child(windowLayer, text_layer_get_layer(timeLayer));
  layer_add_child(windowLayer, text_layer_get_layer(dateLayer));
  layer_add_child(windowLayer, text_layer_get_layer(weatherTextLayer));

  // Add to Window
  layer_add_child(window_get_root_layer(window), batteryLayer);
}

static void mainWindowUnload(Window *window) {
  text_layer_destroy(timeLayer);
  text_layer_destroy(timeShadowLayer);
  text_layer_destroy(dateLayer);
  text_layer_destroy(dateShadowLayer);
  text_layer_destroy(weatherTextLayer);
  text_layer_destroy(weatherTextShadowLayer);
  gbitmap_destroy(borisBitmap);
  bitmap_layer_destroy(borisLayer);
  gbitmap_destroy(weatherBitmap);
  bitmap_layer_destroy(weatherIconLayer);
  fonts_unload_custom_font(weatherFont);
  fonts_unload_custom_font(timeFont);
  layer_destroy(batteryLayer);
}

static void inboxReceivedCallback(DictionaryIterator *iterator, void *context) {
  // Read tuples for data
  Tuple *tempTuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *iconTuple = dict_find(iterator, MESSAGE_KEY_ICON);
  Tuple *bgColorTuple = dict_find(iterator, MESSAGE_KEY_BackgroundColor);

  // If all data is available, use it
  if(tempTuple && iconTuple) {
    char iconBuffer[4];
    snprintf(temperatureBuffer, sizeof(temperatureBuffer), "%dC", (int)tempTuple->value->int32);
    snprintf(iconBuffer, sizeof(iconBuffer), "%s", iconTuple->value->cstring);
    text_layer_set_text(weatherTextLayer, temperatureBuffer);
    text_layer_set_text(weatherTextShadowLayer, temperatureBuffer);
    //APP_LOG(APP_LOG_LEVEL_INFO, "Temperature is: %s", blahBuffer);
    if(!strncmp(iconBuffer, "01d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon01d);
    } else if(!strncmp(iconBuffer, "01n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon01n);
    } else if(!strncmp(iconBuffer, "02d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon02d);
    } else if(!strncmp(iconBuffer, "02n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon02n);
    } else if(!strncmp(iconBuffer, "03d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon03d);
    } else if(!strncmp(iconBuffer, "03n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon03n);
    } else if(!strncmp(iconBuffer, "04d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon04d);
    } else if(!strncmp(iconBuffer, "04n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon04n);
    } else if(!strncmp(iconBuffer, "09d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon09d);
    } else if(!strncmp(iconBuffer, "09n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon09n);
    } else if(!strncmp(iconBuffer, "10d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon10d);
    } else if(!strncmp(iconBuffer, "10n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon10n);
    } else if(!strncmp(iconBuffer, "11d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon11d);
    } else if(!strncmp(iconBuffer, "11n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon11n);
    } else if(!strncmp(iconBuffer, "13d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon13d);
    } else if(!strncmp(iconBuffer, "13n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon13n);
    } else if(!strncmp(iconBuffer, "50d", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon50d);
    } else if(!strncmp(iconBuffer, "50n", 3)) {
      bitmap_layer_set_bitmap(weatherIconLayer, wIcon50n);
    }
    layer_mark_dirty(bitmap_layer_get_layer(weatherIconLayer));
  }
  if(bgColorTuple) {
    settings.bgColor = GColorFromHEX(bgColorTuple->value->int32);
    window_set_background_color(mainWindow, settings.bgColor);
    saveSettings();
  }
}

static void inboxDroppedCallback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outboxFailedCallback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outboxSentCallback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void batteryCallback(BatteryChargeState state) {
  // Record the new battery level
  batteryLevel = state.charge_percent;
  // Update meter
  layer_mark_dirty(batteryLayer);
}

static void init() {
  loadSettings();
  
  mainWindow = window_create();
  window_set_window_handlers(mainWindow, (WindowHandlers) {
    .load = mainWindowLoad,
    .unload = mainWindowUnload
  });
  window_stack_push(mainWindow, true);

  // Make sure the time is displayed from the start
  updateTime();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);
  
  window_set_background_color(mainWindow, settings.bgColor);

  // Register callbacks
  app_message_register_inbox_received(inboxReceivedCallback);
  app_message_register_inbox_dropped(inboxDroppedCallback);
  app_message_register_outbox_failed(outboxFailedCallback);
  app_message_register_outbox_sent(outboxSentCallback);
  battery_state_service_subscribe(batteryCallback);

  // Open AppMessage
  const int inboxSize = 128;
  const int outboxSize = 128;
  app_message_open(inboxSize, outboxSize);
  
  // Ensure battery level is displayed from the start
  batteryCallback(battery_state_service_peek());

  // Initialize Boris with a behaviour
  changeBehaviour(666);
}

static void deinit() {
  window_destroy(mainWindow);
  saveSettings();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}