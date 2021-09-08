// Copyright 2021 by Daniel Winkelman. All rights reserved.

/**
 * Macro for defining const strings so that they are stored efficiently in
 * memory (e.g. as PROGMEM in AVR).
 */
#ifdef TARGET_AVR
#define STRING(name, contents) const char #name[] PROGMEM = #contents
#else
#define STRING(name, contents) const char *name = contents
#endif
