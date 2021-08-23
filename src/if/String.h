// Copyright 2021 by Daniel Winkelman. All rights reserved.

#ifdef TARGET_AVR
#define STRING(name, contents) const char #name[] PROGMEM = #contents
#else
#define STRING(name, contents) const char *name = contents
#endif
