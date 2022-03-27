#ifndef _INC_TYPES
#define _INC_TYPES

#include <stdbool.h>
struct station { 
		const char ssid[65]; 
		const char password[65]; 
		const bool dhcp;
};

#endif
