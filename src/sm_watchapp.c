#include <pebble.h>
#include "globals.h"


#define STRING_LENGTH 255
#define NUM_WEATHER_IMAGES	9


	// Mes variables
static bool Watch_Face_Initialized = false;
static bool Precision_Is_Seconds = false;
static bool music_is_playing = false;
static char appointment_time[30];
static int compteur = 0; //I know it's not right, but timer is not working anymore with SDK 2.0 RC :'( 
static char MusicBuffer[10] = "No Title";
	
	
enum {CALENDAR_LAYER, MUSIC_LAYER, NUM_LAYERS};

static void reset();

static Window *window;
static TextLayer *text_layer;

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




static uint32_t s_sequence_number = 0xFFFFFFFE;
/** —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
—————————————————————————————————————————————————————Calendar Appointments———————————————————————————————————————————————————————————————————
**/

/* Convert letter to digit */
int letter2digit(char letter) {
	if((letter >= 48) && (letter <=57)) {
		return letter - 48;
	}
	
	return -1;
}

/* Convert string to number */
static int string2number(char *string) {
	int32_t result = 0;
	int32_t offset = strlen(string) - 1;
	int32_t digit = -1;
	int32_t unit = 1;
	int8_t letter;	

	for(unit = 1; offset >= 0; unit = unit * 10) {
		letter = string[offset];
		digit = letter2digit(letter);
		if(digit == -1) return -1;
		result = result + (unit * digit);
		offset--;
	}
	
	return result;
}

/* Convert time string ("HH:MM") to number of minutes */
static int timestr2minutes(char *timestr) {
	//timestr = "00:00PM XXXX..." or "0:00PM XXXX..."
	static char hourStr[3], minStr[3];
	int32_t hour, min;
	int8_t hDigits = 2;

	if(timestr[1] == ':') hDigits = 1;
	
	strncpy(hourStr, timestr, hDigits);
	strncpy(minStr, timestr+hDigits+1, 2);
	
	hour = string2number(hourStr);
	if(hour == -1) return -1;
	
	min = string2number(minStr);
	if(min == -1) return -1;
	
	//Manage AM/PM
	if ((timestr[4] == 'P') || (appointment_time[5] == 'P')) {
            hour += 12;
        }
	
	return min + (hour * 60);
}

void apptDisplay() {
	static int apptInMinutes, timeInMinutes, apptInDays, timeInDays, apptInMonths, timeInMonths;
	static char date_time_for_appt[] = "00/00 00:00 XXXXXXX";
	time_t now;
	struct tm *t;
	
	now = time(NULL);
	t = localtime(&now);
		
	
	// Make sure it's not an all day event
	bool appt_is_all_day(char *horaire) { 
		if (horaire[6] == 'a' && horaire[7] == 'l') {
			return true;
		} else { return false; }
	}
	
  if (appt_is_all_day(appointment_time)) {      //  EVENT IS AN ALL DAY EVENT
	  // Assign values
	static char textBuffer[] = "00";
		strncpy(textBuffer, appointment_time + 3,2);
	apptInDays = string2number(textBuffer);
	timeInDays = t->tm_mday;
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in days is %i", timeInDays);
		strncpy(textBuffer, appointment_time,2);
	apptInMonths = string2number(textBuffer);
	timeInMonths = (t->tm_mon + 1); 
	  
	  if (apptInDays - timeInDays == 1) {
				snprintf(date_time_for_appt, 20, "Demain");
				text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
				} else if (apptInDays == timeInDays) {
				      snprintf(date_time_for_appt, 20, "Aujourd'hui");
				// Change lines above for time format, current is days/months
					  text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
			} else {
				      snprintf(date_time_for_appt, 20, "Le %d/%d", apptInDays, apptInMonths);
				// Change lines above for time format, current is days/months
					  text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
				    } 
  } else {
		// First, we assign values
	  
	apptInMinutes = timestr2minutes(appointment_time + 6);
	timeInMinutes = (t->tm_hour * 60) + t->tm_min;
		static char textBuffer[] = "00";
		strncpy(textBuffer, appointment_time + 3,2);
	apptInDays = string2number(textBuffer);
	timeInDays = t->tm_mday;
		strncpy(textBuffer, appointment_time,2);
	apptInMonths = string2number(textBuffer);
	timeInMonths = (t->tm_mon + 1);
	  
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "apptDisplay: Appointment Time is %d minutes. Time in Minutes is %d", (int)apptInMinutes, (int)timeInMinutes);  
	
	// Manage appoitment notification
	
	if(apptInMinutes >= 0) {
		//if(apptInMinutes < timeInMinutes) {
			//layer_set_hidden(&calendar_layer, 1); 	
		//}
		if ((apptInDays > timeInDays)||(apptInMonths > timeInMonths)) {
			if ((apptInMinutes == 0) && (apptInDays - timeInDays == 1)) { 
			snprintf(date_time_for_appt, 20, "A Minuit");
				// Change lines above for time format, current is days/months
			text_layer_set_text(calendar_date_layer, date_time_for_appt);; 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
			} else if ((apptInDays - timeInDays == 1) && (((apptInMinutes) % 60) == 0)) {
				snprintf(date_time_for_appt, 20, "Demain, %dh", (int)((apptInMinutes)/ 60));
				text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
            } else if (apptInDays - timeInDays == 1) {
				snprintf(date_time_for_appt, 20, "Demain, %dh %d", (int)((apptInMinutes)/ 60),(int)((apptInMinutes) % 60));
				text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
            } else {
				      snprintf(date_time_for_appt, 20, "%d/%d %dh %d", apptInDays, apptInMonths, (int)((apptInMinutes)/ 60),
							   (int)((apptInMinutes) % 60));
				// Change lines above for time format, current is days/months
					  text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
			}  	
		}  else if (apptInMinutes == 0) { 
			snprintf(date_time_for_appt, 20, "Aucun");
			text_layer_set_text(calendar_date_layer, date_time_for_appt);; 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
        } else if(timeInMinutes - apptInMinutes == 1) {
			snprintf(date_time_for_appt, 20, "Depuis %d minute", (int)(timeInMinutes - apptInMinutes));
			text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
		} else if((apptInMinutes < timeInMinutes) && (((timeInMinutes - apptInMinutes) % 60) == 0)) {
			snprintf(date_time_for_appt, 20, "Depuis %dh", 
					 (int)((timeInMinutes - apptInMinutes)/60));
			text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);
			vibes_short_pulse();
		} else if((apptInMinutes < timeInMinutes) && (((timeInMinutes - apptInMinutes) / 60) > 0) && (((timeInMinutes - apptInMinutes) % 60) > 0)) {
			snprintf(date_time_for_appt, 20, "Depuis %dh %dmin", 
					 (int)((timeInMinutes - apptInMinutes)/60),(int)((timeInMinutes - apptInMinutes)%60));
			text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
		} else if(apptInMinutes < timeInMinutes) {
			snprintf(date_time_for_appt, 20, "Depuis %d minutes", (int)(timeInMinutes - apptInMinutes));
			text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
		} else if(apptInMinutes > timeInMinutes) {
			if(((apptInMinutes - timeInMinutes) / 60) > 0) {
				snprintf(date_time_for_appt, 20, "Dans %dh %dm", 
						 (int)((apptInMinutes - timeInMinutes) / 60),
						 (int)((apptInMinutes - timeInMinutes) % 60));
			} else if ((apptInMinutes - timeInMinutes) <= 1 ) {  // Décompte en secondes
				int timeInSeconds = t->tm_sec;
				snprintf(date_time_for_appt, 20, "Dans %d secondes", (int)(60 - timeInSeconds));
				if (timeInSeconds > 1){Precision_Is_Seconds = true;}
							// On va pas faire chier pour le 's' quand il reste 1 seconde hein !
			} else if ((apptInMinutes - timeInMinutes) <= 2 ) {  // Décompte en secondes
				int timeInSeconds = t->tm_sec;
				//Vibrate if event is in 15 minutes
					if((timeInSeconds == 0) && ((apptInMinutes - timeInMinutes) == 15)) {
											vibes_short_pulse();
									}
				snprintf(date_time_for_appt, 20, "Dans 1 minute %d", (int)(60 - timeInSeconds));
				if (timeInSeconds > 1){Precision_Is_Seconds = true;}
							// On va pas faire chier pour le 's' quand il reste 1 seconde hein !
			} else if ((apptInMinutes - timeInMinutes) <= 15 ) {  // Décompte en secondes
				int timeInSeconds = t->tm_sec;
				//Vibrate if event is in 15 minutes
					if((timeInSeconds == 0) && ((apptInMinutes - timeInMinutes) == 15)) {
											vibes_short_pulse();
									}
				snprintf(date_time_for_appt, 20, "Dans %d minutes %d", (int)(apptInMinutes - timeInMinutes - 1), (int)(60 - timeInSeconds));
				if (timeInSeconds > 1){Precision_Is_Seconds = true;}
							// On va pas faire chier pour le 's' quand il reste 1 seconde hein !
			} else {
				snprintf(date_time_for_appt, 20, "Dans %d minutes", (int)(apptInMinutes - timeInMinutes));
			}
			text_layer_set_text(calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
		} else if (apptInMinutes == timeInMinutes) {
			Precision_Is_Seconds = false;
			text_layer_set_text(calendar_date_layer, "Maintenant!"); 	
			layer_set_hidden(animated_layer[CALENDAR_LAYER], 0);  	
			vibes_double_pulse();
		} 
		
		
	} // Appointment minutes are positive test
  } // Appointment is all day test
APP_LOG(APP_LOG_LEVEL_DEBUG, "apptDisplay finished the work :D");
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


static void switchLayer() {
	//slide layers in/out

	property_animation_destroy((PropertyAnimation*)ani_in);
	property_animation_destroy((PropertyAnimation*)ani_out);


	ani_out = property_animation_create_layer_frame(animated_layer[active_layer], &GRect(0, 124, 143, 45), &GRect(-138, 124, 143, 45));
	animation_schedule((Animation*)ani_out);


	active_layer = (active_layer + 1) % (NUM_LAYERS);

	ani_in = property_animation_create_layer_frame(animated_layer[active_layer], &GRect(138, 124, 144, 45), &GRect(0, 124, 144, 45));
	animation_schedule((Animation*)ani_in);


		APP_LOG(APP_LOG_LEVEL_DEBUG, "Switched Layers");
}

/*
static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_click_down_handler, select_click_up_handler, context);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

*/
// If you reenable, you'll need the functions. I deleted them for space
// You'll CLICK CONFIG PROVIDER (CMD-F search-it, it's deactivated)

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
	if ((Precision_Is_Seconds) || ((units_changed & MINUTE_UNIT) == MINUTE_UNIT)) {apptDisplay();}
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
	}
if (((units_changed & MINUTE_UNIT) == MINUTE_UNIT) || (!Watch_Face_Initialized) ){
	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Time for a Change! I'm so EXCITED");
  char *time_format;

	int heure = tick_time->tm_hour;

	
  // TODO: Only update the date when it's changed. // DONE ! Even with SECOND ticks
	if ((units_changed & DAY_UNIT) == DAY_UNIT|| (!Watch_Face_Initialized) ){
		  Watch_Face_Initialized = true;
  static char date_text[] = "Xxx 00 Xxx";
	// Check 24h mode to know how to handle date
  if (clock_is_24h_style()) {
   
	     // Format is 24h, so TRANSLATE !  
   strftime(date_text, sizeof(date_text), "%a %e %b", tick_time); //EU mode
  APP_LOG(APP_LOG_LEVEL_DEBUG,"DATE is %s", date_text);
   text_layer_set_text(text_date_layer, date_text);
	  APP_LOG(APP_LOG_LEVEL_DEBUG,"I got the date in European Version. Or should I say... Non-english one?");
	  // Primitive hack to translate the day of week to another language
			// Needs to be exactly 3 characters, e.g. "Mon" or "Mo "
			// Supported characters: A-Z, a-z, 0-9
			
			if (date_text[0] == 'M')
			{
				memcpy(&date_text, "Lun", 3); // Monday
			}
			
			if (date_text[0] == 'T' && date_text[1] == 'u')
			{
				memcpy(&date_text, "Mar", 3); // Tuesday
			}
			
			if (date_text[0] == 'W')
			{
				memcpy(&date_text, "Mer", 3); // Wednesday
			}
			
			if (date_text[0] == 'T' && date_text[1] == 'h')
			{
				memcpy(&date_text, "Jeu", 3); // Thursday
			}
			
			if (date_text[0] == 'F')
			{
				memcpy(&date_text, "Ven", 3); // Friday
			}
			
			if (date_text[0] == 'S' && date_text[1] == 'a')
			{
				memcpy(&date_text, "Sam", 3); // Saturday
			}
			
			if (date_text[0] == 'S' && date_text[1] == 'u')
			{
				memcpy(&date_text, "Dim", 3); // Sunday
			}
			
			
			 
			//Primitive Hack to translate month - Only a few are translated because in french, most of the beginnings are similar to english
			if (date_text[7] == 'F')
			{
				memmove(&date_text[7], "Fev", sizeof(date_text)); // Fevrier
			}
			
			if (date_text[7] == 'A' && date_text[9] == 'r')
			{
				memmove(&date_text[7], "Avr", sizeof(date_text)); // Avril
			}
			
			if (date_text[7] == 'M' && date_text[9] == 'y')
			{
				memmove(&date_text[7], "Mai", sizeof(date_text)); // Mai
			}
			
			if (date_text[7] == 'A' && date_text[9] == 'g')
			{
				memmove(&date_text[7], "Aou", sizeof(date_text)); // Aout
			}
			
			
			
			//  strcat(date_text, month_text);	
				
	if ((date_text[4] == '0') && (date_text[5] == '1')) {
		memcpy(&date_text[4], "1e", 2); //hack to translate 1 in 1e in French
	} 
	else if (date_text[4] == '0') {
			    // Hack to get rid of the leading zero of the day of month
	            memmove(&date_text[4], &date_text[5], sizeof(date_text) - 1);
			    }
	  
	  APP_LOG(APP_LOG_LEVEL_INFO, "Translated the date in French. Affichage!");
	  
  } else {
   strftime(date_text, sizeof(date_text), "%a, %b %e", tick_time);
   text_layer_set_text(text_date_layer, date_text);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time is set to 12Hr Mode. I didn't translate to French. I still like Croissants though...");
	  APP_LOG(APP_LOG_LEVEL_INFO, "Displayed date");
  }
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

		APP_LOG(APP_LOG_LEVEL_DEBUG, "Begin Init...");
  window = window_create();
  window_set_fullscreen(window, true);
  //window_set_click_config_provider(window, click_config_provider);             // CLICK CONFIG PROVIDER HERE
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
	text_layer_set_text(calendar_date_layer, "Chargement..."); 	


	calendar_text_layer = text_layer_create(GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(calendar_text_layer, GTextAlignmentLeft);
	text_layer_set_text_color(calendar_text_layer, GColorWhite);
	text_layer_set_background_color(calendar_text_layer, GColorClear);
	text_layer_set_font(calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(animated_layer[CALENDAR_LAYER], text_layer_get_layer(calendar_text_layer));
	text_layer_set_text(calendar_text_layer, "Erreur ?");
	
	
	
	//init music layer
	animated_layer[MUSIC_LAYER] = layer_create(GRect(144, 124, 144, 45));
	layer_add_child(window_layer, animated_layer[MUSIC_LAYER]);
	
	music_artist_layer = text_layer_create(GRect(6, 0, 132, 21));
	text_layer_set_text_alignment(music_artist_layer, GTextAlignmentLeft);
	text_layer_set_text_color(music_artist_layer, GColorWhite);
	text_layer_set_background_color(music_artist_layer, GColorClear);
	text_layer_set_font(music_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(animated_layer[MUSIC_LAYER], text_layer_get_layer(music_artist_layer));
	text_layer_set_text(music_artist_layer, " "); 	


	music_song_layer = text_layer_create(GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(music_song_layer, GTextAlignmentLeft);
	text_layer_set_text_color(music_song_layer, GColorWhite);
	text_layer_set_background_color(music_song_layer, GColorClear);
	text_layer_set_font(music_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(animated_layer[MUSIC_LAYER], text_layer_get_layer(music_song_layer));
	text_layer_set_text(music_song_layer, " ");


	active_layer = CALENDAR_LAYER;

	reset();

  	//tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick); 
	// Sorry, but PRECISION is crucial with appointments! 
	//By the way, the 2nd argument is the name of the fucntion you run each second. I changed it
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

	bluetooth_connection_service_subscribe(bluetoothChanged);
	battery_state_service_subscribe(batteryChanged);

	
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Init -> DONE");
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


static void resetMusicCount() {
	compteur = 120;
	music_is_playing = true;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "compteur has been reset, music is now playing");
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
  		strncpy(appointment_time, calendar_date_str, 30); //Copy time to appointment_time so that apptDisplay can use it
		APP_LOG(APP_LOG_LEVEL_INFO, "Just RECEIVED appointment time : %s", appointment_time);
		//APP_LOG(APP_LOG_LEVEL_WARNING, "[DEBUG_HERE]Launching Appointment Module [apptDisplay]");
		apptDisplay();
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
		if ((strcmp(MusicBuffer,music_title_str1) != 0) && (strcmp("No Title",music_title_str1) != 0)) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "New Music Title received");
		resetMusicCount();
		switchLayer();
		strcpy(MusicBuffer, music_title_str1);
		}
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
	APP_LOG(APP_LOG_LEVEL_DEBUG, "SM_SONG_LENGTH is not NULL! Interval = %i",interval); //I wonder what this call does. I added this debug to clarify
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
