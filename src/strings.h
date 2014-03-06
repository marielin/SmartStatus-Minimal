/*
	This file may be used for translations. Don't hesitate contacting me if you need help.
	© Alexandre Jouandin — SmartFrenchIze

	Please keep the layout as it is now, and only change the strings, not the names
*/

// Lists of translations
          // Translation for DAYS goes here, starting on SUNDAY. Up to 3 letters per word 
static const char *day_of_week[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

          // Translation for MONTHS goes here. They can have up to 4 letters per word
static const char *month_of_year[] = { "Janv", "Fevr", "Mars", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"};
		  // This is the number of days per month. I don't think this has to be changed :) 
static const int days_per_month [12] = {31,28,31,30,31,30,31,31,30,31,30,31};
		  // This is the header that appears when days are not far away from now. 
		  // 		E.g: If the event is in one day, this will display "Tomorrow"
static const char *days_from_today[] = { "Demain", "Après-demain", "Dans 3 jours", "Dans 4 jours" };


// These are the main translation strings
// Change the "Translation" Column

//		NAME of the strings 		 Translation 			   English Translation
#define STRING_UPDATING				"Mise à jour..."		// "Updating..."
#define STRING_CONNECTED 			"Connecté"				// "Connected"
#define STRING_DISCONNECTED			"Déconnecté"			// "Disconnected"
#define STRING_DEFAULT_CALENDAR_1	"Aucun"					// "No"
#define STRING_DEFAULT_CALENDAR_2	"Rendez-Vous"			// "Appointment"
#define STRING_DEFAULT_MUSIC_1		"Aucune lecture"		// "Nothing"
#define STRING_DEFAULT_MUSIC_2		"En cours"				// "Playing"
#define STRING_FIND_IPHONE_CONF_1	"Faire sonner"			// "Find your"
#define STRING_FIND_IPHONE_CONF_2	"l'iPhone ?"			// "iPhone?"

// Calendar STRINGS. 
/* WARNING! YOU NEED TO KEEP THE ORDER AND NUMBER of the variables like "%i" (for int), "%02i" (for two digits int), "%s" for string etc... 
	If your display style cannot fit this pattern, please contact me first, and i'll adapt it
*/

	/* This list is used to differentiate between "In 3 hours" (the event has not started yet) 
		and "For 2 hours" (the event has been going for 2 hours already).
		It's used in the STRING_EVENT_HOUR, STRING_EVENT_HOURS, STRING_EVENT_MINUTE, STRING_EVENT_MINUTES strings
	*/
static const char *before_after[] = {"Depuis", "Dans"};

//		NAME of the strings 		 Translation 			   English Translation 		   E.g of Diplay (it sounds bad in DAY MONTH frormat)
#define STRING_EVENT_IS_PAST		"Depuis le %i %s"		// "Since %i %s"			// "Since 1 Nov"
#define STRING_EVENT_FUTURE_GLOBAL	"Le %i %s à %ih%02i"	// "%i %s at %i:%02i"		// "1 Nov at 08:00"
#define STRING_EVENT_FUTURE_SOON	"%s, à %ih%02i"			// "%s, at %i:%02i"			// "Tomorrow, at 9:25"
#define STRING_NOW					"Maintenant!"			// "Now!"					// "Now!"
#define STRING_EVENT_HOUR			"%s 1 heure"			// "%s 1 hour"				// "In 1 hour"
#define STRING_EVENT_HOURS			"%s %i heures"			// "%s %i hours"			// "In 3 hours"
#define STRING_EVENT_MINUTE			"%s 1 minute"			// "%s 1 minute"			// "In 1 minute"
#define STRING_EVENT_MINUTES		"%s %i minutes"			// "%s %i minutes"			// "In 48 minutes"
#define STRING_EVENT_MIXED			"%s %ih %i"				// "%s %ih %i min"			// "In 1h 23 min"