#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "globals.h"

#define WATCHFACE 1
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD } 
//it seems like changing UUID makes the app unable to retrieve datas from phone
			PBL_APP_INFO(MY_UUID,
             "FrenchStatus", "Robert Hesse", //Modified by Alexandre Jouandin
             0, 3,
             RESOURCE_ID_APP_ICON,
             #if WATCHFACE
             	APP_INFO_WATCH_FACE
			 #else
			 	APP_INFO_STANDARD_APP
			 #endif
);

#define STRING_LENGTH 255
#define NUM_WEATHER_IMAGES	8
#define VIBE_ON_HOUR true

typedef enum {CALENDAR_LAYER, MUSIC_LAYER, NUM_LAYERS} AnimatedLayers;


AppMessageResult sm_message_out_get(DictionaryIterator **iter_out);
void reset_sequence_number();
char* int_to_str(int num, char *outbuf);
void sendCommand(int key);
void sendCommandInt(int key, int param);
void rcv(DictionaryIterator *received, void *context);
void dropped(void *context, AppMessageResult reason);
void select_up_handler(ClickRecognizerRef recognizer, Window *window);
void select_down_handler(ClickRecognizerRef recognizer, Window *window);
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void config_provider(ClickConfig **config, Window *window);
void battery_layer_update_callback(Layer *me, GContext* ctx);
void handle_status_appear(Window *window);
void handle_status_disappear(Window *window);
void handle_init(AppContextRef ctx);
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t);
void handle_deinit(AppContextRef ctx);	
void reset();	
	
AppContextRef g_app_context;

bool Watch_Face_Initialized = false;

static Window window;
static PropertyAnimation ani_out, ani_in;

static Layer animated_layer[NUM_LAYERS], weather_layer;
static Layer battery_layer;

static TextLayer text_date_layer, text_time_layer;

static TextLayer text_weather_cond_layer, text_weather_temp_layer, text_battery_layer;
static TextLayer calendar_date_layer, calendar_text_layer;
static TextLayer music_artist_layer, music_song_layer;
 
static BitmapLayer background_image, weather_image, battery_image_layer;

static int active_layer;

static char string_buffer[STRING_LENGTH];
static char weather_cond_str[STRING_LENGTH], weather_temp_str[5];
static int weather_img, batteryPercent;

static char calendar_date_str[STRING_LENGTH], calendar_text_str[STRING_LENGTH];
static char music_artist_str[STRING_LENGTH], music_title_str[STRING_LENGTH];
static bool music_is_playing = false;

static char appointment_time[50];

HeapBitmap bg_image, battery_image;
HeapBitmap weather_status_imgs[NUM_WEATHER_IMAGES];

static AppTimerHandle timerUpdateCalendar = 0;
static AppTimerHandle timerUpdateWeather = 0;
static AppTimerHandle timerUpdateMusic = 0;



const int WEATHER_IMG_IDS[] = {
  RESOURCE_ID_IMAGE_SUN,
  RESOURCE_ID_IMAGE_RAIN,
  RESOURCE_ID_IMAGE_CLOUD,
  RESOURCE_ID_IMAGE_SUN_CLOUD,
  RESOURCE_ID_IMAGE_FOG,
  RESOURCE_ID_IMAGE_WIND,
  RESOURCE_ID_IMAGE_SNOW,
  RESOURCE_ID_IMAGE_THUNDER
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
int string2number(char *string) {
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
int timestr2minutes(char *timestr) {
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
	int apptInMinutes, timeInMinutes, apptInDays, timeInDays, apptInMonths, timeInMonths;
	static char date_time_for_appt[] = "00/00 00:00 XXXXXXX";
	// format of appointment_time : "MM/DD 12:34PM LOCATIONXXXX" or "MM/DD all-day LOCATIONXXXX"
	// format of appointment_time : "0--3—-6----11-------------"
	PblTm t;
	
	get_time(&t);
	
	
	// Make sure it's not an all day event
	bool appt_is_all_day(char *time) { 
		if (time[6] == 'a' && time[7] == 'l') {
			return true;
		} else { return false; }
	}
	
  if (appt_is_all_day(appointment_time)) {
		// Assign values
	static char textBuffer[] = "00";
		strncpy(textBuffer, appointment_time + 3,2);
	apptInDays = string2number(textBuffer);
	timeInDays = t.tm_mday;
		strncpy(textBuffer, appointment_time,2);
	apptInMonths = string2number(textBuffer);
	timeInMonths = (t.tm_mon + 1);
	  
	  if (apptInDays - timeInDays == 1) {
				snprintf(date_time_for_appt, 20, "Demain");
				text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
				} else if (apptInDays == timeInDays) {
				      snprintf(date_time_for_appt, 20, "Aujourd'hui");
				// Change lines above for time format, current is days/months
					  text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
			} else {
				      snprintf(date_time_for_appt, 20, "Le %d/%d", apptInDays, apptInMonths);
				// Change lines above for time format, current is days/months
					  text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
				    } 
  } else {
		// First, we assign values
	  
	apptInMinutes = timestr2minutes(appointment_time + 6);
	timeInMinutes = (t.tm_hour * 60) + t.tm_min;
		static char textBuffer[] = "00";
		strncpy(textBuffer, appointment_time + 3,2);
	apptInDays = string2number(textBuffer);
	timeInDays = t.tm_mday;
		strncpy(textBuffer, appointment_time,2);
	apptInMonths = string2number(textBuffer);
	timeInMonths = (t.tm_mon + 1);
	
	
	/* Manage appoitment notification */
	
	if(apptInMinutes >= 0) {
		//if(apptInMinutes < timeInMinutes) {
			//layer_set_hidden(&calendar_layer, 1); 	
		//}
		if ((apptInDays > timeInDays)||(apptInMonths > timeInMonths)) {
			if ((apptInMinutes == 0) && (apptInDays - timeInDays == 1)) { 
			snprintf(date_time_for_appt, 20, "A Minuit");
				// Change lines above for time format, current is days/months
			text_layer_set_text(&calendar_date_layer, date_time_for_appt);; 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
			} else if ((apptInDays - timeInDays == 1) && (((apptInMinutes) % 60) == 0)) {
				snprintf(date_time_for_appt, 20, "Demain, %dh", (int)((apptInMinutes)/ 60));
				text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
            } else if (apptInDays - timeInDays == 1) {
				snprintf(date_time_for_appt, 20, "Demain, %dh %d", (int)((apptInMinutes)/ 60),(int)((apptInMinutes) % 60));
				text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
				layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
            } else {
				      snprintf(date_time_for_appt, 20, "%d/%d %dh %d", apptInDays, apptInMonths, (int)((apptInMinutes)/ 60),
							   (int)((apptInMinutes) % 60));
				// Change lines above for time format, current is days/months
					  text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
					  layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);
			}  	
		}  else if (apptInMinutes == 0) { 
			snprintf(date_time_for_appt, 20, "Aucun");
				// Change lines above for time format, current is days/months
			text_layer_set_text(&calendar_date_layer, date_time_for_appt);; 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
        } else if(timeInMinutes - apptInMinutes == 1) {
			snprintf(date_time_for_appt, 20, "Depuis %d minute", (int)(timeInMinutes - apptInMinutes));
			text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
		} else if((apptInMinutes < timeInMinutes) && (((timeInMinutes - apptInMinutes) / 60) > 0)) {
			snprintf(date_time_for_appt, 20, "Depuis %dh %dmin", 
					 (int)((timeInMinutes - apptInMinutes)/60),(int)((timeInMinutes - apptInMinutes)%60));
			text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
		} else if(apptInMinutes < timeInMinutes) {
			snprintf(date_time_for_appt, 20, "Depuis %d minutes", (int)(timeInMinutes - apptInMinutes));
			text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
		} else if(apptInMinutes > timeInMinutes) {
			if(((apptInMinutes - timeInMinutes) / 60) > 0) {
				snprintf(date_time_for_appt, 20, "Dans %dh %dm", 
						 (int)((apptInMinutes - timeInMinutes) / 60),
						 (int)((apptInMinutes - timeInMinutes) % 60));
			} else {
				snprintf(date_time_for_appt, 20, "Dans %d minutes", (int)(apptInMinutes - timeInMinutes));
			}
			text_layer_set_text(&calendar_date_layer, date_time_for_appt); 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
		}else if(apptInMinutes == timeInMinutes) {
			text_layer_set_text(&calendar_date_layer, "Maintenant!"); 	
			layer_set_hidden(&animated_layer[CALENDAR_LAYER], 0);  	
			vibes_double_pulse();
		} 
		
		//Vibrate if event is in 15 minutes
		if((apptInMinutes >= timeInMinutes) && ((apptInMinutes - timeInMinutes) == 15)) {
			vibes_short_pulse();
		}
	} // Appointment minutes are positive test
  } // Appointment is all day test
}

// End of calendar appointment utilities


AppMessageResult sm_message_out_get(DictionaryIterator **iter_out) {
    AppMessageResult result = app_message_out_get(iter_out);
    if(result != APP_MSG_OK) return result;
    dict_write_int32(*iter_out, SM_SEQUENCE_NUMBER_KEY, ++s_sequence_number);
    if(s_sequence_number == 0xFFFFFFFF) {
        s_sequence_number = 1;
    }
    return APP_MSG_OK;
}

void reset_sequence_number() {
    DictionaryIterator *iter = NULL;
    app_message_out_get(&iter);
    if(!iter) return;
    dict_write_int32(iter, SM_SEQUENCE_NUMBER_KEY, 0xFFFFFFFF);
    app_message_out_send();
    app_message_out_release();
}


char* int_to_str(int num, char *outbuf) {
	int digit, i=0, j=0;
	char buf[STRING_LENGTH];
	bool negative=false;
	
	if (num < 0) {
		negative = true;
		num = -1 * num;
	}
	
	for (i=0; i<STRING_LENGTH; i++) {
		digit = num % 10;
		if ((num==0) && (i>0)) 
			break;
		else
			buf[i] = '0' + digit;
		 
		num/=10;
	}
	
	if (negative)
		buf[i++] = '-';
	
	buf[i--] = '\0';
	
	
	while (i>=0) {
		outbuf[j++] = buf[i--];
	}

	outbuf[j++] = '%';
	outbuf[j] = '\0';
	
	return outbuf;
}


void sendCommand(int key) {
	DictionaryIterator* iterout;
	sm_message_out_get(&iterout);
    if(!iterout) return;
	
	dict_write_int8(iterout, key, -1);
	app_message_out_send();
	app_message_out_release();	
}


void sendCommandInt(int key, int param) {
	DictionaryIterator* iterout;
	sm_message_out_get(&iterout);
    if(!iterout) return;
	
	dict_write_int8(iterout, key, param);
	app_message_out_send();
	app_message_out_release();	
}


void rcv(DictionaryIterator *received, void *context) {
	// Got a message callback
	Tuple *t;


	t=dict_find(received, SM_WEATHER_COND_KEY); 
	if (t!=NULL) {
		memcpy(weather_cond_str, t->value->cstring, strlen(t->value->cstring));
        weather_cond_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(&text_weather_cond_layer, weather_cond_str); 	
	}

	t=dict_find(received, SM_WEATHER_TEMP_KEY); 
	if (t!=NULL) {
		memcpy(weather_temp_str, t->value->cstring, strlen(t->value->cstring));
        weather_temp_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(&text_weather_temp_layer, weather_temp_str); 
		
		layer_set_hidden(&text_weather_cond_layer.layer, true);
		layer_set_hidden(&text_weather_temp_layer.layer, false);
			
	}

	t=dict_find(received, SM_WEATHER_ICON_KEY); 
	if (t!=NULL) {
		bitmap_layer_set_bitmap(&weather_image, &weather_status_imgs[t->value->uint8].bmp);	  	
	}

	t=dict_find(received, SM_COUNT_BATTERY_KEY); 
	if (t!=NULL) {
		batteryPercent = t->value->uint8;
		layer_mark_dirty(&battery_layer);
		text_layer_set_text(&text_battery_layer, int_to_str(batteryPercent, string_buffer) ); 	
	}

	t=dict_find(received, SM_STATUS_CAL_TIME_KEY); 
	if (t!=NULL) {
		memcpy(calendar_date_str, t->value->cstring, strlen(t->value->cstring));
        calendar_date_str[strlen(t->value->cstring)] = '\0';
		//text_layer_set_text(&calendar_date_layer, calendar_date_str); 	
		strncpy(appointment_time, calendar_date_str, 50); //Copy time to appointment_time so that apptDisplay can use it
	}

	t=dict_find(received, SM_STATUS_CAL_TEXT_KEY); 
	if (t!=NULL) {
		memcpy(calendar_text_str, t->value->cstring, strlen(t->value->cstring));
        calendar_text_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(&calendar_text_layer, calendar_text_str); 	
		// Resize Calendar text if needed
		if(strlen(calendar_text_str) <= 15)
			text_layer_set_font(&calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
		else
			if(strlen(calendar_text_str) <= 18)
				text_layer_set_font(&calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
			else 
				text_layer_set_font(&calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	}


	t=dict_find(received, SM_STATUS_MUS_ARTIST_KEY); 
	if (t!=NULL) {
		memcpy(music_artist_str, t->value->cstring, strlen(t->value->cstring));
        music_artist_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(&music_artist_layer, music_artist_str); 	
	}

	t=dict_find(received, SM_STATUS_MUS_TITLE_KEY); 
	if (t!=NULL) {
		memcpy(music_title_str, t->value->cstring, strlen(t->value->cstring));
        music_title_str[strlen(t->value->cstring)] = '\0';
		text_layer_set_text(&music_song_layer, music_title_str); 	
	}

	t=dict_find(received, SM_STATUS_UPD_WEATHER_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		app_timer_cancel_event(g_app_context, timerUpdateWeather);
		timerUpdateWeather = app_timer_send_event(g_app_context, interval /* milliseconds */, 1);
	}

	t=dict_find(received, SM_STATUS_UPD_CAL_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		app_timer_cancel_event(g_app_context, timerUpdateCalendar);
		timerUpdateCalendar = app_timer_send_event(g_app_context, interval /* milliseconds */, 2);
	}

	
	
	t=dict_find(received, SM_PLAY_STATUS_KEY); 
	if (t!=NULL) {
		if (t->value->int32) {
		music_is_playing = true;}
	}
	
	t=dict_find(received, SM_SONG_LENGTH_KEY); 
	if (t!=NULL) {
		int interval = t->value->int32 * 1000;

		app_timer_cancel_event(g_app_context, timerUpdateMusic);
		timerUpdateMusic = app_timer_send_event(g_app_context, interval /* milliseconds */, 3);
	}
	

}

void dropped(void *context, AppMessageResult reason){
	// DO SOMETHING WITH THE DROPPED REASON / DISPLAY AN ERROR / RESEND 
}



void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;


}


void select_up_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

	//revert to showing the temperature 
	layer_set_hidden(&text_weather_temp_layer.layer, false);
	layer_set_hidden(&text_weather_cond_layer.layer, true);

}


void select_down_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

	//show the weather condition instead of temperature while center button is pressed
	layer_set_hidden(&text_weather_temp_layer.layer, true);
	layer_set_hidden(&text_weather_cond_layer.layer, false);

}


void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

	reset();
	
	sendCommandInt(SM_SCREEN_ENTER_KEY, STATUS_SCREEN_APP);


}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
	
	//on a press of the bottom button, scroll in the next layer

	property_animation_init_layer_frame(&ani_out, &animated_layer[active_layer], &GRect(0, 124, 144, 45), &GRect(-144, 124, 144, 45));
	animation_schedule(&(ani_out.animation));


	active_layer = (active_layer + 1) % (NUM_LAYERS);


	property_animation_init_layer_frame(&ani_in, &animated_layer[active_layer], &GRect(144, 124, 144, 45), &GRect(0, 124, 144, 45));
	animation_schedule(&(ani_in.animation));
	
}


void config_provider(ClickConfig **config, Window *window) {
  (void)window;


//  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_SELECT]->raw.up_handler = (ClickHandler) select_up_handler;
  config[BUTTON_ID_SELECT]->raw.down_handler = (ClickHandler) select_down_handler;

//  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;
//  config[BUTTON_ID_SELECT]->long_click.release_handler = (ClickHandler) select_long_release_handler;


  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;
//  config[BUTTON_ID_UP]->long_click.handler = (ClickHandler) up_long_click_handler;
//  config[BUTTON_ID_UP]->long_click.release_handler = (ClickHandler) up_long_release_handler;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
//  config[BUTTON_ID_DOWN]->long_click.handler = (ClickHandler) down_long_click_handler;
//  config[BUTTON_ID_DOWN]->long_click.release_handler = (ClickHandler) down_long_release_handler;

}


void battery_layer_update_callback(Layer *me, GContext* ctx) {
	
	//draw the remaining battery percentage
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_fill_rect(ctx, GRect(2+16-(int)((batteryPercent/100.0)*16.0), 2, (int)((batteryPercent/100.0)*16.0), 8), 0, GCornerNone);
	
}


void handle_status_appear(Window *window)
{
	sendCommandInt(SM_SCREEN_ENTER_KEY, STATUS_SCREEN_APP);
}

void handle_status_disappear(Window *window)
{
	sendCommandInt(SM_SCREEN_EXIT_KEY, STATUS_SCREEN_APP);
	
	app_timer_cancel_event(g_app_context, timerUpdateCalendar);
	app_timer_cancel_event(g_app_context, timerUpdateMusic);
	app_timer_cancel_event(g_app_context, timerUpdateWeather);
	
}

void reset() {
	
	layer_set_hidden(&text_weather_temp_layer.layer, true);
	layer_set_hidden(&text_weather_cond_layer.layer, false);
	text_layer_set_text(&text_weather_cond_layer, "Mise à jour..."); 	
	
}

void handle_init(AppContextRef ctx) {
	(void)ctx;

  g_app_context = ctx;

	window_init(&window, "Window Name");
	window_set_window_handlers(&window, (WindowHandlers) {
	    .appear = (WindowHandler)handle_status_appear,
	    .disappear = (WindowHandler)handle_status_disappear
	});

	window_stack_push(&window, true /* Animated */);
	window_set_fullscreen(&window, true);

	resource_init_current_app(&APP_RESOURCES);


	//init weather images
	for (int i=0; i<NUM_WEATHER_IMAGES; i++) {
		heap_bitmap_init(&weather_status_imgs[i], WEATHER_IMG_IDS[i]);
	}
	
	heap_bitmap_init(&bg_image, RESOURCE_ID_IMAGE_BACKGROUND);


	//init background image
	bitmap_layer_init(&background_image, GRect(0, 0, 144, 168));
	layer_add_child(&window.layer, &background_image.layer);
	bitmap_layer_set_bitmap(&background_image, &bg_image.bmp);


	//init weather layer and add weather image, weather condition, temperature, and battery indicator
	layer_init(&weather_layer, GRect(0, 78, 144, 45));
	layer_add_child(&window.layer, &weather_layer);

	heap_bitmap_init(&battery_image, RESOURCE_ID_IMAGE_BATTERY);

	bitmap_layer_init(&battery_image_layer, GRect(107, 8, 23, 14));
	layer_add_child(&weather_layer, &battery_image_layer.layer);
	bitmap_layer_set_bitmap(&battery_image_layer, &battery_image.bmp);


	text_layer_init(&text_battery_layer, GRect(99, 20, 40, 60));
	text_layer_set_text_alignment(&text_battery_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&text_battery_layer, GColorWhite);
	text_layer_set_background_color(&text_battery_layer, GColorClear);
	text_layer_set_font(&text_battery_layer,  fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(&weather_layer, &text_battery_layer.layer);
	text_layer_set_text(&text_battery_layer, "-");


	layer_init(&battery_layer, GRect(109, 9, 19, 11));
	battery_layer.update_proc = &battery_layer_update_callback;
	layer_add_child(&weather_layer, &battery_layer);

	batteryPercent = 100;
	layer_mark_dirty(&battery_layer);


	text_layer_init(&text_weather_cond_layer, GRect(48, 1, 48, 40)); // GRect(5, 2, 47, 40)
	text_layer_set_text_alignment(&text_weather_cond_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&text_weather_cond_layer, GColorWhite);
	text_layer_set_background_color(&text_weather_cond_layer, GColorClear);
	text_layer_set_font(&text_weather_cond_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(&weather_layer, &text_weather_cond_layer.layer);

	layer_set_hidden(&text_weather_cond_layer.layer, false);
	text_layer_set_text(&text_weather_cond_layer, "Mise à jour..."); 	
	

	weather_img = 0;

	bitmap_layer_init(&weather_image, GRect(5, 2, 40, 40)); // GRect(52, 2, 40, 40)
	layer_add_child(&weather_layer, &weather_image.layer);
	bitmap_layer_set_bitmap(&weather_image, &weather_status_imgs[0].bmp);


	text_layer_init(&text_weather_temp_layer, GRect(48, 3, 48, 40)); // GRect(98, 4, 47, 40)
	text_layer_set_text_alignment(&text_weather_temp_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&text_weather_temp_layer, GColorWhite);
	text_layer_set_background_color(&text_weather_temp_layer, GColorClear);
	text_layer_set_font(&text_weather_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	layer_add_child(&weather_layer, &text_weather_temp_layer.layer);
	text_layer_set_text(&text_weather_temp_layer, "-°"); 	

	layer_set_hidden(&text_weather_temp_layer.layer, true);

	
	//init layers for time and date
	text_layer_init(&text_date_layer, window.layer.frame);
	text_layer_set_text_alignment(&text_date_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&text_date_layer, GColorWhite);
	text_layer_set_background_color(&text_date_layer, GColorClear);
	layer_set_frame(&text_date_layer.layer, GRect(0, 45, 144, 30));
	text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DATE_21)));
	layer_add_child(&window.layer, &text_date_layer.layer);


	text_layer_init(&text_time_layer, window.layer.frame);
	text_layer_set_text_alignment(&text_time_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&text_time_layer, GColorWhite);
	text_layer_set_background_color(&text_time_layer, GColorClear);
	layer_set_frame(&text_time_layer.layer, GRect(0, -5, 144, 50));
	text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HOUR_49)));
	layer_add_child(&window.layer, &text_time_layer.layer);


	//init calendar layer
	layer_init(&animated_layer[CALENDAR_LAYER], GRect(0, 124, 144, 45));
	layer_add_child(&window.layer, &animated_layer[CALENDAR_LAYER]);
	
	text_layer_init(&calendar_date_layer, GRect(6, 0, 132, 21));
	text_layer_set_text_alignment(&calendar_date_layer, GTextAlignmentLeft);
	text_layer_set_text_color(&calendar_date_layer, GColorWhite);
	text_layer_set_background_color(&calendar_date_layer, GColorClear);
	text_layer_set_font(&calendar_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(&animated_layer[CALENDAR_LAYER], &calendar_date_layer.layer);
	text_layer_set_text(&calendar_date_layer, "Aucun"); 	


	text_layer_init(&calendar_text_layer, GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(&calendar_text_layer, GTextAlignmentLeft);
	text_layer_set_text_color(&calendar_text_layer, GColorWhite);
	text_layer_set_background_color(&calendar_text_layer, GColorClear);
	text_layer_set_font(&calendar_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(&animated_layer[CALENDAR_LAYER], &calendar_text_layer.layer);
	text_layer_set_text(&calendar_text_layer, "Rendez-vous");
	
	
	
	
	
	//init music layer
	layer_init(&animated_layer[MUSIC_LAYER], GRect(144, 124, 144, 45));
	layer_add_child(&window.layer, &animated_layer[MUSIC_LAYER]);
	
	text_layer_init(&music_artist_layer, GRect(6, 0, 132, 21));
	text_layer_set_text_alignment(&music_artist_layer, GTextAlignmentLeft);
	text_layer_set_text_color(&music_artist_layer, GColorWhite);
	text_layer_set_background_color(&music_artist_layer, GColorClear);
	text_layer_set_font(&music_artist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	layer_add_child(&animated_layer[MUSIC_LAYER], &music_artist_layer.layer);
	text_layer_set_text(&music_artist_layer, "Musique"); 	


	text_layer_init(&music_song_layer, GRect(6, 15, 132, 28));
	text_layer_set_text_alignment(&music_song_layer, GTextAlignmentLeft);
	text_layer_set_text_color(&music_song_layer, GColorWhite);
	text_layer_set_background_color(&music_song_layer, GColorClear);
	text_layer_set_font(&music_song_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(&animated_layer[MUSIC_LAYER], &music_song_layer.layer);
	text_layer_set_text(&music_song_layer, "Aucune lecture en cours");
	


	window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

	active_layer = CALENDAR_LAYER;

	reset();
}

void switchLayer () { //to use when music plays
property_animation_init_layer_frame(&ani_out, &animated_layer[active_layer], &GRect(0, 124, 144, 45), &GRect(-144, 124, 144, 45));
	animation_schedule(&(ani_out.animation));


	active_layer = (active_layer + 1) % (NUM_LAYERS);


	property_animation_init_layer_frame(&ani_in, &animated_layer[active_layer], &GRect(144, 124, 144, 45), &GRect(0, 124, 144, 45));
	animation_schedule(&(ani_in.animation));
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
/* Display the time */
  (void)ctx;

  
PblTm time;    //Structure to store time info
get_time(&time);    //Fill the structure with current time
 
int seconds = time.tm_sec;    //Get the current number of seconds
 
	// Display music if any is playing
	if( ((seconds % 10) == 5) && music_is_playing /**(strcmp(text_layer_get_text(&music_song_layer),"") != 0)**/ ) {
		switchLayer();
	} else if ( ((seconds % 10) == 0) && (active_layer == MUSIC_LAYER) ) {
		switchLayer();
	}

// EXECUTE THE FOLLOWING ONLY ONCE PER MINUTE
	if ((seconds == 0) || (!Watch_Face_Initialized)) {	
	
		Watch_Face_Initialized = true;
static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  // Check 24h mode to now how to handle date
  if (clock_is_24h_style()) {
   string_format_time(date_text, sizeof(date_text), "%a %e %b", t->tick_time); //EU mode
   text_layer_set_text(&text_date_layer, date_text);
  } else {
   string_format_time(date_text, sizeof(date_text), "%a, %b %e", t->tick_time);
   text_layer_set_text(&text_date_layer, date_text);
  }
	
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
			
			
			

  // Check 24h mode for hour
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  string_format_time(time_text, sizeof(time_text), time_format, t->tick_time);

  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(&text_time_layer, time_text);
		apptDisplay(); }

	int heure = t->tick_time->tm_hour;
#if VIBE_ON_HOUR
  if (((t->units_changed & HOUR_UNIT) == HOUR_UNIT) && ((heure > 9) && (heure < 23))) {
    vibes_double_pulse();
  }
#endif
	
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

	for (int i=0; i<NUM_WEATHER_IMAGES; i++) {
	  	heap_bitmap_deinit(&weather_status_imgs[i]);
	}

  	heap_bitmap_deinit(&bg_image);

	
}


void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

/* Request new data from the phone once the timers expire */

	if (cookie == 1) {
		sendCommand(SM_STATUS_UPD_WEATHER_KEY);	
	}

	if (cookie == 2) {
		sendCommand(SM_STATUS_UPD_CAL_KEY);	
	}

	if (cookie == 3) {
		sendCommand(SM_SONG_LENGTH_KEY);	
	}

	
		
}

void pbl_main(void *params) {

  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
	.messaging_info = {
		.buffer_sizes = {
			.inbound = 124,
			.outbound = 256
		},
		.default_callbacks.callbacks = {
			.in_received = rcv,
			.in_dropped = dropped
		}
	},
	.tick_info = {
	  .tick_handler = &handle_minute_tick,
	  .tick_units = SECOND_UNIT //Second for debugging
	},
    .timer_handler = &handle_timer,

  };
  app_event_loop(params, &handlers);
}