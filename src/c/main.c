#include <pebble.h>

static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;

static GFont s_time_font;

// #define foreground (GColorBlack)
// #define background GColorClear)
// static GColor foreground;
// static GColor background;
// foreground = (GColor)GColorBlack;
// background = (GColor)GColorClear;

// for Bluetooth Connection
static void bluetooth_callback(bool connected) {
  static char connection_text[] = " ";
  if (connected) {
    snprintf(connection_text, sizeof(connection_text), " ");
  } else {
    snprintf(connection_text, sizeof(connection_text), "x");
    // Issue a vibrating alert
    vibes_double_pulse();
  }
  text_layer_set_text(s_connection_layer, connection_text);
}


// for Battery
static void battery_callback(BatteryChargeState charge_state) {
  static char battery_text[] = "charging";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}


// for weather
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dc", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
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


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_buffer);
  
  static char s_buffer_date[] = "Jan 01 Wed";
  strftime(s_buffer_date, sizeof(s_buffer_date), "%b %d %a", tick_time);
  text_layer_set_text(s_date_layer, s_buffer_date);
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
  window_set_background_color(window, GColorBlack);
  
  // Date
  s_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  s_date_layer = text_layer_create(GRect(0, 30, bounds.size.w, 24));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_time_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  // Time
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Weather
  s_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_weather_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 28));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_font(s_weather_layer, s_time_font);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  
  // Battery
  s_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_battery_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(140, 120), bounds.size.w, 28));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_font(s_battery_layer, s_time_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  
  static char battery_text[] = "charging";
  BatteryChargeState charge_state = battery_state_service_peek();
    if (!charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
  
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  // Connection
  s_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_connection_layer = text_layer_create(GRect(20, PBL_IF_ROUND_ELSE(140, 140), bounds.size.w, 28));
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_text_color(s_connection_layer, GColorWhite);
  text_layer_set_font(s_connection_layer, s_time_font);
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentCenter);
  static char connection_text[] = "-";
  if (bluetooth_connection_service_peek()) {
    snprintf(connection_text, sizeof(connection_text), " ");
  } else {
    snprintf(connection_text, sizeof(connection_text), "x");
  }
  text_layer_set_text(s_connection_layer, connection_text);
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));

  
  
  update_time();
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_time_font);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_battery_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);  
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}