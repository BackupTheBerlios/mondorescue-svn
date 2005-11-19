/*********************************-*- C++ -*-*******************************
                          xmondorestore.h - restore functions
                          -----------------------------------
    begin                : Sun Nov 22 2003
    copyright            : (C) 2003 by Joshua Oreman
    email                : oremanj@get-linux.org
    cvsid                : $Id$
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef XMONDORESTORE_H
#define XMONDORESTORE_H

#include "xmondo.h"

class QButtonGroup;
class QLineEdit;
class QLabel;
class QListView;

class RestoreThread;
/**
 * The class that handles all the restore functions.
 * @author Joshua Oreman
 */
class XMondoRestore:public QObject {
	Q_OBJECT friend void *XMondoRestore_preparer_thread(void *arg);

  public:
	 XMondoRestore(QWidget * parent, QButtonGroup * mediaType,
				   QLineEdit * device, QLineEdit * nfsRemoteDir,
				   QLineEdit * filelistFilter);
	 virtual ~ XMondoRestore();

	virtual void go();
	bool good() {
		return ok;
	}
	bool isSetupDone() {
		return doneSetup;
	}

	public slots:void slotAbortRestore();

  protected:
	bool ok;
	bool doneSetup;

	QButtonGroup *rMediaType;
	QLineEdit *rDevice, *rNFSRemoteDir, *rFilter;

	QWidget *files;
	QLabel *fStatusMsg;
	QListView *fList;
	QLabel *fRestoreDirLabel;
	QLineEdit *fRestoreDir;

	QString tempdir, filelistLocation, cfgLocation, cdMountpoint;

	pthread_t preparer_thread;

	s_bkpinfo *bkpinfo;

	RestoreThread *th;
};

#endif
