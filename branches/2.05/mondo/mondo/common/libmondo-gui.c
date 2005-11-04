/* libmondo-gui.c
 * $Id: libmondo-gui.c,v 1.2 2004/06/10 15:29:12 hugo Exp $
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






