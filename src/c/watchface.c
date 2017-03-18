//Main watchface code. Formatted off the orignal "HexaTransit".
#include <pebble.h>

static Window *window;
static Layer *layer;

int is_charging = 0;
bool is_connected = true;
int battery_percent = 0;
static GRect bounds;
static GBitmap *numbers[16];
static GBitmap *dates[7];
static GFont font;
static Layer *window_layer;

enum TimeOfDay {
  DAY, NIGHT, TWILIGHT  
};
typedef enum TimeOfDay TimeOfDay;
TimeOfDay timeOfDay;

long time_running = 0l;

static void layer_update_callback(Layer *me, GContext* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handling layer update callback");
  
  int TRAFFIC_LENS_NIGHT[] = { 1, 1, 2, 1, 1};
  int TRAFFIC_LENS_NIGHT_LEN = 5;
  int TRAFFIC_LENS_DAY_OR_TWLIGHT[] = { 2, 2, 3, 1, 2, 1 };
  int TRAFFIC_LENS_DAY_OR_TWLIGHT_LEN = 6;
  GColor TRAFFIC_MEDLEY_NIGHT[] = { GColorRed, GColorYellow };
  int TRAFFIC_MEDLEY_NIGHT_LEN = 2;
  GColor TRAFFIC_MEDLEY_TWLIGHT[] = { GColorBlack };
  int TRAFFIC_MEDLEY_TWLIGHT_LEN  = 1;
  GColor TRAFFIC_MEDLEY_DAY[] = { GColorBlue, GColorGreen, GColorRed, GColorRajah };
  int TRAFFIC_MEDLEY_DAY_LEN = 4;
  
  time_t in_time_units;
  time(&in_time_units);
  struct tm* curr_time = localtime(&in_time_units);
  
  int hours = curr_time->tm_hour;
  int minutes = curr_time->tm_min;
  int seconds = curr_time->tm_sec;
    
  char time_buf[10];
  strftime(time_buf, 10, "%l:%M", curr_time);
  
  int day_of_week = curr_time->tm_wday;
  
  if(hours < 7 || hours > 20){
    timeOfDay = NIGHT; 
  } else if (hours == 7 || (hours < 20 && hours > 17)){
    timeOfDay = TWILIGHT;
  } else {
    timeOfDay = DAY;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "time is : %d:%d:%d", hours, minutes, seconds);
  
  if(minutes == 0 && seconds == 0){
    vibes_long_pulse();
  }
  
  GColor skyColor = GColorDarkCandyAppleRed;
  GColor windowColor = GColorDarkGray;
  GColor buildingColor = GColorDarkGray;
  GColor roadColor = GColorLightGray;  
  GColor waterPrimaryColor = GColorBlue;
  GColor waterSecondaryColor = GColorBlue;
  GColor beachColor = GColorBlue;
  const GColor* trafficMedley = NULL;
  int trafficMedleyLen = 0;
  const int* trafficLengths = NULL;
  int trafficLengthsLen = 0;
  int trafficChance = 1;
  bool drawStars = false;
  bool drawClouds = false;
  bool drawSomeWindows = false;
  
  switch(timeOfDay){
    case NIGHT:
      skyColor = GColorOxfordBlue;
      windowColor = GColorChromeYellow;
      buildingColor = GColorBlack;
      roadColor = GColorBlack;
      waterPrimaryColor = GColorMidnightGreen;
      waterSecondaryColor = GColorOxfordBlue;
      beachColor = GColorWindsorTan;
      trafficChance = 5;
      trafficMedley = TRAFFIC_MEDLEY_NIGHT;
      trafficMedleyLen = TRAFFIC_MEDLEY_NIGHT_LEN;
      trafficLengths = TRAFFIC_LENS_NIGHT;
      trafficLengthsLen = TRAFFIC_LENS_NIGHT_LEN;
      drawStars = true;
      drawSomeWindows = true;
      break;
    case TWILIGHT:
      skyColor = GColorDarkCandyAppleRed;
      windowColor = GColorBlack;
      buildingColor = GColorBlack;
      roadColor = GColorBlack;
      waterPrimaryColor = GColorOrange;
      waterSecondaryColor = GColorRed;
      beachColor = GColorChromeYellow;
      trafficChance = 4;
      trafficMedley = TRAFFIC_MEDLEY_TWLIGHT;
      trafficMedleyLen = TRAFFIC_MEDLEY_TWLIGHT_LEN;
      trafficLengths = TRAFFIC_LENS_DAY_OR_TWLIGHT;
      trafficLengthsLen = TRAFFIC_LENS_DAY_OR_TWLIGHT_LEN;
      break;
    case DAY:
      skyColor = GColorMediumAquamarine;
      windowColor = GColorWhite;
      buildingColor = GColorBlack;
      roadColor = GColorLightGray;
      waterPrimaryColor = GColorCobaltBlue;
      waterSecondaryColor = GColorPictonBlue;
      beachColor = GColorRajah;
      trafficChance = 2;
      trafficMedley = TRAFFIC_MEDLEY_DAY;
      trafficMedleyLen = TRAFFIC_MEDLEY_DAY_LEN;
      trafficLengths = TRAFFIC_LENS_DAY_OR_TWLIGHT;
      trafficLengthsLen = TRAFFIC_LENS_DAY_OR_TWLIGHT_LEN;
      drawClouds = true;
      break;
  }
  
  //Draw background color
  graphics_context_set_fill_color(ctx, skyColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 0 }, .size = { 180, 180 }}, 0, (GCornerMask) NULL);
  
  //Draw stars or clouds
  if(drawStars){
    int starCount = (rand() % 25) + 25; //50-100
    graphics_context_set_fill_color(ctx, GColorWhite);
    while(starCount > 0){
      int yOrigin = (rand() % 180);
      int xOrigin = (rand() % 180);
      
      graphics_fill_rect(ctx, (GRect) { .origin = { xOrigin, yOrigin }, .size = { 1, 1 }}, 0, (GCornerMask) NULL);
      
      starCount--;
    }
  }
  
  if(drawClouds){
    //Draw between small white rectangles (60x15) in the top 1/3 of the display
    int cloudCount = (rand() % 2) + 2; //2-4
    graphics_context_set_fill_color(ctx, GColorWhite);
    while(cloudCount > 0){
      int yOrigin = (rand() % 51) + 10; //10-60
      int xOrigin = (rand() % 240) - 60; //-60 to 180ish
      
      graphics_fill_rect(ctx, (GRect) { .origin = { xOrigin, yOrigin }, .size = { 60, 15 }}, 0, (GCornerMask) NULL);
            
      cloudCount--;
    }
  }
  
  //Draw road (light gray)
  graphics_context_set_fill_color(ctx, roadColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 110 }, .size = { 180, 10 }}, 0, (GCornerMask) NULL);
  
  //Draw cars on road
  //Basically, we do this in "scanlines" like we do the water
  int carScanline = 112;
  while(carScanline < 120){
    int carXPos = -1;
    while(carXPos < 181){
      int carWidth = trafficLengths[rand() % trafficLengthsLen];
      GColor carColor = trafficMedley[rand() % trafficMedleyLen];
      bool happens = rand() % trafficChance == 0;
      
      if(happens){
          graphics_context_set_fill_color(ctx, carColor);
          graphics_fill_rect(ctx, (GRect) { .origin = { carXPos, carScanline }, .size = { carWidth, 2 }}, 0, (GCornerMask) NULL);
      }
      
      carXPos += (carWidth + 2);
    }
    
    carScanline += 4;
  }
  
  //Draw the beach (just a blob for now)
  graphics_context_set_fill_color(ctx, beachColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 120 }, .size = { 180, 5 }}, 0, (GCornerMask) NULL);
  
  //Draw water (liney)
  int waterScanline = 126;
  bool primaryColor = true;
  while(waterScanline < 181){
    //Draw single lines up to 10 wide, alternating color
    int waterXPos = 0;
    while(waterXPos < 181){
      int waterLineWidth = (rand() % 10);
      if(primaryColor)
        graphics_context_set_fill_color(ctx, waterPrimaryColor);
      else
        graphics_context_set_fill_color(ctx, waterSecondaryColor);
      
      graphics_fill_rect(ctx, (GRect) { .origin = { waterXPos, waterScanline }, .size = { waterLineWidth, 1 }}, 0, (GCornerMask) NULL);
      primaryColor = !primaryColor;
      waterXPos += waterLineWidth;
    }
    waterScanline++;
  }
  
  //Draw "skyscrapers" w/ lights and windows
  int scraperCount = 0;
  while(scraperCount < 9){
    graphics_context_set_fill_color(ctx, buildingColor);
    
    int yOrigin = (rand() % 70) + 20; //30-90ish
    int xOrigin = scraperCount * 20; //0, 20, ..., 160
    int height = 110 - yOrigin;
    
    graphics_fill_rect(ctx, (GRect) { .origin = { xOrigin, yOrigin }, .size = { 20, height }}, 0, (GCornerMask) NULL);
    
    if(rand() % 3 == 0){
      //Draw a "cap" from startX to 20 above (decomposes to rects)
      int capPos = 1;
      while(capPos < 10){
        graphics_fill_rect(ctx, (GRect) { .origin = { xOrigin + capPos, yOrigin - capPos }, .size = { 20 - (capPos * 2), 1 }}, 0, (GCornerMask) NULL);
        capPos++;
      }
    }
  
    //Draw windows
    graphics_context_set_fill_color(ctx, windowColor);
    int windowStartY = yOrigin + 4;
    while(windowStartY <= 106){
      int count = 0;
      int windowStartX = xOrigin + 4;
      while(count < 3){
        if(!drawSomeWindows || (drawSomeWindows && (rand() % 3 == 0))){
          graphics_fill_rect(ctx, (GRect) { .origin = { windowStartX, windowStartY }, .size = { 2, 2 }}, 0, (GCornerMask) NULL);
          windowStartX += 5;
          
          //Draw reflection in water!
          if(windowStartY < 90 && timeOfDay == NIGHT){ //Will it be reflected?
            //Reflect over y=110 axis
            int expectedWaterPos = 110 + (110 - windowStartY) + 1;
            //Draw as a 2x1 colored splotch
            graphics_context_set_fill_color(ctx, windowColor);
            int newXPos = windowStartX - 6 + (rand() % 3);
            graphics_fill_rect(ctx, (GRect) { .origin = { newXPos, expectedWaterPos }, .size = { (rand() % 1 == 0 ? 3 : 5), 1 }}, 0, (GCornerMask) NULL);
          }
        }
        count++;
      }
      
      windowStartY += 5;
    }
    
    scraperCount++;
  }
  
  //Draw the time
  
  
    /*
  float hourProgress = hours / 23.0f;
  float hourAddtlXWidth = (hourProgress * 62);
  
  float minuteProgress = minutes / 60.0f;
  float minuteAddtlXWidth = (minuteProgress * 62);
  
  //Draw the "outline"
  graphics_context_set_fill_color(ctx, GColorBlack);
  //Hour outline
  graphics_fill_rect(ctx, (GRect) { .origin = { 91 - 20 - 4 - 4 + hourAddtlXWidth, 50 }, .size = { 28, 40 }}, 2, GCornersAll);  
  //Minute outline
  graphics_fill_rect(ctx, (GRect) { .origin = { 91 - 20 - 4 - 4 + (minutes > 16 ? -22 : 0) + minuteAddtlXWidth, 90 }, .size = { 28 + (minutes > 16 ? 22 : 0), 40 }}, 2, GCornersAll);  
  
  //Draw the hour in hex; always guaranteed a single numeral
  int hourToDraw = hours > 12 ? hours - 12 : hours;
  graphics_draw_bitmap_in_rect(ctx, numbers[hourToDraw], (GRect) { .origin = { 91 - 24 + hourAddtlXWidth, 54 }, .size = { 20, 32 }});
  
  //Draw the minutes in hex
  int first_hex_digit = minutes / 16;
  int second_hex_digit = minutes % 16;
  if(minutes > 16){
    //Draw the first
    graphics_draw_bitmap_in_rect(ctx, numbers[first_hex_digit], (GRect) { .origin = { 91 - 20 - 2 - 20 - 4 + minuteAddtlXWidth, 94 }, .size = { 20, 32 }});
  }
  //Draw the second
  graphics_draw_bitmap_in_rect(ctx, numbers[second_hex_digit], (GRect) { .origin = { 91 - 20 - 4 + minuteAddtlXWidth, 94 }, .size = { 20, 32 }});
  */
  GSize expectedSize = graphics_text_layout_get_content_size(time_buf, font, (GRect) { .origin = { 90, 90 }, .size = { 200, 100 }}, GTextOverflowModeFill, GTextAlignmentCenter);
  int expectedWidth = expectedSize.w;
  int expectedHeight = expectedSize.h;
  
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, (GRect) { .origin = { 88 - (expectedWidth / 2), 28 }, .size = { expectedWidth + 4, expectedHeight + 4 }}, 2, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, (GRect) { .origin = { 90 - (expectedWidth / 2), 30 }, .size = { expectedWidth, expectedHeight }}, 2, GCornersAll);
  graphics_draw_text(ctx, time_buf, font, (GRect) { .origin = { 90 - (expectedWidth / 2), 30 - 5 }, .size = { expectedWidth, expectedHeight }} , GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  
  //Draw day of week (0-6)
  graphics_draw_bitmap_in_rect(ctx, dates[day_of_week], (GRect) { .origin = { 75, 150 }, .size = { 30, 20 } }); 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done handling layer update");
}

static void handle_bluetooth(bool connected) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handling bt change");
  
  is_connected = connected;
  if(time_running > 5)
    vibes_long_pulse();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done handling bt change");
}

static void handle_battery(BatteryChargeState charge_state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handling battery change");
  
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done handling battery change");
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handling tick");

  time_running++;
  if(tick_time->tm_sec == 0)
    layer_mark_dirty(layer);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done handling tick");
}

void init(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "initing");
  
  window = window_create();
  window_stack_push(window, true);

  // Init the layer for display the image
  window_layer = window_get_root_layer(window);
  bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_update_callback);
  layer_add_child(window_layer, layer);
  numbers[0] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_0_BLACK);
  numbers[1] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_1_BLACK);
  numbers[2] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_2_BLACK);
  numbers[3] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_3_BLACK);
  numbers[4] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_4_BLACK);
  numbers[5] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_5_BLACK);
  numbers[6] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_6_BLACK);
  numbers[7] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_7_BLACK);
  numbers[8] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_8_BLACK);
  numbers[9] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_9_BLACK);
  numbers[10] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_A_BLACK);
  numbers[11] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_B_BLACK);
  numbers[12] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_C_BLACK);
  numbers[13] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_D_BLACK);
  numbers[14] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_E_BLACK);
  numbers[15] = gbitmap_create_with_resource(RESOURCE_ID_NUMBER_F_BLACK);
  
  dates[0] = gbitmap_create_with_resource(RESOURCE_ID_DATE_SU);
  dates[1] = gbitmap_create_with_resource(RESOURCE_ID_DATE_MO);
  dates[2] = gbitmap_create_with_resource(RESOURCE_ID_DATE_TU);
  dates[3] = gbitmap_create_with_resource(RESOURCE_ID_DATE_WE);
  dates[4] = gbitmap_create_with_resource(RESOURCE_ID_DATE_TH);
  dates[5] = gbitmap_create_with_resource(RESOURCE_ID_DATE_FR);
  dates[6] = gbitmap_create_with_resource(RESOURCE_ID_DATE_SA);
  
  font = fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init'd windows + all resources");
  
  srand(time(NULL));
  
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  handle_battery(battery_state_service_peek());
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init'd callbacks");
  
  layer_mark_dirty(layer);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done init'ing");
}

void deinit(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deiniting");
    
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  
  int i;
  for(i = 0; i < 16; i++){
    gbitmap_destroy(numbers[i]);
  }
  for(i = 0; i < 7; i++){
    gbitmap_destroy(dates[i]);
  }
  
  window_destroy(window);
  layer_destroy(layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}