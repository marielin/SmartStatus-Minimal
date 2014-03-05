/*
	This file may be used for translations. Don't hesitate contacting me if you need help.
	© Alexandre Jouandin — SmartFrenchIze

	Please keep the layout as it is now, and only change the strings, not the names
*/

// Lists of translations
          // Translation for DAYS goes here, starting on SUNDAY. Up to 3 letters per word 
static const char *day_of_week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

          // Translation for MONTHS goes here. They can have up to 4 letters per word
static const char *month_of_year[] = { "Jan", "Febr", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
		  // This is the number of days per month. I don't think this has to be changed :) 
static const int days_per_month [12] = {31,28,31,30,31,30,31,31,30,31,30,31};
		  // This is the header that appears when days are not far away from now. 
		  // 		E.g: If the event is in one day, this will display "Tomorrow"
static const char *days_from_today[] = { "Tomorrow", "In 2 days", "In 3 days", "In 4 days" };


// These are the main translation strings
// Change the "Translation" Column

//		NAME of the strings 		 Translation
#define STRING_UPDATING				"Updating..."
#define STRING_CONNECTED 			"Connected"
#define STRING_DISCONNECTED			"Disconnected"
#define STRING_DEFAULT_CALENDAR_1	"No"
#define STRING_DEFAULT_CALENDAR_2	"Appointment"
#define STRING_DEFAULT_MUSIC_1		"Nothing is"
#define STRING_DEFAULT_MUSIC_2		"Playing"
#define STRING_FIND_IPHONE_CONF_1	"Find your"
#define STRING_FIND_IPHONE_CONF_2	"iPhone?"

// Calendar STRINGS. 
/* WARNING! YOU NEED TO KEEP THE ORDER AND NUMBER of the variables like "%i" (for int), "%02i" (for two digits int), "%s" for string etc... 
	If your display style cannot fit this pattern, please contact me first, and i'll adapt it
*/

	/* This list is used to differentiate between "In 3 hours" (the event has not started yet) 
		and "For 2 hours" (the event has been going for 2 hours already).
		It's used in the STRING_EVENT_HOUR, STRING_EVENT_HOURS, STRING_EVENT_MINUTE, STRING_EVENT_MINUTES strings
	*/
static const char *before_after[] = {"For", "In"};

//		NAME of the strings 		English Translation 
#define STRING_EVENT_IS_PAST		"Started %s %i"		
#define STRING_EVENT_FUTURE_GLOBAL	"%s %i at %i:%02i"	
#define STRING_EVENT_FUTURE_SOON	"%s, at %i:%02i"	
#define STRING_NOW					"Now!"				
#define STRING_EVENT_HOUR			"%s 1 hour"			
#define STRING_EVENT_HOURS			"%s %i hours"		
#define STRING_EVENT_MINUTE			"%s 1 minute"		
#define STRING_EVENT_MINUTES		"%s %i minutes"		
#define STRING_EVENT_MIXED			"%s %ih %i min"		