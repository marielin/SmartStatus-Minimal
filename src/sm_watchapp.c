#include <pebble.h>
#include "globals.h"


#define STRING_LENGTH 255
#define NUM_WEATHER_IMAGES	9
#define VIBE_ON_HOUR true


	// Mes variables
static bool Watch_Face_Initialized = false;
static bool Precision_Is_Seconds = false;
static bool music_is_playing = false;
	
	
enum {CALENDAR_LAYER, MUSIC_LAYER, NUM_LAYERS};

static void reset();

static Window *window;
//static TextLayer *text_layer;

static PropertyAnimation *ani_out, *ani_in;

static Layer *animated_layer[NUM_LAYERS], *weather_layer;
static Layer *battery_layer, *battery_pbl_layer;

static TextLayer *text_date_layer, *text_time_layer;

static TextLayer *text_weather_cond_layer, *text_weather_temp_layer, *text_battery_layer;
static TextLayer *calendar_date_layer, *calendar_text_layer;
static TextLayer *music_artist_layer, *music_song_layer;
 
static BitmapLayer *background_image, *weather_image, *battery_image_layer, *battery_pbl_image_layer;

static int active_layer;

static char string_buffer[STRING_LENGTH];
static char weather_cond_str[STRING_LENGTH], weather_temp_str[5];
static int weather_img, batteryPercent, batteryPblPercent;

static char calendar_date_str[STRING_LENGTH], calendar_text_str[STRING_LENGTH];
static char music_artist_str1[STRING_LENGTH], music_title_str1[STRING_LENGTH];


GBitmap *bg_image, *battery_image, *battery_pbl_image;
GBitmap *weather_status_imgs[NUM_WEATHER_IMAGES];

static AppTimer *timerUpdateCalendar = NULL;
static AppTimer *timerUpdateWeather = NULL;
static AppTimer *timerUpdateMusic = NULL;



const int WEATHER_IMG_IDS[] = {	
  RESOURCE_ID_IMAGE_SUN,
  RESOURCE_ID_IMAGE_RAIN,
  RESOURCE_ID_IMAGE_CLOUD,
  RESOURCE_ID_IMAGE_SUN_CLOUD,
  RESOURCE_ID_IMAGE_FOG,
  RESOURCE_ID_IMAGE_WIND,
  RESOURCE_ID_IMAGE_SNOW,
  RESOURCE_ID_IMAGE_THUNDER,
  RESOURCE_ID_IMAGE_DISCONNECT
};

// Lists of days and months
          // Translation for DAYS goes here, starting on SUNDAY :
const char *day_of_week[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

          // Translation for MONTHS goes here :
const char *month_of_year[] = { "Janv", "Fevr", "Mars", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"};   



static uint32_t s_sequence_number = 0xFFFFFFFE;
/** —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
—————————————————————————————————————————————————————Calendar Appointments———————————————————————————————————————————————————————————————————
**/

/* Convert letter to digit */
int letter2digit(char letter) {
	if (letter == '\0') {
		APP_LOG(APP_LOG_LEVEL_ERROR, "letter2digit failed!");
		return -1;
	}
	if((letter >= 48) && (letter <=57)) {
		return letter - 48;
	}
	APP_LOG(APP_LOG_LEVEL_ERROR, "letter2digit(%c) failed", letter);
	return -1;
}

/* Convert string to number */
static int string2number(char *string) {
	int result = 0;
	static int32_t offset;
		offset = strlen(string) - 1;
	static int32_t digit = -1;
	static int32_t unit = 1;
	static int8_t letter;	

	for(unit = 1; offset >= 0; unit = unit * 10) {
		letter = string[offset];
		digit = letter2digit(letter);
		if(digit == -1){
			APP_LOG(APP_LOG_LEVEL_WARNING, "string2number had to deal with '%s' as an argument and failed",string);
			return -1;
		}
		result = result + (unit * digit);
		offset--;
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "string2number(%s) -> %i", string, result);
	return result;
}

// XX/XX 8:30 BLABLABLA
// 0123456789
static void apptDisplay(char *appt_string) {
	
	// Make sure there is no error in argument
	APP_LOG(APP_LOG_LEVEL_INFO, "apptDisplay started with argument '%s' ", appt_string);
	if (appt_string[0] == '\0') {
		APP_LOG(APP_LOG_LEVEL_WARNING, "appt_string is empty! ABORTING apptDisplay");
		return;
	} else if (sizeof(appt_string) != 4) {
		APP_LOG(APP_LOG_LEVEL_WARNING, "appt_string is too small (%i characters)! ABORTING apptDisplay", (int)(sizeof(appt_string)));
			text_layer_set_text(calendar_date_layer, appt_string); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
		return;
	}

	// Init some variables
	static char date_time_for_appt[20]; // = "Le XX XXXX à ##h##";
	static char stringBuffer[]="XX";
	APP_LOG(APP_LOG_LEVEL_INFO, "INIT : stringBuffer = '%s' ", stringBuffer);
	time_t now;
	struct tm *t;
	now = time(NULL);
	t = localtime(&now);
	static bool event_is_today = false;
	static bool event_is_all_day = false;
	
		//	Determine the variables
	static int appt_day;
					strncpy(stringBuffer, appt_string,2);
					APP_LOG(APP_LOG_LEVEL_INFO, "stringBuffer = '%s' ", stringBuffer);
					appt_day = string2number(stringBuffer);
				APP_LOG(APP_LOG_LEVEL_DEBUG,"appt_day is %i",appt_day);

	static int appt_month;
					strncpy(stringBuffer, appt_string+3,2);
					appt_month = string2number(stringBuffer);
				APP_LOG(APP_LOG_LEVEL_DEBUG,"appt_month is %i",appt_month);

	static int appt_hour;
					if (appt_string[7] == ':'){
						strncpy(stringBuffer, appt_string+5,2);
						stringBuffer[0]='0';
						appt_hour = string2number(stringBuffer);
					} else if (appt_string[8] == ':') {
						strncpy(stringBuffer, appt_string+6,2);
						appt_hour = string2number(stringBuffer);
					} else {
						APP_LOG(APP_LOG_LEVEL_DEBUG,"Event is ALL DAY");
						event_is_all_day = true;
					}
				APP_LOG(APP_LOG_LEVEL_DEBUG,"appt_hour is %i",appt_hour);

	static int appt_minute;
					if (appt_string[7] == ':'){
						strncpy(stringBuffer, appt_string+8,2);
						appt_minute = string2number(stringBuffer);
					} else if (appt_string[8] == ':') {
						strncpy(stringBuffer, appt_string+9,2);
						appt_minute = string2number(stringBuffer);
					} else {APP_LOG(APP_LOG_LEVEL_ERROR, "appt_minute cannot be determined...");}
				APP_LOG(APP_LOG_LEVEL_DEBUG,"appt_minute is %i",appt_minute);
		
	 static int hour_now;
	 static int min_now;
	 static int mday_now;
	 static int mon_now;
	 hour_now = t->tm_hour;
	 min_now = t->tm_min;
	 mday_now = t->tm_mday;
	 mon_now = t->tm_mon + 1;
	 APP_LOG(APP_LOG_LEVEL_DEBUG,"INIT : mon_now = '%i' ", mon_now);
		// Check the DAY and Month of Appointment and write it in date_of_appt
	static char date_of_appt[20];
	int interm = (appt_month - 1);
				APP_LOG(APP_LOG_LEVEL_DEBUG,"INIT : date_of_appt = '%s' ", date_of_appt);
				if ((appt_day < (mday_now) && ((mon_now) == appt_month)) || ((mon_now) > appt_month ) ) {
					snprintf(date_of_appt,20, "Depuis le %i %s",appt_day, month_of_year[interm]);
					event_is_all_day = true;
					APP_LOG(APP_LOG_LEVEL_DEBUG,"Event has started in the past, not today");
				} else if ( ((mday_now + 1) < (appt_day)) || ((mon_now) != appt_month) ) {
					snprintf(date_of_appt, 20, "Le %i %s à %ih%i",appt_day, month_of_year[interm], appt_hour, appt_minute);
					event_is_today = false; // Just so we don't write the time again
				} else if ((mday_now + 1) == (appt_day)) {
					snprintf(date_of_appt,20, "Demain à %ih%i",appt_hour, appt_minute);
					event_is_today = false; // Just so we don't write the time again
				} else {
					event_is_today = true;
				}
		// Check the Hour and write it in time_string
	 static char time_string[20];
				APP_LOG(APP_LOG_LEVEL_DEBUG,"INIT : time_string = '%s' ", time_string);
				if ((event_is_all_day) || (!event_is_today)) {
					APP_LOG(APP_LOG_LEVEL_DEBUG, "Do nothing with hour and minutes");
				} else if (((hour_now) > appt_hour) || (((hour_now) == appt_hour) && (min_now >= appt_minute))) {
					int hour_since = 0;
					int minutes_since = 0;
					minutes_since = ((min_now) - appt_minute);
					hour_since = ((hour_now) - appt_hour);
					if (minutes_since == 0) {
						hour_since += 1;
					} else if (minutes_since < 0) {
						hour_since -= 1;
						minutes_since += 60;
					}
					
					if ((minutes_since == 0) && hour_since == 0) {
						snprintf(time_string,20, "Maintenant !");
					} else if (minutes_since == 0) {
						if (hour_since == 1){
							snprintf(time_string,20, "Depuis 1 heure");
						} else {
							snprintf(time_string,20, "Depuis %i heures",hour_since);
						}
					} else if (hour_since == 0) {
						if (minutes_since == 1){
							snprintf(time_string,20, "Depuis 1 minute");
						} else {
							snprintf(time_string,20, "Depuis %i minutes",minutes_since);
						}
					} else {
						snprintf(time_string,20, "Depuis %ih %i",hour_since, minutes_since);
					}

				} else if (((hour_now) < appt_hour) || (((hour_now) == appt_hour) && (min_now < appt_minute))) {
					int hour_difference = 0;
					int minutes_difference = 0;
					minutes_difference = (appt_minute - (min_now));
					hour_difference = (appt_hour - (hour_now));
					if (minutes_difference == 0) {
						hour_difference += 1;
					} else if (minutes_difference < 0) {
						hour_difference -= 1;
						minutes_difference += 60;
					}
					
					if (minutes_difference == 0) {
						if (hour_difference == 1){
							snprintf(time_string,20, "Dans 1 heure");
						} else {
							snprintf(time_string,20, "Dans %i heures",hour_difference);
						}
					} else if (hour_difference == 0) {
						if (minutes_difference == 1){
							snprintf(time_string,20, "Dans 1 minute");
						} else {
							snprintf(time_string,20, "Dans %i minutes",minutes_difference);
						}
					} else {
						snprintf(time_string,20, "Dans %ih %i",hour_difference, minutes_difference);
					}
				}

	strcpy (date_time_for_appt,date_of_appt);
  	strcat (date_time_for_appt,time_string);


	text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
	layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
}

// End of calendar appointment utilities */

AppMessageResult sm_message_out_get(DictionaryIterator **iter_out) {
    AppMessageResult result = app_message_outbox_begin(iter_out);
    if(result != APP_MSG_OK) return result;
    dict_write_int32(*iter_out, SM_SEQUENCE_NUMBER_KEY, ++s_sequence_number);
    if(s_sequence_number == 0xFFFFFFFF) {
        s_sequence_number = 1;
    }
    return APP_MSG_OK;
}

void reset_sequence_number() {
    DictionaryIterator *iter = NULL;
    app_message_outbox_begin(&iter);
    if(!iter) return;
    dict_write_int32(iter, SM_SEQUENCE_NUMBER_KEY, 0xFFFFFFFF);
    app_message_outbox_send();
}


void sendCommand(int key) {
	DictionaryIterator* iterout;
	sm_message_out_get(&iterout);
    if(!iterout) return;
	
	dict_write_int8(iterout, key, -1);
	app_message_outbox_send();
}


void sendCommandInt(int key, int param) {
	DictionaryIterator* iterout;
	sm_message_out_get(&iterout);
    if(!iterout) return;
	
	dict_write_int8(iterout, key, param);
	app_message_outbox_send();
}





static void select_click_down_handler(ClickRecognizerRef recognizer, void *context) {
	//show the weather condition instead of temperature while center button is pressed
	layer_set_hidden(text_layer_get_layer(text_weather_temp_layer), true);
	layer_set_hidden(text_layer_get_layer(text_weather_cond_layer), false);
}

static void select_click_up_handler(ClickRecognizerRef recognizer, void *context) {
	//revert to showing the temperature 
	layer_set_hidden(text_layer_get_layer(text_weather_temp_layer), false);
	layer_set_hidden(text_layer_get_layer(text_weather_cond_layer), true);
}


/*static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	//update all data
	reset();
	
	sendCommandInt(SM_SCREEN_ENTER_KEY, STATUS_SCREEN_APP);
}*/  // Not Jailbreak
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	sendCommand(SM_OPEN_SIRI_KEY);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	//slide layers in/out

	property_animation_destroy((PropertyAnimation*)ani_in);
	property_animation_destroy((PropertyAnimation*)ani_out);


	ani_out = property_animation_create_layer_frame(animated_layer[active_layer], &GRect(0, 124, 143, 45), &GRect(-138, 124, 143, 45));
	animation_schedule((Animation*)ani_out);


	active_layer = (active_layer + 1) % (NUM_LAYERS);

	ani_in = property_animation_create_layer_frame(animated_layer[active_layer], &GRect(138, 124, 144, 45), &GRect(0, 124, 144, 45));
	animation_schedule((Animation*)ani_in);


}

static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_click_down_handler, select_click_up_handler, context);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

}

static void window_unload(Window *window) {
	
	
}

static void window_appear(Window *window)
{
	sendCommandInt(SM_SCREEN_ENTER_KEY, STATUS_SCREEN_APP);
	  
}


static void window_disappear(Window *window)
{
	sendCommandInt(SM_SCREEN_EXIT_KEY, STATUS_SCREEN_APP);
	
/*
	app_timer_cancel_event(g_app_context, timerUpdateCalendar);
	app_timer_cancel_event(g_app_context, timerUpdateMusic);
	app_timer_cancel_event(g_app_context, timerUpdateWeather);
*/	
}


void battery_layer_update_callback(Layer *me, GContext* ctx) {
	
	//draw the remaining battery percentage
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_fill_rect(ctx, GRect(2+16-(int)((batteryPercent/100.0)*16.0), 2, (int)((batteryPercent/100.0)*16.0), 8), 0, GCornerNone);
	
}

void battery_pbl_layer_update_callback(Layer *me, GContext* ctx) {
	
	//draw the remaining pebble battery percentage
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_fill_rect(ctx, GRect(2+16-(int)((batteryPblPercent/100.0)*16.0), 2, (int)((batteryPblPercent/100.0)*16.0), 8), 0, GCornerNone);
	
}


void reset() {
	
	layer_set_hidden(text_layer_get_layer(text_weather_temp_layer), true);
	layer_set_hidden(text_layer_get_layer(text_weather_cond_layer), false);
	text_layer_set_text(text_weather_cond_layer, "Mise a jour..."); 	
	
}


void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	if ((Precision_Is_Seconds) || ((units_changed & MINUTE_UNIT) == MINUTE_UNIT) || (!Watch_Face_Initialized)) {apptDisplay(calendar_date_str);}
	/*   //Future implementation
	// Display music if any is playing
	if ( ((tick_time->tm_sec % 10) == 5) && (music_is_playing) && (active_layer != MUSIC_LAYER)) {
		switchLayer();
	} else if ( ((tick_time->tm_sec % 10) == 0) && (active_layer == MUSIC_LAYER) ) {
		switchLayer();
	}
	if (compteur > 0) {
		compteur = (compteur - 1) ;
		if (compteur == 0) {
			music_is_playing = false;
		}
	}*/
if (((units_changed & MINUTE_UNIT) == MINUTE_UNIT) || (!Watch_Face_Initialized) ){
	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Time for a Change! I'm so EXCITED");
  char *time_format;

	static int heure;
	heure = tick_time->tm_hour;

	
  // TODO: Only update the date when it's changed. // DONE ! Even with SECOND ticks
	if ((units_changed & DAY_UNIT) == DAY_UNIT|| (!Watch_Face_Initialized) ){
		  Watch_Face_Initialized = true;
  	static char date_text[] = "DAY 00 MOIS";
  		// Get the day and month as int
  	static int day_int;
	 day_int = tick_time->tm_wday;
	static int month_int;
	 month_int = tick_time->tm_mon;
	 	// Print the result
   snprintf(date_text, sizeof(date_text), "%s %i %s", day_of_week[day_int], tick_time->tm_mday, month_of_year[month_int]);
   text_layer_set_text(text_date_layer, date_text);
	  APP_LOG(APP_LOG_LEVEL_INFO, "Displayed date : [%s %i %s]", day_of_week[day_int], tick_time->tm_mday, month_of_year[month_int]);
  	}

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

	
	// Don't forget the "heure" variable if you copy this small paragraph
  if (((units_changed & HOUR_UNIT) == HOUR_UNIT) && ((heure > 9) && (heure < 23))){
    vibes_double_pulse();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Hour changed -> Vibration complete");
  } else {APP_LOG(APP_LOG_LEVEL_DEBUG, "However, Hour Unit did not change, no vibration");}
	
  text_layer_set_text(text_time_layer, time_text);
}
}


void reconnect(void *data) {
	reset();

	sendCommandInt(SM_SCREEN_ENTER_KEY, STATUS_SCREEN_APP);
	
}

void bluetoothChanged(bool connected) {

	if (connected) {
		app_timer_register(5000, reconnect, NULL);
	} else {
		bitmap_layer_set_bitmap(weather_image, weather_status_imgs[NUM_WEATHER_IMAGES-1]);
		vibes_double_pulse();
	}
	
}


void batteryChanged(BatteryChargeState batt) {
	
	batteryPblPercent = batt.charge_percent;
	layer_mark_dirty(battery_layer);
	
}


static void init(void) {
  window = window_create();
  window_set_fullscreen(window, true);
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
	.appear = window_appear,
	.disappear = window_disappear
  });
  const bool animated = true;
  window_stack_push(window, animated);

	//init weather images
	for (int i=0; i<NUM_WEATHER_IMAGES; i++) {
	  	weather_status_imgs[i] = gbitmap_create_with_resource(WEATHER_IMG_IDS[i]);
	}
	
  	bg_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);


  	Layer *window_layer = window_get_root_layer(window);

	//init background image
  	GRect bg_bounds = layer_get_frame(window_layer);

	background_image = bitmap_layer_create(bg_bounds);
	layer_add_child(window_layer, bitmap_layer_get_layer(background_image));
	bitmap_layer_set_bitmap(background_image, bg_image);
	

	//init weather layer and add weather image, weather condition, temperature, and battery indicator
	weather_layer = layer_create(GRect(0, 78, 144, 45));
	layer_add_child(window_layer, weather_layer);

	battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_PHONE);
	battery_pbl_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_PEBBLE);

	battery_image_layer = bitmap_layer_create(GRect(100, 7, 37, 14));
	layer_add_child(weather_layer, bitmap_layer_get_layer(battery_image_layer));
	bitmap_layer_set_bitmap(battery_image_layer, battery_image);

	battery_pbl_image_layer = bitmap_layer_create(GRect(100, 23, 37, 14));
	layer_add_child(weather_layer, bitmap_layer_get_layer(battery_pbl_image_layer));
	bitmap_layer_set_bitmap(battery_pbl_image_layer, battery_pbl_image);


	text_battery_layer = text_layer_create(GRect(99, 20, 40, 60));
	text_layer_set_text_alignment(text_battery_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_battery_layer, GColorWhite);
	text_layer_set_background_color(text_battery_layer, GColorClear);
	text_layer_set_font(text_battery_layer,  fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(weather_layer, text_layer_get_layer(text_battery_layer));
	text_layer_set_text(text_battery_layer, "-");
	layer_set_hidden(text_layer_get_layer(text_battery_layer), true);


	battery_layer = layer_create(GRect(102, 8, 19, 11));
	layer_set_update_proc(battery_layer, battery_layer_update_callback);
	layer_add_child(weather_layer, battery_layer);

	batteryPercent = 100;
	layer_mark_dirty(battery_layer);

	battery_pbl_layer = layer_create(GRect(102, 24, 19, 11));
	layer_set_update_proc(battery_pbl_layer, battery_pbl_layer_update_callback);
	layer_add_child(weather_layer, battery_pbl_layer);

	BatteryChargeState pbl_batt = battery_state_service_peek();
	batteryPblPercent = pbl_batt.charge_percent;
	layer_mark_dirty(battery_pbl_layer);


	text_weather_cond_layer = text_layer_create(GRect(48, 1, 48, 40)); // GRect(5, 2, 47, 40)
	text_layer_set_text_alignment(text_weather_cond_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_weather_cond_layer, GColorWhite);
	text_layer_set_background_color(text_weather_cond_layer, GColorClear);
	text_layer_set_font(text_weather_cond_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(weather_layer, text_layer_get_layer(text_weather_cond_layer));

	layer_set_hidden(text_layer_get_layer(text_weather_cond_layer), false);
	text_layer_set_text(text_weather_cond_layer, "Mise a jour..."); 	
	
	if (bluetooth_connection_service_peek()) {
		weather_img = 0;
	} else {
		weather_img = NUM_WEATHER_IMAGES - 1;
	}

	weather_image = bitmap_layer_create(GRect(5, 2, 40, 40)); 
	layer_add_child(weather_layer, bitmap_layer_get_layer(weather_image));
	bitmap_layer_set_bitmap(weather_image, weather_status_imgs[weather_img]);


	text_weather_temp_layer = text_layer_create(GRect(48, 3, 48, 40)); 
	text_layer_set_text_alignment(text_weather_temp_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_weather_temp_layer, GColorWhite);
	text_layer_set_background_color(text_weather_temp_layer, GColorClear);
	text_layer_set_font(text_weather_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	layer_add_child(weather_layer, text_layer_get_layer(text_weather_temp_layer));
	text_layer_set_text(text_weather_temp_layer, "-°"); 	

	layer_set_hidden(text_layer_get_layer(text_weather_temp_layer), true);

	
	//init layers for time and date
	text_date_layer = text_layer_create(bg_bounds);
	text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	layer_set_frame(text_layer_get_layer(text_date_layer), GRect(0, 45, 144, 30));
	text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));


	text_time_layer = text_layer_create(bg_bounds);
	text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	layer_set_frame(text_layer_get_layer(text_time_layer), GRect(0, -5, 144, 50));
	text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));


	//init calendar layer
	animated_layer[CALENDAR_LAYER] = layer_create(GRect(0, 124, 144, 45));
	layer_add_child(window_layer, animated_layer[CALENDAR_LAYER]);
	
	calendar_date_layer = text_layer_create(GRect(6, 0, 132, 21));
	text_layer_set_text_alignment(calendar_date_layer, GTextAlignmentLeft);
	text_layer_set_text_color(calendar_date_layer, GColorWhite);
	text_layer_set_background_color(calendar_date_layer, GColorClear);
	text_layer_set_font(calendar_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(animated_layer[CALENDAR_LAYER], text_layer_get_layer(calendar_date_layer));
	text_layer_set_text(calendar_date_layer, "Aucun"); 	


	calendar_text_layer = text_layer_create(GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(calendar_text_layer, GTextAlignmentLeft);
	text_layer_set_text_color(calendar_text_layer, GColorWhite);
	text_layer_set_background_color(calendar_text_layer, GColorClear);
	text_layer_set_font(calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(animated_layer[CALENDAR_LAYER], text_layer_get_layer(calendar_text_layer));
	text_layer_set_text(calendar_text_layer, "Evènement");
	
	
	
	//init music layer
	animated_layer[MUSIC_LAYER] = layer_create(GRect(144, 124, 144, 45));
	layer_add_child(window_layer, animated_layer[MUSIC_LAYER]);
	
	music_artist_layer = text_layer_create(GRect(6, 0, 132, 21));
	text_layer_set_text_alignment(music_artist_layer, GTextAlignmentLeft);
	text_layer_set_text_color(music_artist_layer, GColorWhite);
	text_layer_set_background_color(music_artist_layer, GColorClear);
	text_layer_set_font(music_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(animated_layer[MUSIC_LAYER], text_layer_get_layer(music_artist_layer));
	text_layer_set_text(music_artist_layer, "Aucune lecture"); 	


	music_song_layer = text_layer_create(GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(music_song_layer, GTextAlignmentLeft);
	text_layer_set_text_color(music_song_layer, GColorWhite);
	text_layer_set_background_color(music_song_layer, GColorClear);
	text_layer_set_font(music_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(animated_layer[MUSIC_LAYER], text_layer_get_layer(music_song_layer));
	text_layer_set_text(music_song_layer, "en cours");


	active_layer = CALENDAR_LAYER;

	reset();

  	//tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick); 
	// Sorry, but PRECISION is crucial with appointments! 
	//By the way, the 2nd argument is the name of the fucntion you run each second. I changed it
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

	bluetooth_connection_service_subscribe(bluetoothChanged);
	battery_state_service_subscribe(batteryChanged);

}

static void deinit(void) {
	
	
	property_animation_destroy((PropertyAnimation*)ani_in);
	property_animation_destroy((PropertyAnimation*)ani_out);
	

	
	if (timerUpdateCalendar != NULL)
		app_timer_cancel(timerUpdateCalendar);
	timerUpdateCalendar = NULL;

	if (timerUpdateWeather != NULL)	
		app_timer_cancel(timerUpdateWeather);
	timerUpdateWeather = NULL;
	
	if (timerUpdateMusic != NULL)
		app_timer_cancel(timerUpdateMusic);
	timerUpdateMusic = NULL;
	


	bitmap_layer_destroy(background_image);
	layer_destroy(weather_layer);
	bitmap_layer_destroy(battery_image_layer);
	bitmap_layer_destroy(battery_pbl_image_layer);
	text_layer_destroy(text_battery_layer);
	layer_destroy(battery_layer);
	layer_destroy(battery_pbl_layer);
	text_layer_destroy(text_weather_cond_layer);
	bitmap_layer_destroy(weather_image);
	text_layer_destroy(text_weather_temp_layer);
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_time_layer);
	text_layer_destroy(calendar_date_layer);
	text_layer_destroy(calendar_text_layer);
	text_layer_destroy(music_artist_layer);
	text_layer_destroy(music_song_layer);
	

	for (int i=0; i<NUM_LAYERS; i++) {
		if (animated_layer[i]!=NULL)
			layer_destroy(animated_layer[i]);
	}

	for (int i=0; i<NUM_WEATHER_IMAGES; i++) {
	  	gbitmap_destroy(weather_status_imgs[i]);
	}
	

	gbitmap_destroy(bg_image);
	gbitmap_destroy(battery_image);
	gbitmap_destroy(battery_pbl_image);


	tick_timer_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	battery_state_service_unsubscribe();

  
  window_destroy(window);
}


static void updateWeather(void *data) {
	sendCommand(SM_STATUS_UPD_WEATHER_KEY);	
}

static void updateCalendar(void *data) {
	sendCommand(SM_STATUS_UPD_CAL_KEY);	
}

static void updateMusic(void *data) {
	sendCommand(SM_SONG_LENGTH_KEY);	
}


void rcv(DictionaryIterator *received, void *context) {
	// Got a message callback
	Tuple *t;


	t=dict_find(received, SM_WEATHER_COND_KEY); 
	if (t!=NULL) {
		memcpy(weather_cond_str, t->value->cstring, strlen(t->value->cstring));
        weather_cond_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(text_weather_cond_layer, weather_cond_str); 	
	}

	t=dict_find(received, SM_WEATHER_TEMP_KEY); 
	if (t!=NULL) {
		memcpy(weather_temp_str, t->value->cstring, strlen(t->value->cstring));
        weather_temp_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(text_weather_temp_layer, weather_temp_str); 
		
		layer_set_hidden(text_layer_get_layer(text_weather_cond_layer), true);
		layer_set_hidden(text_layer_get_layer(text_weather_temp_layer), false);
			
	}

	t=dict_find(received, SM_WEATHER_ICON_KEY); 
	if (t!=NULL) {
		bitmap_layer_set_bitmap(weather_image, weather_status_imgs[t->value->uint8]);	  	
	}

	t=dict_find(received, SM_COUNT_BATTERY_KEY); 
	if (t!=NULL) {
		batteryPercent = t->value->uint8;
		layer_mark_dirty(battery_layer);
		snprintf(string_buffer, sizeof(string_buffer), "%d", batteryPercent);
		text_layer_set_text(text_battery_layer, string_buffer ); 	
	}

	t=dict_find(received, SM_STATUS_CAL_TIME_KEY);   // I changed this if that's what you're looking for
 	if (t!=NULL) {
 		memcpy(calendar_date_str, t->value->cstring, strlen(t->value->cstring));
          calendar_date_str[strlen(t->value->cstring)] = '\0';
 		//text_layer_set_text(&calendar_date_layer, calendar_date_str);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received DATA for Calendar");
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Launching Appointment Module [apptDisplay]");
		apptDisplay(calendar_date_str);
  	}
	
	t=dict_find(received, SM_STATUS_CAL_TEXT_KEY); 
	if (t!=NULL) {
		memcpy(calendar_text_str, t->value->cstring, strlen(t->value->cstring));
        calendar_text_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(calendar_text_layer, calendar_text_str); 	
		// Resize Calendar text if needed
		if(strlen(calendar_text_str) <= 15)
			text_layer_set_font(calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
		else
			if(strlen(calendar_text_str) <= 18)
				text_layer_set_font(calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
			else 
				text_layer_set_font(calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	}

	t=dict_find(received, SM_STATUS_MUS_ARTIST_KEY); 
	if (t!=NULL) {
		memcpy(music_artist_str1, t->value->cstring, strlen(t->value->cstring));

        music_artist_str1[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(music_artist_layer, music_artist_str1); 	
	}

	t=dict_find(received, SM_STATUS_MUS_TITLE_KEY); 
	if (t!=NULL) {
		memcpy(music_title_str1, t->value->cstring, strlen(t->value->cstring));

        music_title_str1[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(music_song_layer, music_title_str1); 	
	}


	t=dict_find(received, SM_STATUS_UPD_WEATHER_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		if (timerUpdateWeather != NULL)
			app_timer_cancel(timerUpdateWeather);
		timerUpdateWeather = app_timer_register(interval , updateWeather, NULL);
	}

	t=dict_find(received, SM_STATUS_UPD_CAL_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		if (timerUpdateCalendar != NULL)
			app_timer_cancel(timerUpdateCalendar);
		timerUpdateCalendar = app_timer_register(interval , updateCalendar, NULL);
	}

	t=dict_find(received, SM_SONG_LENGTH_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		if (timerUpdateMusic != NULL)
			app_timer_cancel(timerUpdateMusic);
		timerUpdateMusic = app_timer_register(interval , updateMusic, NULL);

	}

}

int main(void) {
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum() );
	app_message_register_inbox_received(rcv);
	
  init();


  app_event_loop();
  app_message_deregister_callbacks();

  deinit();

}
