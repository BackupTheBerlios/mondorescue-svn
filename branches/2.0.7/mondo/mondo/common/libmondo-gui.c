/* libmondo-gui.c
 * $Id$
 */

/**
 * @file
 * Chooses which GUI file to compile based on #defines.
 * @see newt-specific.c
 */

#ifdef _XWIN
#include "X-specific.c"
#else
#include "newt-specific.c"
#endif
