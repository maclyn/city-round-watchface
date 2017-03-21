//Main watchface code. Formatted off the orignal "HexaTransit".
#include <pebble.h>
#define FPS 1000/10

/* TODO before release
 * Performance/power improvements
 * Centering of time
 * Addition of day of week
 * Proper time of day support
 * Color tweaking
 * Proper metadata 
 */

/* Constants region */
int WIDTH = PBL_IF_ROUND_ELSE(180, 144);
int HEIGHT = PBL_IF_ROUND_ELSE(180, 168);

static Window* window;
static Layer* layer;
static AppTimer* timer;

int is_charging = 0;
bool is_connected = true;
int battery_percent = 0;
static GRect bounds;
static GFont font;
static Layer *window_layer;

enum TimeOfDay {
  DAY, NIGHT, TWILIGHT  
};
typedef enum TimeOfDay TimeOfDay;
TimeOfDay timeOfDay;

long time_running = 0l;

/* Buffers region 
 *
 * Any value in these buffers of -1 indicates end-of-list; readers can just stop.
*/

/* Have we init'd the data buffers? */
bool buffersInited = false;

/* Sky information */
int starLocations[100][2];
int cloudLocations[4][2];

/* Skyscraper information */
int scraperStart[9][2];
bool scraperHasCap[9];
bool scraperWindowsOn[9][20][3];

/* Water information */
GColor waterColors[60][180];
int waterStart = 0; //We start drawing at start -> end
int waterLengths[60][180];

/* Traffic information */
GColor trafficColor[2][60];
int trafficWidth[2][60];
int trafficX[2][60];
int trafficStart[2]; //Start drawing L->R from start, wrapping through the buffer
int trafficEnd[2];

//TODO: Only supports NIGHT for now; ignores TOD
void populate_data_buffers(TimeOfDay timeOfDay){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Building buffers for %d", (int) timeOfDay);
  
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
  
  const GColor* trafficMedley = NULL;
  int trafficMedleyLen = 0;
  const int* trafficLengths = NULL;
  int trafficLengthsLen = 0;
  int trafficChance = 1;
  
  GColor waterPrimaryColor = GColorBlue;
  GColor waterSecondaryColor = GColorBlue;
  
  bool drawStars = false;
  bool drawClouds = false;
  bool drawSomeWindows = false;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", (int) drawSomeWindows);
  
  switch(timeOfDay){
    case NIGHT:
      waterPrimaryColor = GColorMidnightGreen;
      waterSecondaryColor = GColorOxfordBlue;
      trafficChance = 5;
      trafficMedley = TRAFFIC_MEDLEY_NIGHT;
      trafficMedleyLen = TRAFFIC_MEDLEY_NIGHT_LEN;
      trafficLengths = TRAFFIC_LENS_NIGHT;
      trafficLengthsLen = TRAFFIC_LENS_NIGHT_LEN;
      drawStars = true;
      drawSomeWindows = true;
      break;
    case TWILIGHT:
      waterPrimaryColor = GColorOrange;
      waterSecondaryColor = GColorRed;
      trafficChance = 4;
      trafficMedley = TRAFFIC_MEDLEY_TWLIGHT;
      trafficMedleyLen = TRAFFIC_MEDLEY_TWLIGHT_LEN;
      trafficLengths = TRAFFIC_LENS_DAY_OR_TWLIGHT;
      trafficLengthsLen = TRAFFIC_LENS_DAY_OR_TWLIGHT_LEN;
      break;
    case DAY:
      waterPrimaryColor = GColorCobaltBlue;
      waterSecondaryColor = GColorPictonBlue;
      trafficChance = 2;
      trafficMedley = TRAFFIC_MEDLEY_DAY;
      trafficMedleyLen = TRAFFIC_MEDLEY_DAY_LEN;
      trafficLengths = TRAFFIC_LENS_DAY_OR_TWLIGHT;
      trafficLengthsLen = TRAFFIC_LENS_DAY_OR_TWLIGHT_LEN;
      drawClouds = true;
      break;
  }
  
  if(drawStars){
    int starCount = (rand() % 25) + 25; //50-100 stars
    int starNum = 0;
    while(starCount > starNum){
      int yOrigin = (rand() % HEIGHT);
      int xOrigin = (rand() % WIDTH);
      
      starLocations[starNum][0] = xOrigin;
      starLocations[starNum][1] = yOrigin;
            
      starNum++;
    }
    
    if(starNum != 99) starLocations[starNum][0] = -1;
  }
  
  if(drawClouds){
    //Draw between small white rectangles (60x15) in the top 1/3 of the display
    int cloudCount = (rand() % 2) + 2; //2-4
    int cloudNum = 0;
    while(cloudCount > cloudNum){
      int yOrigin = (rand() % (HEIGHT / 3)); //10-60
      int xOrigin = (rand() % (WIDTH + 60)) - 60; //-60 to 180ish
      
      cloudLocations[cloudNum][0] = xOrigin;
      cloudLocations[cloudNum][1] = yOrigin;
                  
      cloudNum++;
    }
    
    if(cloudNum != 4) cloudLocations[cloudNum][0] = -1;
  }
  
  //Draw cars on road
  trafficStart[0] = 0;
  trafficStart[1] = 0;
  
  int carLine = 0;
  while(carLine < 2){
    int carXPos = -1;
    int carNum = 0;
    
    while(carXPos < WIDTH + 1){
      int carWidth = trafficLengths[rand() % trafficLengthsLen];
      GColor carColor = trafficMedley[rand() % trafficMedleyLen];
      bool happens = rand() % trafficChance == 0;
      
      if(happens){
          trafficColor[carLine][carNum] = carColor;
          trafficWidth[carLine][carNum] = carWidth;
          trafficX[carLine][carNum] = carXPos;
          carNum++;
      }
      
      carXPos += (carWidth + 2);
    }
       
    if(carNum != 60){
      trafficWidth[carLine][carNum] = -1;
    }
    trafficEnd[carLine] = carNum;
    
    carLine += 1;
  }
  
  //Draw water (liney)
  int waterLineNum = 0;
  bool primaryColor = true;
  while(waterLineNum < (HEIGHT - 125)){
    //Draw single lines up to 10 wide, alternating color
    int waterHorizNum = 0;
    int waterXPos = 0;
    
    while(waterXPos < (WIDTH + 1)){
      int waterLineWidth = (rand() % 10);
      
      waterLengths[waterLineNum][waterHorizNum] = waterLineWidth;
      waterColors[waterLineNum][waterHorizNum] = (primaryColor ? waterPrimaryColor : waterSecondaryColor);
      
      primaryColor = !primaryColor;
      waterXPos += waterLineWidth;
      waterHorizNum++;
    }
    waterLengths[waterLineNum][waterHorizNum] = -1;
    
    waterLineNum++;
  }
  
  int scraperCount = 0;
  while(scraperCount < 9){    
    int yOrigin = ((rand() % 14) * 5) + 20; //30-90ish
    int xOrigin = scraperCount * 20; //0, 20, ..., 160
    
    scraperStart[scraperCount][0] = xOrigin;
    scraperStart[scraperCount][1] = yOrigin;    
    
    if(rand() % 3 == 0){
      scraperHasCap[scraperCount] = true;
    } else {
      scraperHasCap[scraperCount] = false;
    }
  
    //Draw windows
    int windowStartY = yOrigin + 4;
    int windowY = 0;
    while(windowStartY <= 106){
      int count = 0;
      while(count < 3){
        if(!drawSomeWindows || (drawSomeWindows && (rand() % 3 == 0))){
          scraperWindowsOn[scraperCount][windowY][count] = true;
        } else {
          scraperWindowsOn[scraperCount][windowY][count] = false;
        }
        count++;
      }
      
      windowStartY += 5;
      windowY++;
    }
    
    scraperCount++;
  }
  
  buffersInited = true;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "done initing buffer");
}

static void app_timer_callback(void* data){
  layer_mark_dirty(layer);
  timer = app_timer_register(FPS, app_timer_callback, NULL);
}

static void layer_update_callback(Layer *me, GContext* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "handling layer update callback");
  
  time_t in_time_units;
  time(&in_time_units);
  struct tm* curr_time = localtime(&in_time_units);
  
  int hours = curr_time->tm_hour;
  int minutes = curr_time->tm_min;
  int seconds = curr_time->tm_sec;
    
  char time_buf[10];
  strftime(time_buf, 10, "%l:%M", curr_time);
  
  //int day_of_week = curr_time->tm_wday;
  
  if(hours < 7 || hours > 20){
    timeOfDay = NIGHT; 
  } else if (hours == 7 || (hours < 20 && hours > 17)){
    timeOfDay = TWILIGHT;
  } else {
    timeOfDay = DAY;
  }
  timeOfDay = NIGHT; //Everything else looks UGLY. Boo.
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing at: %d:%d:%d", hours, minutes, seconds);
  
  if(!buffersInited){
    populate_data_buffers(timeOfDay);
  }
  
  if(minutes == 0 && seconds == 0){
    vibes_long_pulse();
  }
  
  GColor skyColor = GColorDarkCandyAppleRed;
  GColor windowColor = GColorDarkGray;
  GColor buildingColor = GColorDarkGray;
  GColor roadColor = GColorLightGray;  
  GColor beachColor = GColorBlue;
  bool drawStars = false;
  bool drawClouds = false;
  
  switch(timeOfDay){
    case NIGHT:
      skyColor = GColorOxfordBlue;
      windowColor = GColorChromeYellow;
      buildingColor = GColorBlack;
      roadColor = GColorBlack;
      beachColor = GColorWindsorTan;
      drawStars = true;
      break;
    case TWILIGHT:
      skyColor = GColorDarkCandyAppleRed;
      windowColor = GColorBlack;
      buildingColor = GColorBlack;
      roadColor = GColorBlack;
      beachColor = GColorChromeYellow;
      break;
    case DAY:
      skyColor = GColorMediumAquamarine;
      windowColor = GColorWhite;
      buildingColor = GColorBlack;
      roadColor = GColorLightGray;
      beachColor = GColorRajah;
      drawClouds = true;
      break;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "colors %d %d", windowColor.r, buildingColor.r);
  
  //Fill order:
  //  Background color (width x height) - check
  //  Stars OR clouds (width x height for stars; in top 1/3 for clouds) - check
  //  Beach - check
  //  Road - check
  //  Cars - check
  //  Water - check
  //  Skyscrapers - check
  //    Windows - check
  //    Window reflections - check
  //  Time - check 
  //    Time reflection
  
  //Draw background color
  graphics_context_set_fill_color(ctx, skyColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 0 }, .size = { WIDTH, HEIGHT }}, 0, (GCornerMask) NULL);
  
  //Draw stars or clouds
  if(drawStars){
    graphics_context_set_fill_color(ctx, GColorWhite);
    int starNum = 0;
    while(starNum < 100){
      if(starLocations[starNum][0] == -1) break; //EOL
      
      graphics_fill_rect(ctx, (GRect) { .origin = { 
        starLocations[starNum][0], starLocations[starNum][1] 
      }, .size = { 1, 1 }}, 0, (GCornerMask) NULL);
      
      starNum++;
    }
  }
  
  if(drawClouds){
    //Draw between small white rectangles (60x15) in the top 1/3 of the display
    graphics_context_set_fill_color(ctx, GColorWhite);
    int cloudNum = 0;
    while(cloudNum < 100){
      if(cloudLocations[cloudNum][0] == -1) break; //EOL;
      
      graphics_fill_rect(ctx, (GRect) { .origin = { 
        cloudLocations[cloudNum][0], cloudLocations[cloudNum][1]
      }, .size = { 60, 15 }}, 0, (GCornerMask) NULL);
            
      cloudNum++;
    }
  }
  
  //Draw the beach (just a blob for now)
  graphics_context_set_fill_color(ctx, beachColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 120 }, .size = { WIDTH, 5 }}, 0, (GCornerMask) NULL);
  
  //Draw road (light gray)
  graphics_context_set_fill_color(ctx, roadColor);
  graphics_fill_rect(ctx, (GRect) { .origin = { 0, 110 }, .size = { WIDTH, 10 }}, 0, (GCornerMask) NULL);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done drawing up to cars");
  
  //We find where end of list
  int carScanline = 112;
  int carLine = 0;
  while(carScanline < 120){
    int carNum = 0;
    int endCar = trafficEnd[carLine];
    
    trafficStart[carLine] += (rand() % 3); //Move 0-2 pixels/second
    trafficStart[carLine] = (trafficStart[carLine] % WIDTH); //Can't start too far out...
    
    while(carNum < endCar){        
      graphics_context_set_fill_color(ctx, 
        trafficColor[carLine][carNum]
      );
      graphics_fill_rect(ctx, (GRect) { .origin = { 
        (trafficX[carLine][carNum] + trafficStart[carLine]) % WIDTH, carScanline
      }, .size = {
        trafficWidth[carLine][carNum], 2 
      }}, 0, (GCornerMask) NULL);
      
      carNum++;
    }
        
    carScanline += 4;
    carLine += 1;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done drawing up to water");
     
  int waterLineNum = 0;
  int waterEnd = HEIGHT - 125;
  while(waterLineNum < waterEnd){
    //Draw single lines up to 10 wide, alternating color
    int waterHorizNum = 0;
    int waterXPos = 0;
    
    while(waterXPos < (WIDTH + 1)){
      int waterLineWidth = waterLengths[(waterLineNum + waterStart) % waterEnd][waterHorizNum];
      if(waterLineWidth == -1) break;
      
      GColor waterColor = waterColors[(waterLineNum + waterStart) % waterEnd][waterHorizNum];
      
      graphics_context_set_fill_color(ctx, waterColor);
      graphics_fill_rect(ctx, (GRect) { .origin = { 
        waterXPos, waterLineNum + 125 
      }, .size = { 
        waterLineWidth, 1 
      }}, 0, (GCornerMask) NULL);
      
      waterXPos += waterLineWidth;
      waterHorizNum++;
    }    
    waterLineNum++;
  }  
  
  waterStart += 1;
  waterStart = waterStart % waterEnd;
    
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done drawing up to scrapers");

  //Draw "skyscrapers" w/ lights and windows
  int scraperCount = 0;
  while(scraperCount < 9){
    graphics_context_set_fill_color(ctx, buildingColor);
    
    int yOrigin = scraperStart[scraperCount][1]; //30-90ish
    int xOrigin = scraperStart[scraperCount][0]; //0, 20, ..., 160
    int height = 110 - yOrigin;
    
    graphics_fill_rect(ctx, (GRect) { .origin = { xOrigin, yOrigin }, .size = { 20, height }}, 0, (GCornerMask) NULL);
    
    if(scraperHasCap[scraperCount]){
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
    int windowY = 0;
    while(windowStartY <= 106){
      int count = 0;
      int windowStartX = xOrigin + 4;
      while(count < 3){
        if(scraperWindowsOn[scraperCount][windowY][count]){
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
        
        if(rand() % 1000 == 0) scraperWindowsOn[scraperCount][windowY][count] = !scraperWindowsOn[scraperCount][windowY][count];
        count++;
      }
      
      windowY++;
      windowStartY += 5;
    }
    
    scraperCount++;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing scrapers done! Performing fbuffer effects...");
  
  //Draw the time
  GSize expectedSize = graphics_text_layout_get_content_size(time_buf, font, (GRect) { .origin = { 0, 0 }, .size = { 200, 100 }}, GTextOverflowModeFill, GTextAlignmentCenter);
  int expectedWidth = expectedSize.w;
  int expectedHeight = expectedSize.h;
  
  graphics_context_set_text_color(ctx, GColorWhite);
  int timeX = 90 - (expectedWidth / 2);
  int timeY = 122 - 38;
  graphics_draw_text(ctx, time_buf, font, (GRect) { .origin = { timeX, timeY }, .size = { expectedWidth, expectedHeight }} , GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    
  APP_LOG(APP_LOG_LEVEL_DEBUG, "at %d, %d and %d x %d", timeX, timeY, expectedWidth, expectedHeight);
  
  //Snoop out where we *expect* the time to be and mirror it in the water
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  int x, y;
  for(y = timeY; y < timeY + expectedHeight; y++){
    int mirrorYPosition = 125 + (125 - y);
    if(mirrorYPosition > HEIGHT) continue;    
    
    GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
    if(timeX > info.max_x || info.min_x > (timeX + expectedWidth)) continue; //Ignore this row
    GBitmapDataRowInfo mirrorInfo = gbitmap_get_data_row_info(fb, mirrorYPosition);
    
    int maxX = info.max_x;
    if(timeY + expectedWidth < maxX) maxX = timeY + expectedWidth;
    
    int xOffset = (rand() % 3) + -1;
    for(x = timeX; x < maxX; x++){            
      int newX = xOffset + x;
      
      if(mirrorInfo.min_x > newX || mirrorInfo.max_x < newX) continue; //Not important
      
      int colorAtPoint = info.data[x];
      if(colorAtPoint == GColorWhiteARGB8){
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Found point to mirror at %d %d", y, newX);
        
        //Modify framebuffer to contain white flipped over axis
        memset(&mirrorInfo.data[newX], colorAtPoint, 1);
      }
    }
  }
  graphics_release_frame_buffer(ctx, fb);
  
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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Handling battery change");
  
  is_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Doe handling battery change");
}

void init(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Init'ing");
  
  window = window_create();
  window_stack_push(window, true);

  // Init the layer for display the image
  window_layer = window_get_root_layer(window);
  bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_update_callback);
  layer_add_child(window_layer, layer);
  
  font = fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init'd windows + all resources");
  
  srand(time(NULL));
  
  timer = app_timer_register(FPS, app_timer_callback, NULL);
  
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
    
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  
  window_destroy(window);
  layer_destroy(layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}