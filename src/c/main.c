#include <pebble.h>

// Persistent storage keys
#define PERSIST_CONFIG_COUNT_KEY 100
#define PERSIST_CONFIG_BASE_KEY 101
#define PERSIST_CONFIG_MAX_CHUNKS 16
#define PERSIST_CHUNK_SIZE 256
#define PERSIST_NAME_BASE_KEY 200

static Window *s_window;
static TextLayer *s_title_layer;
static TextLayer *s_up_layer;
static TextLayer *s_select_layer;
static TextLayer *s_down_layer;
static TextLayer *s_status_layer;

static char s_up_text[64] = "UP1: (not set)";
static char s_select_text[64] = "OK: (not set)";
static char s_down_text[64] = "DN: (not set)";
static char s_status_text[64] = "";

static void save_config_to_persist(const char *json)
{
  int len = strlen(json) + 1; // include null terminator
  int chunks = (len + PERSIST_CHUNK_SIZE - 1) / PERSIST_CHUNK_SIZE;
  if (chunks > PERSIST_CONFIG_MAX_CHUNKS)
    chunks = PERSIST_CONFIG_MAX_CHUNKS;
  persist_write_int(PERSIST_CONFIG_COUNT_KEY, chunks);
  for (int i = 0; i < chunks; i++)
  {
    int offset = i * PERSIST_CHUNK_SIZE;
    int chunk_len = len - offset;
    if (chunk_len > PERSIST_CHUNK_SIZE)
      chunk_len = PERSIST_CHUNK_SIZE;
    persist_write_data(PERSIST_CONFIG_BASE_KEY + i, json + offset, chunk_len);
  }
}

static bool load_config_from_persist(char *buf, int buf_size)
{
  if (!persist_exists(PERSIST_CONFIG_COUNT_KEY))
    return false;
  int chunks = persist_read_int(PERSIST_CONFIG_COUNT_KEY);
  if (chunks <= 0 || chunks > PERSIST_CONFIG_MAX_CHUNKS)
    return false;
  int total = 0;
  for (int i = 0; i < chunks; i++)
  {
    int remaining = buf_size - total - 1;
    if (remaining <= 0)
      break;
    int read = persist_read_data(PERSIST_CONFIG_BASE_KEY + i, buf + total, remaining);
    if (read < 0)
      break;
    total += read;
  }
  buf[total] = '\0';
  return total > 0;
}

static void send_button_press(int index)
{
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK)
    return;

  dict_write_int32(iter, MESSAGE_KEY_ButtonIndex, index);
  app_message_outbox_send();

  snprintf(s_status_text, sizeof(s_status_text), "Sending...");
  text_layer_set_text(s_status_layer, s_status_text);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  send_button_press(0);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  send_button_press(1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  send_button_press(2);
}

static void click_config_provider(void *context)
{
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void update_request_name(int index, const char *name)
{
  char *buf;
  TextLayer *layer;
  const char *prefix;

  switch (index)
  {
  case 0:
    buf = s_up_text;
    layer = s_up_layer;
    prefix = "UP: ";
    break;
  case 1:
    buf = s_select_text;
    layer = s_select_layer;
    prefix = "OK: ";
    break;
  case 2:
    buf = s_down_text;
    layer = s_down_layer;
    prefix = "DN: ";
    break;
  default:
    return;
  }

  if (name && strlen(name) > 0)
  {
    snprintf(buf, 64, "%s%s", prefix, name);
  }
  else
  {
    snprintf(buf, 64, "%s(not set)", prefix);
  }
  text_layer_set_text(layer, buf);
}

static void load_persisted_names(void)
{
  const char *prefixes[] = {"UP: ", "OK: ", "DN: "};
  char *bufs[] = {s_up_text, s_select_text, s_down_text};
  for (int i = 0; i < 3; i++)
  {
    if (persist_exists(PERSIST_NAME_BASE_KEY + i))
    {
      char name[64];
      persist_read_string(PERSIST_NAME_BASE_KEY + i, name, sizeof(name));
      if (strlen(name) > 0)
      {
        snprintf(bufs[i], 64, "%s%s", prefixes[i], name);
      }
    }
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context)
{
  // Handle request names from phone — also persist them on the watch
  Tuple *name0 = dict_find(iter, MESSAGE_KEY_RequestName0);
  if (name0)
  {
    update_request_name(0, name0->value->cstring);
    persist_write_string(PERSIST_NAME_BASE_KEY + 0, name0->value->cstring);
  }

  Tuple *name1 = dict_find(iter, MESSAGE_KEY_RequestName1);
  if (name1)
  {
    update_request_name(1, name1->value->cstring);
    persist_write_string(PERSIST_NAME_BASE_KEY + 1, name1->value->cstring);
  }

  Tuple *name2 = dict_find(iter, MESSAGE_KEY_RequestName2);
  if (name2)
  {
    update_request_name(2, name2->value->cstring);
    persist_write_string(PERSIST_NAME_BASE_KEY + 2, name2->value->cstring);
  }

  // Handle config backup from phone
  Tuple *config_data = dict_find(iter, MESSAGE_KEY_ConfigData);
  if (config_data)
  {
    save_config_to_persist(config_data->value->cstring);
  }

  // Handle backup restore request from phone
  Tuple *request_backup = dict_find(iter, MESSAGE_KEY_RequestBackup);
  if (request_backup && request_backup->value->int32)
  {
    static char config_buf[PERSIST_CHUNK_SIZE * PERSIST_CONFIG_MAX_CHUNKS];
    if (load_config_from_persist(config_buf, sizeof(config_buf)))
    {
      DictionaryIterator *out;
      if (app_message_outbox_begin(&out) == APP_MSG_OK)
      {
        dict_write_cstring(out, MESSAGE_KEY_ConfigData, config_buf);
        app_message_outbox_send();
      }
    }
  }

  // Handle response status
  Tuple *success = dict_find(iter, MESSAGE_KEY_ResponseSuccess);
  Tuple *status = dict_find(iter, MESSAGE_KEY_ResponseStatus);
  if (success && status)
  {
    if (success->value->int32)
    {
      snprintf(s_status_text, sizeof(s_status_text), "OK (%d)", (int)status->value->int32);
      vibes_short_pulse();
    }
    else
    {
      snprintf(s_status_text, sizeof(s_status_text), "Failed (%d)", (int)status->value->int32);
      vibes_double_pulse();
    }
    text_layer_set_text(s_status_layer, s_status_text);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context)
{
  snprintf(s_status_text, sizeof(s_status_text), "Msg dropped");
  text_layer_set_text(s_status_layer, s_status_text);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context)
{
  snprintf(s_status_text, sizeof(s_status_text), "Send failed");
  text_layer_set_text(s_status_layer, s_status_text);
}

static TextLayer *create_text_layer(GRect bounds, GFont font, GTextAlignment align)
{
  TextLayer *layer = text_layer_create(bounds);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, align);
  return layer;
}

static void window_load(Window *window)
{
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int w = bounds.size.w;

  GFont title_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  GFont body_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);
  GFont status_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);

  int y_start = PBL_IF_ROUND_ELSE(16, 4);
  int row_h = 42;
  int pad = PBL_IF_ROUND_ELSE(44, 24);

  // Title
  s_title_layer = create_text_layer(GRect(0, y_start, w, 36), title_font, GTextAlignmentCenter);
  text_layer_set_text(s_title_layer, "fetchy");
  layer_add_child(root, text_layer_get_layer(s_title_layer));

  // Request rows
  int row_y = y_start + 44;
  s_up_layer = create_text_layer(GRect(pad, row_y, w - pad * 2, row_h), body_font, GTextAlignmentLeft);
  text_layer_set_text(s_up_layer, s_up_text);
  layer_add_child(root, text_layer_get_layer(s_up_layer));

  s_select_layer = create_text_layer(GRect(pad, row_y + row_h, w - pad * 2, row_h), body_font, GTextAlignmentLeft);
  text_layer_set_text(s_select_layer, s_select_text);
  layer_add_child(root, text_layer_get_layer(s_select_layer));

  s_down_layer = create_text_layer(GRect(pad, row_y + row_h * 2, w - pad * 2, row_h), body_font, GTextAlignmentLeft);
  text_layer_set_text(s_down_layer, s_down_text);
  layer_add_child(root, text_layer_get_layer(s_down_layer));

  // Status bar
  s_status_layer = create_text_layer(GRect(pad, row_y + row_h * 3 + 4, w - pad * 2, 26), status_font, GTextAlignmentCenter);
  text_layer_set_text(s_status_layer, s_status_text);
  layer_add_child(root, text_layer_get_layer(s_status_layer));
}

static void window_unload(Window *window)
{
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_up_layer);
  text_layer_destroy(s_select_layer);
  text_layer_destroy(s_down_layer);
  text_layer_destroy(s_status_layer);
}

static void init(void)
{
  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(4096, 4096);

  // Load persisted request names so they show immediately
  load_persisted_names();

  // Create window
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, true);
}

static void deinit(void)
{
  window_destroy(s_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
