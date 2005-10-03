/***************************************************************************
                          xmondobackup.cpp  -  description
                             -------------------
    begin                : Mon Apr 28 2003
    copyright            : (C) 2003 by Joshua Oreman
    email                : oremanj@get-linux.org
    cvsid                : $Id: xmondobackup.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "xmondobackup.h"
#include "X-specific.h"
extern "C" {
#include "libmondo-archive-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-verify-EXT.h"
}
#include <qprogressbar.h>
#include <qmultilineedit.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <qlayout.h>
#include <qthread.h>
#include <qpushbutton.h>

static char cvsid[] = "$Id: xmondobackup.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $";

extern "C" {
    char g_xmondo_logfile[MAX_STR_LEN] = "/var/log/mondo-archive.log";
}

extern QProgressBar            *XMondoProgress;
extern QLabel                  *XMondoProgressWhat, *XMondoProgressWhat2, *XMondoProgressWhat3;
extern QCheckBox               *XMondoVerbose;
extern QMultiLineEdit          *XMondoLog;
extern QLabel                  *XMondoStatus;
extern QLabel                  *XMondoTimeTaken, *XMondoTimeToGo, *XMondoProgressPercent;
extern QPushButton             *XMondoCancel;
extern XMEventHolder            events;

XMondoBackup::XMondoBackup() : QObject (0, 0)
{

    disconnect (XMondoCancel, SIGNAL(clicked()), 0, 0);
    connect (XMondoCancel, SIGNAL(clicked()), this, SLOT(abortBackup()));

    XMondoProgress->hide();
    XMondoProgressWhat->hide();
    XMondoProgressWhat2->hide();
    XMondoProgressWhat3->hide();
    XMondoTimeTaken->hide();
    XMondoTimeToGo->hide();
    XMondoStatus->setText ("");
    XMondoStatus->show();
    XMondoLog->setText ("");
    XMondoLog->show();
}

XMondoBackup::~XMondoBackup()
{
}

void *BackupThread__run (void *arg);

class BackupThread
{
    friend void *BackupThread__run (void *arg);
public:
    BackupThread (struct s_bkpinfo *bkpinfo, bool bkup = true) : _bkpinfo (bkpinfo), _bkup (bkup),
    _ret (-1), _aborted (false) {}
    int returns() {
	return _ret;
    }
    void start() {
	_running = true;
	pthread_create (&_thr, 0, BackupThread__run, this);
    }
    void terminate() {
	_running = false;
	pthread_cancel (_thr);
	_thr = 0;
    }
    bool running() {
	return _running;
    }
    bool aborted() {
	return _aborted;
    }
protected:
    static void setStop (void *arg) {
	BackupThread *bt = static_cast <BackupThread*> (arg);
	if (!bt) return;
	bt->_running = false;
	bt->_ret = -1;
	bt->_aborted = true;
    }
    void run() {
	pthread_cleanup_push (BackupThread::setStop, this);
	if (_bkup) {
	    log_to_screen ("Backup started on %s", call_program_and_get_last_line_of_output ("date"));
	    _ret = backup_data (_bkpinfo);
	    log_to_screen ("Backup finished on %s", call_program_and_get_last_line_of_output ("date"));
	} else {
	    log_to_screen ("Compare started on %s", call_program_and_get_last_line_of_output ("date"));
	    _ret = verify_data (_bkpinfo);
	    log_to_screen ("Compare finished on %s", call_program_and_get_last_line_of_output ("date"));
	}
	pthread_cleanup_pop (FALSE);
	_running = false;
    }
    struct s_bkpinfo *_bkpinfo;
    int _ret;
    bool _bkup;
    pthread_t _thr;
    bool _running;
    bool _aborted;
};

void *BackupThread__run (void *arg)
{
    BackupThread *bt = static_cast <BackupThread *> (arg);
    if (!bt && (sizeof(void*) >= 4)) return (void *) 0xDEADBEEF;
    bt->run();
}

int XMondoBackup::run (struct s_bkpinfo *bkpinfo)
{
    th = new BackupThread (bkpinfo);
    th->start();
    while (th->running()) {
			usleep (100000);
			events.send();
			kapp->processEvents();
    }
    if (th->aborted()) {
	return -1;
    }
    return th->returns();
}

int XMondoBackup::compare (struct s_bkpinfo *bkpinfo)
{
    th = new BackupThread (bkpinfo, 0);
    th->start();
    while (th->running()) {
			usleep (100000);
			events.send();
			kapp->processEvents();
    }
    if (th->aborted()) {
	return -1;
    }
    return th->returns();
}

void XMondoBackup::abortBackup() 
{
    th->terminate();
}
