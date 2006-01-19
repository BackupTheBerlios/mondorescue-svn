/***************************************************************************
                          xmondo.h  -  description
                             -------------------
    begin                : Thu Apr 24 19:44:32 PDT 2003
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

#ifndef XMONDO_H
#define XMONDO_H

#undef scroll					// newt weirdness

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <kmainwindow.h>
#include <kmenubar.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qwidgetstack.h>
#include <qtabwidget.h>
#include "xmondobackup.h"

extern "C" {
#undef DEBUG
#define bool int
#include "my-stuff.h"
#include "mondostructures.h"
#undef bool
} class QVButtonGroup;
class QLabel;
class QLineEdit;
class QListView;
class QCheckBox;

class XMondoBackup;
class XMondoRestore;

struct s_bkpinfo;

/** XMondo is the base class of the project */
class XMondo:public KMainWindow {
  Q_OBJECT public:
	/** construtor */
	XMondo(QWidget * parent = 0, const char *name = 0);
	/** destructor */
	 virtual ~ XMondo();

	void fillBkpinfo(struct s_bkpinfo *);

	public slots:void slotVerboseChange(bool v);
	void slotMode(int x);
	void slotMediaType(int x);
	void slotCompareMediaType(int x);
	void slotRestoreMediaType(int x);
	void slotStartBackup();
	void slotStartCompare();
	void slotAddInclude();
	void slotAddExclude();
	void slotDelInclude();
	void slotDelExclude();
	void slotTabChange(QWidget *);
	void slotPrevRestore();
	void slotNextRestore();

  private:
	void initBackupTab(QWidget * &);
	void initOptionsTab(QWidget * &);
	void initAdvancedTab(QWidget * &);
	void initCompareTab(QWidget * &);
	void initRestoreTab(QWidget * &);

	QVButtonGroup *buttons;
	QWidgetStack *stack, *restoreStack;
	QTabWidget *backup;
	QWidget *compare, *restore, *tabHardware, *tabOptions, *tabAdvanced,
		*restoreInfo, *restoreFiles;
	QPushButton *bStartBackup, *bStartCompare;
	QPushButton *rPrev, *rNext;
	QButtonGroup *bgMediaType, *bgCompression, *bgCompareMediaType,
		*bgCompareCompression, *bgBootLoader, *mainButtons,
		*bgRestoreMediaType;
	QLabel *lDOption, *compareLDOption, *nfsRemoteDir,
		*compareNFSRemoteDir, *restoreLDOption, *restoreNFSRemoteDir;
	QLineEdit *editDOption, *editMediaSize, *compareEditDOption,
		*compareEditMediaSize, *restoreEditDOption, *restoreEditMediaSize,
		*editBootDevice, *editKernel, *editNFSRemoteDir,
		*compareEditNFSRemoteDir, *restoreEditNFSRemoteDir, *restoreFilter;
	QListView *listImageDevs, *listInclude, *listExclude, *listExcludeDevs;
	QCheckBox *checkDifferential, *checkBackupNFS, *checkCompare,
		*checkMakeBootFloppies, *checkUseLilo;
	QLineEdit *pendingInclude, *pendingExclude;
	XMondoBackup *bkup;
	XMondoRestore *rstr;
};

#endif
