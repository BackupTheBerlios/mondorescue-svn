/***************************************************************************
                          main.cpp  -  main file for XMondo
                             -------------------
    begin                : Thu Apr 24 19:44:32 PDT 2003
    copyright            : (C) 2003 by Joshua Oreman
    email                : oremanj@get-linux.org
    cvsid                : $Id: main.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

extern "C" {
#include <libmondo-files-EXT.h>
#include <libmondo-tools-EXT.h>
    int g_ISO_restore_mode = 0;
    extern int g_main_pid;
}

#include "xmondo.h"
#include "xmondobackup.h"
#include <X-specific.h>

static char cvsid[] = "$Id: main.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $";
static const char *description = I18N_NOOP("XMondo");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE	

static KCmdLineOptions options[] =
{
    { 0, 0, 0 }
    // If you want automated backup, you run command-line Mondo. Period.
};

static void unregister_pid() 
{
    register_pid (0, "mondo");
    register_pid (0, "xmondo");
}


int main(int argc, char *argv[])
{    
    KAboutData aboutData( "xmondo", I18N_NOOP("XMondo"),
			  VERSION, description, KAboutData::License_GPL,
			  "(c) 2003, Joshua Oreman", 0, 0, "oremanj@get-linux.org");
    aboutData.addAuthor("Joshua Oreman",0, "oremanj@get-linux.org");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
    
    KApplication a;

    g_main_pid = getpid();
    register_pid (g_main_pid, "mondo");
    register_pid (g_main_pid, "xmondo");
    
    atexit (unregister_pid);
    
    malloc_libmondo_global_strings();

    if (getuid() != 0) {
	popup_and_OK ("Please run as root.");
	exit (1);
    }

    char path[4096];
    strncpy (path, getenv ("PATH"), 4050);
    strcat (path, ":/sbin:/usr/sbin:/usr/local/sbin");
    setenv ("PATH", path, TRUE);

    XMondo *xmondo = new XMondo();
    a.setMainWidget(xmondo);
    xmondo->show();  
    
    return a.exec();
}
