/***************************************************************************
                          xmondo.cpp  -  description
                             -------------------
    begin                : Thu Apr 24 19:44:32 PDT 2003
    copyright            : (C) 2003 by Joshua Oreman
    email                : oremanj@get-linux.org
    cvsid                : $Id: xmondo.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "xmondo.h"
#include "xmondobackup.h"
#include "xmondorestore.h"
#include <config.h>
#include <qwidgetstack.h>
#include <qpushbutton.h>
#include <qsize.h>
#include <qgrid.h>
#include <qlayout.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qthread.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qsizepolicy.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qfile.h>
#include <qtextstream.h>
#include <kaboutdialog.h>
#include <kapp.h>

#include "X-specific.h"
extern "C" {
#define bool int
#include "libmondo-files-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-devices-EXT.h"
#include "libmondo-string-EXT.h"
#include "libmondo-tools-EXT.h"
#undef bool
}

int g_operation_in_progress = 0;

static char cvsid[] = "$Id: xmondo.cpp,v 1.1 2004/06/10 16:13:06 hugo Exp $";

QProgressBar            *XMondoProgress;
QLabel                  *XMondoProgressWhat, *XMondoProgressWhat2, *XMondoProgressWhat3;
QCheckBox               *XMondoVerbose;
QMultiLineEdit          *XMondoLog;
QLabel                  *XMondoStatus;
QLabel                  *XMondoTimeTaken, *XMondoTimeToGo, *XMondoProgressPercent;
QPushButton             *XMondoCancel;
XMEventHolder            events;

/* log to both the X window and the logfile */
void xlog_debug_msg (int debug_level, const char *szFile,
		     const char *szFunction, int nLine, const char *fmt, ...)
{
    char buf[MAX_STR_LEN];

    va_list ap;
    va_start (ap, fmt);
    vsprintf (buf, fmt, ap);
    va_end (ap);

    log_to_screen (buf);
    standard_log_debug_msg (debug_level, szFile, szFunction, nLine, buf);
}

XMondo::XMondo(QWidget *parent, const char *name) : KMainWindow(parent, name)
{
    if (does_file_exist (MONDO_LOGFILE)) {
	unlink (MONDO_LOGFILE);
    }
    log_it ("+----------------- XMondo v%-8s -----------+", VERSION);
    log_it ("| DON'T PANIC! Mondo logs almost everything, so|");
    log_it ("| don't start worrying just because you see a  |");
    log_it ("| few error messages in the log. Mondo will    |");
    log_it ("| tell you if something really has gone wrong, |");
    log_it ("| so if it says everything is all right, it    |");
    log_it ("| probably is. But remember to compare your    |");
    log_it ("| backups before trusting them.        -- Josh |");
    log_it ("+----------------------------------------------+");

    QBoxLayout *mainHBox = new QHBoxLayout (this, 5, 5, "mainHBox");
    buttons = new QVButtonGroup (this, "buttons");
    QPushButton *bBackup, *bCompare, *bRestore, *bAbout, *bQuit;
    bBackup  = new QPushButton ("Backup", buttons, "bBackup");
    bCompare = new QPushButton ("Compare", buttons, "bCompare");
    bRestore = new QPushButton ("Restore", buttons, "bRestore");
    bAbout   = new QPushButton ("About", buttons, "bAbout");
    bQuit    = new QPushButton ("Quit", buttons, "bQuit");
    mainButtons = buttons;
    connect (buttons, SIGNAL(clicked(int)), this, SLOT(slotMode(int)));
    buttons->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Expanding));
    mainHBox->addWidget (buttons);
		
    stack = new QWidgetStack (this, "stack");
    mainHBox->addWidget (stack);

    QWidget *backupPane = new QWidget (stack, "backupPane");
    QBoxLayout *backupVBox = new QVBoxLayout (backupPane, 5, 5, "backupVBox");
    backup = new QTabWidget (backupPane, "backup");
    backupVBox->addWidget (backup);
    bStartBackup = new QPushButton ("START", backupPane, "startBackup");
    connect (bStartBackup, SIGNAL(clicked()), this, SLOT(slotStartBackup()));
    backupVBox->addWidget (bStartBackup);
    QWidget *bBkpinfoTab, *bOptionsTab, *bAdvancedTab;

    initBackupTab (bBkpinfoTab);
    tabHardware = bBkpinfoTab;
    initOptionsTab (bOptionsTab);
    tabOptions = bOptionsTab;
    initAdvancedTab (bAdvancedTab);
    tabAdvanced = bAdvancedTab;
    
    backup->addTab (bBkpinfoTab, "Hardware");
    backup->addTab (bOptionsTab, "Options");
    backup->addTab (bAdvancedTab, "Advanced");
    connect (backup, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotTabChange(QWidget*)));
    stack->addWidget (backupPane, 0);
    
    compare = new QWidget (stack, "compare");
    initCompareTab (compare);
    stack->addWidget (compare, 1);

    restore = new QWidget (stack, "restore");
    QBoxLayout *restoreBox = new QVBoxLayout (restore, 0, 0, "restoreBox");
    restoreStack = new QWidgetStack (restore, "restoreStack");
    restoreInfo = new QWidget (restoreStack, "restoreInfo");
    initRestoreTab (restoreInfo);
    QWidget *restoreButtonsWidget = new QWidget (restore, "restoreButtonsWidget");
    restoreButtonsWidget->setSizePolicy (QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed));
    QBoxLayout *restoreButtons = new QHBoxLayout (restoreButtonsWidget, 5, 5, "restoreButtons");
    rPrev   = new QPushButton ("<- Prev", restoreButtonsWidget, "rPrev");
    rNext   = new QPushButton ("Next ->", restoreButtonsWidget, "rNext");
    rPrev->hide();
    rNext->show();
    connect (rPrev, SIGNAL(clicked()), this, SLOT(slotPrevRestore()));
    connect (rNext, SIGNAL(clicked()), this, SLOT(slotNextRestore()));
    restoreButtons->addWidget (rPrev);
    restoreButtons->addWidget (rNext);
    restoreBox->addWidget (restoreStack);
    restoreBox->addWidget (restoreButtonsWidget);
    restoreStack->addWidget (restoreInfo, 0);
    restoreStack->raiseWidget (0);

    stack->addWidget (restore, 2);
    
    QWidget *progress   = new QWidget (stack, "progress");
    QGridLayout *grid   = new QGridLayout (progress, 9, 2, 5, 5);
    XMondoProgress      = new QProgressBar (progress);
    XMondoProgressWhat  = new QLabel (progress);
    XMondoProgressWhat2 = new QLabel (progress);
    XMondoProgressWhat3 = new QLabel (progress);
    XMondoVerbose       = new QCheckBox ("Verbose", progress);
    XMondoLog           = new QMultiLineEdit (progress);
    XMondoStatus        = new QLabel (progress);
    XMondoTimeTaken     = new QLabel (progress);
    XMondoTimeToGo      = new QLabel (progress);
    XMondoCancel        = new QPushButton ("Cancel", progress, "bCancel");    
    grid->addMultiCellWidget (XMondoStatus,        0, 0, 0, 1);
    grid->addMultiCellWidget (XMondoVerbose,       1, 1, 0, 1, AlignRight);
    grid->addMultiCellWidget (XMondoLog,           2, 2, 0, 1);
    grid->addMultiCellWidget (XMondoProgressWhat,  3, 3, 0, 1);
    grid->addMultiCellWidget (XMondoProgressWhat2, 4, 4, 0, 1);
    grid->addMultiCellWidget (XMondoProgressWhat3, 5, 5, 0, 1);
    grid->addMultiCellWidget (XMondoProgress,      6, 6, 0, 1);
    grid->addWidget          (XMondoTimeTaken,        7,    0);
    grid->addWidget          (XMondoTimeToGo,         7,    1);
    grid->addMultiCellWidget (XMondoCancel,        8, 8, 0, 1);

    connect (XMondoVerbose, SIGNAL(toggled(bool)), this, SLOT(slotVerboseChange(bool)));
    slotVerboseChange (false);

    stack->addWidget (progress, 3);
    stack->raiseWidget (0);
}

XMondo::~XMondo()
{
    if (rstr) delete rstr;
}

void XMondo::slotVerboseChange (bool on) 
{
    if (on) {
	log_debug_msg = xlog_debug_msg;
    } else {
	log_debug_msg = standard_log_debug_msg;
    }
}

void XMondo::slotMode (int x)
{
    if (x < 3) {
	stack->raiseWidget (x);
	return;
    }

    if (x >= 4) {
	kapp->quit();
	return;
    }

    KAboutDialog *about;

    about = new KAboutDialog (0, "about");
    about->setCaption ("About XMondo");
    about->setVersion ("Version 0.1");
    QPixmap logo;
    if (logo.load ("mondo.png")) {
	about->setLogo (logo);
    } else {
	char logo_location[MAX_STR_LEN];

	if (find_and_store_mondoarchives_home (logo_location) == 0) {
	    strcat (logo_location, "/mondo.png");
	    if (logo.load (logo_location)) {
		about->setLogo (logo);
	    } else {
		qDebug ("Warning - logo not found in Mondo's home directory");
	    }
	} else {
	    qDebug ("Warning - can't find & store Mondo's home directory");
	}
    }

    about->setMaintainer ("Joshua Oreman", "oremanj@get-linux.org", "", "Author of XMondo");
    about->setAuthor ("Hugo Rabson", "hugo.rabson@mondorescue.org", "http://www.mondorescue.org/", "Author of Mondo and Mindi");
    about->addContributor ("Stan Benoit", "troff@nakedsoul.org", "http://www.nakedsoul.org/~troff/", "Beta testing & bugfixes");
    about->addContributor ("Jesse Keating", "hosting@j2solutions.net", "http://geek.j2solutions.net/", "Package maintainer");
    about->addContributor ("Hector Alvarez", "hector@debian.org", "", "Debian package maintainer");
    about->exec();
}

void doMediaType (int x, QLabel *lDOption, QLineEdit *editDOption, QLineEdit *editMediaSize,
		  QLabel *nfsRemoteDir, QLineEdit *editNFSRemoteDir)
{
    char cdrw_dev [100], cdrom_dev [100];
    if (find_cdrw_device (cdrw_dev)) strcpy (cdrw_dev, "0,0,0");
    if (find_cdrom_device (cdrom_dev, 0)) strcpy (cdrom_dev, "");

    switch (x) {
    case 0:
	lDOption->setText ("CD-R node:");
	editDOption->setText (cdrw_dev);
	if (editMediaSize) {
	    editMediaSize->setText ("650");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 1:
	lDOption->setText ("CD-RW node:");
	editDOption->setText (cdrw_dev);
	if (editMediaSize) {
	    editMediaSize->setText ("650");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 2:
	lDOption->setText ("CD streamer node:");
	editDOption->setText (cdrw_dev);
	if (editMediaSize) {
	    editMediaSize->setText ("650");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 3:
	lDOption->setText ("DVD /dev entry:");
	editDOption->setText (cdrom_dev);
	if (editMediaSize) {
	    editMediaSize->setText ("4480");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 4:
	lDOption->setText ("Directory for ISOs:");
	editDOption->setText ("/root/images/mondo");
	if (editMediaSize) {
	    editMediaSize->setText ("650");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 5:
	lDOption->setText ("NFS share:");
	editDOption->setText ("");
	if (editMediaSize) {
	    editMediaSize->setText ("4600");
	    editMediaSize->setEnabled (true);
	}
	nfsRemoteDir->setEnabled (true);
	editNFSRemoteDir->setEnabled (true);
	editNFSRemoteDir->setText ("");
	break;
    case 6:
	lDOption->setText ("Tape /dev entry:");
#if __FreeBSD__
	editDOption->setText ("/dev/sa0");
#elif linux
	editDOption->setText ("/dev/st0");
#else
	editDOption->setText ("");
#endif
	if (editMediaSize) {
	    editMediaSize->setText ("n/a");
	    editMediaSize->setEnabled (false);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    case 7:
	lDOption->setText ("Device/file to use:");
	editDOption->setText ("");
	if (editMediaSize) {
	    editMediaSize->setText ("n/a");
	    editMediaSize->setEnabled (false);
	}
	nfsRemoteDir->setEnabled (false);
	editNFSRemoteDir->setEnabled (false);
	break;
    }
}

void XMondo::slotMediaType (int x)
{
    doMediaType (x, lDOption, editDOption, editMediaSize, nfsRemoteDir, editNFSRemoteDir);
}

void XMondo::slotCompareMediaType (int x)
{
    doMediaType (x, compareLDOption, compareEditDOption, 0, compareNFSRemoteDir, compareEditNFSRemoteDir);
}

void XMondo::slotRestoreMediaType (int x)
{
    char cdrom_device[256];
    if (does_file_exist ("/dev/cdroms/cdrom0")) {
	strcpy (cdrom_device, "/dev/cdroms/cdrom0");
    } else if (does_file_exist ("/dev/cdrom")) {
	strcpy (cdrom_device, "/dev/cdrom");
    } else {
	find_cdrom_device (cdrom_device, false);
    }

    switch (x) {
    case 0:
	restoreLDOption->setText ("CD device:");
	restoreEditDOption->setText (cdrom_device);
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 1:
	restoreLDOption->setText ("CD device:");
	restoreEditDOption->setText (cdrom_device);
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 2:
	restoreLDOption->setText ("CD device:");
	restoreEditDOption->setText (cdrom_device);
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 3:
	restoreLDOption->setText ("DVD device:");
	restoreEditDOption->setText (cdrom_device);
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 4:
	restoreLDOption->setText ("Directory for ISOs:");
	restoreEditDOption->setText ("/root/images/mondo");
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 5:
	restoreLDOption->setText ("NFS share:");
	restoreEditDOption->setText ("");
	restoreNFSRemoteDir->setEnabled (true);
	restoreEditNFSRemoteDir->setEnabled (true);
	restoreEditNFSRemoteDir->setText ("");
	break;
    case 6:
	restoreLDOption->setText ("Tape /dev entry:");
#if __FreeBSD__
	restoreEditDOption->setText ("/dev/sa0");
#elif linux
	restoreEditDOption->setText ("/dev/st0");
#else
	restoreEditDOption->setText ("");
#endif
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    case 7:
	restoreLDOption->setText ("Device/file to use:");
	restoreEditDOption->setText ("");
	restoreNFSRemoteDir->setEnabled (false);
	restoreEditNFSRemoteDir->setEnabled (false);
	break;
    }
}
	
void XMondo::initBackupTab (QWidget *& bBkpinfoTab)
{
    QGridLayout *bBkpinfoGrid;
    QVButtonGroup *mediaType, *compression;
    QLabel *dOption, *mediaSize;
    QRadioButton *mTape, *mUdev, *mISO, *mNFS, *mCDR, *mCDRW, *mCDStream, *mDVD;
    QRadioButton *cMax, *cAvg, *cMin, *cNone;
    char *cdrwDev = new char [100];
    if (find_cdrw_device (cdrwDev)) strcpy (cdrwDev, "0,0,0");
    bBkpinfoTab   = new QWidget (backup, "bBkpinfoTab");
    mediaType     = new QVButtonGroup ("&Media Type:", bBkpinfoTab, "mediaType");
    mCDR          = new QRadioButton ("CD-R discs", mediaType);
    mCDRW         = new QRadioButton ("CD-RW discs", mediaType);
    mCDStream     = new QRadioButton ("CD streamer", mediaType);
    mDVD          = new QRadioButton ("DVD discs", mediaType);
    mISO          = new QRadioButton ("ISOs in dir.", mediaType);
    mNFS          = new QRadioButton ("NFS (network)", mediaType);
    mTape         = new QRadioButton ("Tape streamer", mediaType);
    mUdev         = new QRadioButton ("Universal device", mediaType);
    compression   = new QVButtonGroup ("&Compression", bBkpinfoTab, "compression");
    cMax          = new QRadioButton ("Maximum (very slow)", compression);
    cAvg          = new QRadioButton ("Average compression", compression);
    cMin          = new QRadioButton ("Minimum (very fast)", compression);
    cNone         = new QRadioButton ("No software compression", compression);
    dOption       = new QLabel ("CD-R node:", bBkpinfoTab, "dOption");
    nfsRemoteDir  = new QLabel ("NFS remote directory:", bBkpinfoTab, "nfsRemoteDir");
    mediaSize     = new QLabel ("Media size (MB):", bBkpinfoTab, "mediaSize");
    editDOption   = new QLineEdit (bBkpinfoTab, "editDOption");
    editNFSRemoteDir=new QLineEdit(bBkpinfoTab, "editNFSRemoteDir");
    editMediaSize = new QLineEdit (bBkpinfoTab, "editMediaSize");
    mCDR->setChecked (true);
    cAvg->setChecked (true);
    editDOption->setText (cdrwDev);
    nfsRemoteDir->setEnabled (false);
    editNFSRemoteDir->setEnabled (false);
    editMediaSize->setText ("650");
    bBkpinfoGrid = new QGridLayout (bBkpinfoTab, 5, 2, 5, 5, "bBkpinfoGrid");
    bBkpinfoGrid->addMultiCellWidget (mediaType, 0, 1, 0, 0);
    bBkpinfoGrid->addMultiCellWidget (compression, 0, 1, 1, 1);
    bBkpinfoGrid->addWidget (dOption, 2, 0, AlignRight);
    bBkpinfoGrid->addWidget (mediaSize, 3, 0, AlignRight);
    bBkpinfoGrid->addWidget (nfsRemoteDir, 4, 0, AlignRight);
    bBkpinfoGrid->addWidget (editDOption, 2, 1); 
    bBkpinfoGrid->addWidget (editMediaSize, 3, 1);
    bBkpinfoGrid->addWidget (editNFSRemoteDir, 4, 1);
    connect (mediaType, SIGNAL(clicked(int)), this, SLOT(slotMediaType(int)));
    lDOption = dOption;
    bgMediaType = mediaType; bgCompression = compression;
}

void XMondo::initCompareTab (QWidget *& bBkpinfoTab)
{
    QGridLayout *bBkpinfoGrid;
    QVButtonGroup *mediaType;
    QLabel *dOption, *mediaSize;
    QRadioButton *mTape, *mUdev, *mISO, *mNFS, *mCDR, *mCDRW, *mCDStream, *mDVD;
    char *cdrwDev = new char [100];
    if (find_cdrw_device (cdrwDev)) strcpy (cdrwDev, "0,0,0");
    bBkpinfoTab          = new QWidget (backup, "bBkpinfoTab");
    mediaType            = new QVButtonGroup ("&Media type to compare:", bBkpinfoTab, "compareMediaType");
    mCDR                 = new QRadioButton ("CD-R discs", mediaType);
    mCDRW                = new QRadioButton ("CD-RW discs", mediaType);
    mCDStream            = new QRadioButton ("CD streamer", mediaType);
    mDVD                 = new QRadioButton ("DVD discs", mediaType);
    mISO                 = new QRadioButton ("ISOs in dir.", mediaType);
    mNFS                 = new QRadioButton ("NFS (network)", mediaType);
    mTape                = new QRadioButton ("Tape streamer", mediaType);
    mUdev                = new QRadioButton ("Universal device", mediaType);
    dOption              = new QLabel ("CD-R node:", bBkpinfoTab, "compareDOption");
    compareNFSRemoteDir  = new QLabel ("NFS remote directory:", bBkpinfoTab, "compareNFSRemoteDir");
    compareEditDOption   = new QLineEdit (bBkpinfoTab, "compareEditDOption");
    compareEditNFSRemoteDir=new QLineEdit(bBkpinfoTab, "compareEditNFSRemoteDir");
    mCDR->setChecked (true);
    compareEditDOption->setText (cdrwDev);
    compareNFSRemoteDir->setEnabled (false);
    compareEditNFSRemoteDir->setEnabled (false);
    bBkpinfoGrid = new QGridLayout (bBkpinfoTab, 4, 2, 5, 5, "bBkpinfoGrid");
    bBkpinfoGrid->addMultiCellWidget (mediaType, 0, 1, 0, 1);
    bBkpinfoGrid->addWidget (dOption, 2, 0, AlignRight);
    bBkpinfoGrid->addWidget (compareNFSRemoteDir, 3, 0, AlignRight);
    bBkpinfoGrid->addWidget (compareEditDOption, 2, 1);
    bBkpinfoGrid->addWidget (compareEditNFSRemoteDir, 3, 1);
    bStartCompare = new QPushButton ("START", bBkpinfoTab, "startCompare");
    connect (bStartCompare, SIGNAL(clicked()), this, SLOT(slotStartCompare()));
    bBkpinfoGrid->addMultiCellWidget (bStartCompare, 4, 4, 0, 3);
    connect (mediaType, SIGNAL(clicked(int)), this, SLOT(slotCompareMediaType(int)));
    compareLDOption = dOption;
    bgCompareMediaType = mediaType;
}

void XMondo::initRestoreTab (QWidget *& bRestoreTab)
{
    QGridLayout *bRestoreGrid;
    QVButtonGroup *mediaType;
    QLabel *dOption, *mediaSize, *filter;
    QRadioButton *mTape, *mUdev, *mISO, *mNFS, *mCDR, *mCDRW, *mCDStream, *mDVD;
    char *cdrom_device = new char [100];
    if (does_file_exist ("/dev/cdroms/cdrom0")) {
	strcpy (cdrom_device, "/dev/cdroms/cdrom0");
    } else if (does_file_exist ("/dev/cdrom")) {
	strcpy (cdrom_device, "/dev/cdrom");
    } else {
	find_cdrom_device (cdrom_device, false);
    }
    bRestoreTab          = new QWidget (backup, "bRestoreTab");
    mediaType            = new QVButtonGroup ("&Media type to restore from:", bRestoreTab, "restoreMediaType");
    mCDR                 = new QRadioButton ("CD-R discs", mediaType);
    mCDRW                = new QRadioButton ("CD-RW discs", mediaType);
    mCDStream            = new QRadioButton ("CD streamer", mediaType);
    mDVD                 = new QRadioButton ("DVD discs", mediaType);
    mISO                 = new QRadioButton ("ISOs in dir.", mediaType);
    mNFS                 = new QRadioButton ("NFS (network)", mediaType);
    mTape                = new QRadioButton ("Tape streamer", mediaType);
    mUdev                = new QRadioButton ("Universal device", mediaType);
    dOption              = new QLabel ("CD-R node:", bRestoreTab, "restoreDOption");
    restoreNFSRemoteDir  = new QLabel ("NFS remote directory:", bRestoreTab, "restoreNFSRemoteDir");
    restoreEditDOption   = new QLineEdit (bRestoreTab, "restoreEditDOption");
    restoreEditNFSRemoteDir=new QLineEdit(bRestoreTab, "restoreEditNFSRemoteDir");
    filter               = new QLabel ("Filelist filter (regexp):", bRestoreTab);
    restoreFilter        = new QLineEdit (bRestoreTab, "restoreFilter");
    mCDR->setChecked (true);
    restoreEditDOption->setText (cdrom_device);
    restoreNFSRemoteDir->setEnabled (false);
    restoreEditNFSRemoteDir->setEnabled (false);
    bRestoreGrid = new QGridLayout (bRestoreTab, 5, 2, 5, 5, "bRestoreGrid");
    bRestoreGrid->addMultiCellWidget (mediaType, 0, 1, 0, 1);
    bRestoreGrid->addWidget (dOption, 2, 0, AlignRight);
    bRestoreGrid->addWidget (restoreNFSRemoteDir, 3, 0, AlignRight);
    bRestoreGrid->addWidget (filter, 4, 0, AlignRight);
    bRestoreGrid->addWidget (restoreEditDOption, 2, 1);
    bRestoreGrid->addWidget (restoreEditNFSRemoteDir, 3, 1);
    bRestoreGrid->addWidget (restoreFilter, 4, 1);
    connect (mediaType, SIGNAL(clicked(int)), this, SLOT(slotRestoreMediaType(int)));
    restoreLDOption = dOption;
    bgRestoreMediaType = mediaType;
}

void XMondo::initOptionsTab (QWidget *& bOptionsTab)
{
    QGridLayout *bOptionsGrid;
    QListView *imageDevs, *exclude, *include;
    QPushButton *incAdd, *incDel, *exAdd, *exDel;
    QLineEdit *incPending, *exPending;
    QCheckBox *compare, *bnfs, *diff;

    bOptionsTab  = new QWidget (backup, "bOptionsTab");
    bOptionsGrid = new QGridLayout (bOptionsTab, 19, 5, 5, 5, "bOptionsGrid");
    imageDevs    = new QListView (bOptionsTab, "imageDevs");
    exclude      = new QListView (bOptionsTab, "exclude");
    include      = new QListView (bOptionsTab, "include");
    incAdd       = new QPushButton ("Add", bOptionsTab);
    incDel       = new QPushButton ("Delete", bOptionsTab);
    exAdd        = new QPushButton ("Add", bOptionsTab);
    exDel        = new QPushButton ("Delete", bOptionsTab);
    incPending   = new QLineEdit (bOptionsTab);
    exPending    = new QLineEdit (bOptionsTab);
    compare      = new QCheckBox ("Compare after backup", bOptionsTab);
    bnfs         = new QCheckBox ("Backup NFS mounts", bOptionsTab);
    diff         = new QCheckBox ("Differential backup", bOptionsTab);

    imageDevs->addColumn ("Devices to back up as images:");
    exclude->addColumn ("Exclude paths:");
    include->addColumn ("Include paths:");
    (void) new QListViewItem (include, "/");

    connect (incAdd, SIGNAL(clicked()), this, SLOT(slotAddInclude()));
    connect (incDel, SIGNAL(clicked()), this, SLOT(slotDelInclude()));
    connect (exAdd,  SIGNAL(clicked()), this, SLOT(slotAddExclude()));
    connect (exDel,  SIGNAL(clicked()), this, SLOT(slotDelExclude()));
		
    bOptionsGrid->addMultiCellWidget (imageDevs,    0,  5, 0, 4);
    bOptionsGrid->addRowSpacing (6, 10);
    bOptionsGrid->addRowSpacing (2, 90);
    bOptionsGrid->addMultiCellWidget (exclude,      7, 12, 3, 4);
    bOptionsGrid->addMultiCellWidget (include,      7, 12, 0, 1);
    bOptionsGrid->addRowSpacing (10, 90);
    bOptionsGrid->addWidget (incAdd, 13, 0);
    bOptionsGrid->addWidget (incDel, 13, 1);
    bOptionsGrid->addWidget (exAdd,  13, 3);
    bOptionsGrid->addWidget (exDel,  13, 4);
    bOptionsGrid->addMultiCellWidget (incPending,  14, 14, 0, 1);
    bOptionsGrid->addMultiCellWidget (exPending,   14, 14, 3, 4);
    bOptionsGrid->addRowSpacing (15, 10);
    bOptionsGrid->addMultiCellWidget (compare,     16, 16, 0, 4);
    bOptionsGrid->addMultiCellWidget (bnfs,        17, 17, 0, 4);
    bOptionsGrid->addMultiCellWidget (diff,        18, 18, 0, 4);

    listImageDevs     = imageDevs;
    listInclude       = include;
    listExclude       = exclude;
    checkBackupNFS    = bnfs;
    checkCompare      = compare;
    checkDifferential = diff;
    pendingInclude    = incPending;
    pendingExclude    = exPending;
}

void XMondo::initAdvancedTab (QWidget *&bAdvancedTab)
{
    QGridLayout *bAdvancedGrid;
    QListView *excludeDevs;
    QCheckBox *useLilo, *makeBootFloppies;
#ifndef __FreeBSD__
    QLabel *kernel;
#endif
    QLabel *bootDevice, *bootLoader;
    QRadioButton *grub, *lilo, *boot0, *raw, *autodetect;
    QHButtonGroup *radioBootLoader;
    int rowAdd = 0;

    bAdvancedTab     = new QWidget (backup, "bAdvancedTab");
#ifndef __FreeBSD__
    bAdvancedGrid    = new QGridLayout (bAdvancedTab, 11, 2, 5, 5, "bAdvancedGrid");
    kernel           = new QLabel ("Kernel path:", bAdvancedTab, "kernel");
    editKernel       = new QLineEdit (bAdvancedTab, "editKernel");
    useLilo          = new QCheckBox ("Use LILO (instead of syslinux) for boot disk", bAdvancedTab, "useLilo");
#else
    bAdvancedGrid    = new QGridLayout (bAdvancedTab, 9, 2, 5, 5, "bAdvancedGrid");
    useLilo          = 0;
#endif
    excludeDevs      = new QListView (bAdvancedTab, "excludeDevs");
    makeBootFloppies = new QCheckBox ("Make boot floppies after backup", bAdvancedTab, "makeFloppies");
    makeBootFloppies    -> setChecked (true);
    bootDevice       = new QLabel ("Boot device:", bAdvancedTab, "bootDevice");
    bootLoader       = new QLabel ("Boot loader:", bAdvancedTab, "bootLoader");
    editBootDevice   = new QLineEdit (bAdvancedTab, "editBootDevice");
    radioBootLoader  = new QHButtonGroup (bAdvancedTab, "radioBootLoader");
    grub             = new QRadioButton ("GRUB", radioBootLoader);
#ifdef __IA64
    lilo             = new QRadioButton ("ELILO", radioBootLoader);
#else
    lilo             = new QRadioButton ("LILO", radioBootLoader);
#endif
#ifdef __FreeBSD__
    boot0            = new QRadioButton ("boot0", radioBootLoader);
#endif
    raw              = new QRadioButton ("Raw", radioBootLoader);
    autodetect       = new QRadioButton ("Auto", radioBootLoader);
    autodetect          -> setChecked (true);

    excludeDevs->addColumn ("Devices to exclude from backup:");
    bAdvancedGrid->addMultiCellWidget (excludeDevs, 0, 5, 0, 1);
#ifndef __FreeBSD__
    bAdvancedGrid->addWidget (kernel, 6, 0, AlignRight);
    bAdvancedGrid->addWidget (editKernel, 6, 1);
    rowAdd = 1;
#endif
    bAdvancedGrid->addWidget (bootDevice, 6 + rowAdd, 0, AlignRight);
    bAdvancedGrid->addWidget (editBootDevice, 6 + rowAdd, 1);
    bAdvancedGrid->addWidget (bootLoader, 7 + rowAdd, 0, AlignRight);
    bAdvancedGrid->addWidget (radioBootLoader, 7 + rowAdd, 1);
    bAdvancedGrid->addMultiCellWidget (makeBootFloppies, 8 + rowAdd, 8 + rowAdd, 0, 1);
#ifndef __FreeBSD__
    // We know rowAdd = 1 so we can just use 10 instead of 9+rowAdd...
    bAdvancedGrid->addMultiCellWidget (useLilo, 10, 10, 0, 1);
#endif

    listExcludeDevs       = excludeDevs;
    bgBootLoader          = radioBootLoader;
    checkUseLilo          = useLilo;
    checkMakeBootFloppies = makeBootFloppies;
}

void XMondo::fillBkpinfo (struct s_bkpinfo *bkpinfo)
{
    reset_bkpinfo (bkpinfo);

    switch (bgMediaType->id (bgMediaType->selected())) {
    case 0:
	bkpinfo->backup_media_type = cdr;
	break;
    case 1:
	bkpinfo->backup_media_type = cdrw;
	bkpinfo->wipe_media_first  = 1;
	break;
    case 2:
	bkpinfo->backup_media_type = cdstream;
	break;
    case 3:
	bkpinfo->backup_media_type = dvd;
	break;
    case 4:
	bkpinfo->backup_media_type = iso;
	strcpy (bkpinfo->isodir, editDOption->text());
	break;
    case 5:
	bkpinfo->backup_media_type = nfs;
	strcpy (bkpinfo->nfs_mount, editDOption->text());
	while (system (QString ("mount | grep -qx '%1 .*'").arg (bkpinfo->nfs_mount))) {
	    if (QMessageBox::warning (this, "XMondo", QString ("%1 must be mounted. Please mount it and press Retry, or press Cancel to abort..").arg (bkpinfo->nfs_mount), "&Retry", "&Cancel")) {
		return;
	    }
	}
	strcpy (bkpinfo->nfs_remote_dir, editNFSRemoteDir->text());
	break;
    case 6:
	bkpinfo->backup_media_type = tape;
	break;
    case 7:
	bkpinfo->backup_media_type = udev;
	break;
    }
    switch (bgCompression->id (bgCompression->selected())) {
    case 0:
	bkpinfo->compression_level = 9;
	break;
    case 1:
	bkpinfo->compression_level = 6;
	break;
    case 2:
	bkpinfo->compression_level = 1;
	break;
    case 3:
	bkpinfo->compression_level = 0;
	break;
    }
    bkpinfo->cdrw_speed         = (bkpinfo->backup_media_type == cdstream) ? 2 : 4;
    bkpinfo->use_lzo            = (bkpinfo->backup_media_type == cdstream) ? 1 : 0;
    bkpinfo->nonbootable_backup = 0;
    strcpy (bkpinfo->media_device, editDOption->text());
    for (int i = 0; i <= MAX_NOOF_MEDIA; ++i) {
	if (bkpinfo->backup_media_type != udev && bkpinfo->backup_media_type != tape) {
	    bkpinfo->media_size[i]  = editMediaSize->text().toInt();
	} else {
	    bkpinfo->media_size[i]  = 0;
	}
    }
    strcpy (bkpinfo->boot_device, editBootDevice->text());
    switch (bgBootLoader->id (bgBootLoader->selected())) {
    case 0:
	bkpinfo->boot_loader    = 'G';
	break;
    case 1:
	bkpinfo->boot_loader    = 'L';
	break;
#ifdef __FreeBSD__
    case 2:
	bkpinfo->boot_loader    = 'B';
	break;
    case 3:
	bkpinfo->boot_loader    = 'R';
	break;
#else
    case 2:
	bkpinfo->boot_loader    = 'R';
	break;
#endif
    default:
	bkpinfo->boot_loader    = which_boot_loader (bkpinfo->boot_device);
	break;
    }
#ifndef __FreeBSD__
    bkpinfo->make_cd_use_lilo   = checkUseLilo->isChecked();
#endif
    bkpinfo->backup_data        = 1;
    bkpinfo->verify_data        = checkCompare->isChecked();
    bkpinfo->differential       = checkDifferential->isChecked();
#ifndef __FreeBSD__
    strcpy (bkpinfo->kernel_path, editKernel->text());
    if (!bkpinfo->kernel_path[0]) {
	strcpy (bkpinfo->kernel_path, call_program_and_get_last_line_of_output ("mindi --findkernel"));
	if (!bkpinfo->kernel_path[0]) {
	    popup_and_OK_sub ("You left the kernel blank and I couldn't find one for you. Using my failsafe kernel.");
	    strcpy (bkpinfo->kernel_path, "FAILSAFE");
	}
    }
    else if (!strcasecmp (bkpinfo->kernel_path, "FAILSAFE") ||
	     !strcasecmp (bkpinfo->kernel_path, "SUCKS")) {
	for (char *p = bkpinfo->kernel_path; *p; ++p) {
	    *p = toupper (*p);
	}
    }
    else if (!does_file_exist (bkpinfo->kernel_path)) {
	strcpy (bkpinfo->kernel_path, call_program_and_get_last_line_of_output ("mindi --findkernel"));
	if (!bkpinfo->kernel_path[0]) {
	    popup_and_OK_sub ("You specified a nonexistent kernel and I couldn't find one. Please correct the kernel location on the \"Advanced\" tab.");
	    return;
	}
    }
#endif

    for (QListViewItem *li = listImageDevs->firstChild(); li; li = li->itemBelow()) {
	QCheckListItem *cli;
	if ((cli = dynamic_cast <QCheckListItem*> (li)) && cli->isOn()) {
	    if (bkpinfo->image_devs[0])
		strcat (bkpinfo->image_devs, " ");
	    strcat (bkpinfo->image_devs, li->text (0));
	}
    }
    bkpinfo->include_paths[0] = '\0';
    for (QListViewItem *li = listInclude->firstChild(); li; li = li->itemBelow()) {
	if (bkpinfo->include_paths[0])
	    strcat (bkpinfo->include_paths, " ");
	strcat (bkpinfo->include_paths, li->text (0));
    }
    for (QListViewItem *li = listExclude->firstChild(); li; li = li->itemBelow()) {
	if (bkpinfo->exclude_paths[0])
	    strcat (bkpinfo->exclude_paths, " ");
	strcat (bkpinfo->exclude_paths, li->text (0));
    }
    for (QListViewItem *li = listExcludeDevs->firstChild(); li; li = li->itemBelow()) {
	QCheckListItem *cli;
	if ((cli = dynamic_cast <QCheckListItem*> (li)) && cli->isOn()) {
	    if (bkpinfo->exclude_paths[0])
		strcat (bkpinfo->exclude_paths, " ");
	    strcat (bkpinfo->exclude_paths, li->text (0));
	}
    }

    if (!checkBackupNFS->isChecked()) {
	char tmp[MAX_STR_LEN];
	strncpy(tmp, list_of_NFS_devices_and_mounts(), MAX_STR_LEN);
	if (bkpinfo->exclude_paths[0]) { strcat(bkpinfo->exclude_paths, " "); }
	strcat(bkpinfo->exclude_paths, tmp);
    }
    sensibly_set_tmpdir_and_scratchdir (bkpinfo);
}

void XMondo::slotStartBackup()
{
  unlink(MONDO_LOGFILE);

    if (geteuid()) {
	fatal_error_sub ("Please run as root.");
	exit (1);
    }

    struct s_bkpinfo *bkpinfo = new s_bkpinfo;
    reset_bkpinfo (bkpinfo);
    
    bStartBackup->setEnabled (false);
    bStartBackup->setText ("Initializing...");
    
    if (pre_param_configuration (bkpinfo)) {
	fatal_error_sub ("Failed to pre-param initialize.");
	exit (1);
    }

    fillBkpinfo (bkpinfo);

    if (post_param_configuration (bkpinfo)) {
	fatal_error_sub ("Failed to post-param initialize.");
	exit (1);
    }

    XMondoBackup *backup = new XMondoBackup;
    system (QString ("mkdir -p %1 %2").arg (bkpinfo->tmpdir).arg (bkpinfo->scratchdir).ascii());
    g_operation_in_progress = 1;

    stack->raiseWidget (3);
    buttons->setEnabled (false);
    bStartBackup->setEnabled (true);
    bStartBackup->setText ("START");

    int res;
    if ((res = backup->run (bkpinfo)) > 0) {
	popup_and_OK_sub ("Backup completed. However, there were some errors. Please check /var/log/mondo-archive.log for details.");
    } else if (res == 0) {
	popup_and_OK_sub ("Backup completed with no errors.");
    }

    buttons->setEnabled (true);
    stack->raiseWidget (0); // backup widget
    g_operation_in_progress = 0;
}



class XMondoDiffList : public QDialog
{
public:
    XMondoDiffList (QWidget *parent, const char *name = 0, bool modal = TRUE, WFlags f = 0)
	: QDialog (parent, name, modal, f)
    {
	QVBoxLayout *box = new QVBoxLayout (this, 5, 5, "xmondoDiffListBox");
	QLabel *infoText = new QLabel ("<b>The following files were found to contain differences:</b>", this);
	QListView *diffs = new QListView (this);
	QPushButton *bOK = new QPushButton ("OK", this);

	diffs->addColumn ("File");
	diffs->addColumn ("Severity (1 to 3, 1 is low)");
	diffs->setSelectionMode (QListView::NoSelection);
	bOK->setDefault (true);
	connect (bOK, SIGNAL(clicked()), this, SLOT(accept()));

	box->addWidget (infoText);
	box->addWidget (diffs);
	box->addWidget (bOK);

	QFile file ("/tmp/changed.files");
	if (file.open (IO_ReadOnly)) {
	    QTextStream stream (&file);
	    QString line;
	    int i = 1;
	    while (!stream.atEnd()) {
		line = stream.readLine();
		(void) new QListViewItem (diffs, line, QString::number
					  (severity_of_difference (const_cast<char*> (line.ascii()), 0)));
	    }
	    file.close();
	} else {
	    popup_and_OK_sub ("Compare completed with errors.");
	}
    }

    virtual ~XMondoDiffList() {}
};

void XMondo::slotStartCompare()
{
    struct s_bkpinfo bi;
    struct s_bkpinfo *bkpinfo = &bi;
    reset_bkpinfo (bkpinfo);
    
    if (geteuid()) {
	fatal_error_sub ("Please run as root.");
    }

    bStartCompare->setEnabled (false);
    bStartCompare->setText ("Initializing...");

    if (pre_param_configuration (bkpinfo)) {
	fatal_error_sub ("Failed to pre-param initialize.");
	exit (1);
    }
    switch (bgCompareMediaType->id (bgCompareMediaType->selected())) {
    case 0:
	bkpinfo->backup_media_type = cdr;
	break;
    case 1:
	bkpinfo->backup_media_type = cdrw;
	bkpinfo->wipe_media_first  = 1;
	break;
    case 2:
	bkpinfo->backup_media_type = cdstream;
	break;
    case 3:
	bkpinfo->backup_media_type = dvd;
	break;
    case 4:
	bkpinfo->backup_media_type = iso;
	strcpy (bkpinfo->isodir, compareEditDOption->text());
	break;
    case 5:
	bkpinfo->backup_media_type = nfs;
	strcpy (bkpinfo->nfs_mount, compareEditDOption->text());
	while (system (QString ("mount | grep -qx '%1 .*'").arg (bkpinfo->nfs_mount))) {
	    if (QMessageBox::warning (this, "XMondo", QString ("%1 must be mounted. Please mount it and press Retry, or press Quit to abort..").arg (bkpinfo->nfs_mount), "&Retry", "&Quit")) {
		return;
	    }
	}
	strcpy (bkpinfo->nfs_remote_dir, compareEditNFSRemoteDir->text());
	break;
    case 6:
	bkpinfo->backup_media_type = tape;
	break;
    case 7:
	bkpinfo->backup_media_type = udev;
	break;
    }

    strcpy (bkpinfo->media_device, compareEditDOption->text());
    bkpinfo->verify_data        = 1;
    bkpinfo->backup_data        = 0;

    sensibly_set_tmpdir_and_scratchdir (bkpinfo);
    if (post_param_configuration (bkpinfo)) {
	fatal_error_sub ("Failed to post-param initialize.");
	exit (1);
    }

    XMondoBackup *backup = new XMondoBackup;
    system (QString ("mkdir -p %1 %2").arg (bkpinfo->tmpdir).arg (bkpinfo->scratchdir).ascii());
    g_operation_in_progress = 1;

    stack->raiseWidget (3);
    buttons->setEnabled (false);
    bStartCompare->setEnabled (true);
    bStartCompare->setText ("START");

    if (backup->compare (bkpinfo)) {
	if (does_file_exist ("/tmp/changed.files")) {
	    XMondoDiffList *xmdl = new XMondoDiffList (this);
	    xmdl->exec();
	    unlink ("/tmp/changed.txt");
	    unlink ("/tmp/changed.files");
	} else
	    popup_and_OK_sub ("Compare completed with errors.");
    } else {
	popup_and_OK_sub ("Compare completed with no differences. Your backup is perfect.");
    }

    buttons->setEnabled (true);
    stack->raiseWidget (1); // compare widget
    g_operation_in_progress = 0;
}

void XMondo::slotPrevRestore()
{
    rPrev->hide();
    rNext->setText ("Next ->");
    rNext->setEnabled (true);
    rNext->show();
    delete rstr; rstr = 0;
    restoreStack->raiseWidget (0);
    delete restoreFiles;
}

void XMondo::slotNextRestore() 
{
    if (rNext->text() == "Next ->") {
	rNext->setEnabled (false);
	rNext->setText ("Finish");
	rPrev->show();

	restoreFiles = new QWidget (restoreStack);
	restoreStack->addWidget (restoreFiles, 1);

	rstr = new XMondoRestore (restoreFiles, bgRestoreMediaType, restoreEditDOption, restoreEditNFSRemoteDir, restoreFilter);
	if (!rstr->good()) {
	    slotPrevRestore();
	    return;
	}
	restoreStack->raiseWidget (1);
	while (rstr && !rstr->isSetupDone() && rstr->good()) {
	    kapp->processEvents();
	    usleep (50000);
	}
	if (!rstr) return;
	if (rstr->isSetupDone() && rstr->good()) {
	    rNext->setEnabled (true);
	}
    } else {
	stack->raiseWidget (3);
	buttons->setEnabled (false);
	rstr->go();
	if (!rstr->good()) {
	    return;
	}
	delete rstr; rstr = 0;
	stack->raiseWidget (2);
	buttons->setEnabled (true);

	rNext->setText ("Next ->");
	rNext->setEnabled (true);
	rPrev->hide();
	restoreStack->raiseWidget (0);
	delete restoreFiles;
    }
    rNext->show();
}

void XMondo::slotAddInclude()
{
    if (!(pendingInclude->text().isEmpty()) && pendingInclude->text().at(0) == '/') {
	(void) new QListViewItem (listInclude, pendingInclude->text());
	pendingInclude->setText ("");
    }
}

void XMondo::slotDelInclude()
{
    QListViewItem *selection = listInclude->selectedItem();
    if (!selection) return;
    QString txt = selection->text (0);
    listInclude->takeItem (selection);
    delete selection;
    pendingInclude->setText (txt);
}

void XMondo::slotAddExclude()
{
    if (!(pendingExclude->text().isEmpty()) && pendingExclude->text().at(0) == '/') {
	(void) new QListViewItem (listExclude, pendingExclude->text());
	pendingExclude->setText ("");
    }
}

void XMondo::slotDelExclude()
{
    QListViewItem *selection = listExclude->selectedItem();
    if (!selection) return;
    QString txt = selection->text (0);
    listExclude->takeItem (selection);
    delete selection;
    pendingExclude->setText (txt);
}

void XMondo::slotTabChange (QWidget *newtab)
{
    if (newtab == tabOptions) {
	QStringList result;
	QListViewItem *item, *itembelow;
	QCheckListItem *checkitem, *below;
	QStringList checkedBoxes;
#ifdef __FreeBSD__
	result = QStringList::split (" ", call_program_and_get_last_line_of_output ("ls -1 /dev/ad??* /dev/da??* 2>/dev/null | tr '\n' ' ' | tr -s ' '; echo"));
#else
	result = QStringList::split (" ", call_program_and_get_last_line_of_output ("ls -1 /dev/ide/host?/bus?/target?/lun?/part? /dev/hd??* /dev/scsi/host?/bus?/target?/lun?/part? /dev/sd??* 2>/dev/null | tr '\n' ' ' | tr -s ' '; echo"));
#endif
	for (item = listImageDevs->firstChild(); item; item = itembelow) {
	    if ((checkitem = dynamic_cast <QCheckListItem*> (item))) {
		if (checkitem->isOn()) {
		    if (checkedBoxes.grep (checkitem->text()).empty()) {
			checkedBoxes += checkitem->text();
		    }
		}
	    }
	    itembelow = item->itemBelow();
	    listImageDevs->takeItem (item);
	    delete item;
	}
	for (QStringList::iterator it = result.begin(); it != result.end(); ++it) {
	    if (is_this_device_mounted (const_cast <char*> ((*it).ascii()))) continue;
	    checkitem = new QCheckListItem (listImageDevs, *it, QCheckListItem::CheckBox);
	    for (itembelow = listExcludeDevs->firstChild(); itembelow; itembelow = itembelow->itemBelow()) {
		if (itembelow->text (0) == checkitem->text (0) && (below = dynamic_cast <QCheckListItem*> (itembelow)) && below->isOn()) {
		    delete checkitem;
		    checkitem = 0;
		    (void) new QListViewItem (listImageDevs, QString ("%1 is excluded (can't be both)").arg (*it));
		    break;
		}
	    }
	    if (checkedBoxes.grep (QRegExp (*it + "$")).count() && checkitem) {
		checkitem->setOn (true);
	    }
	}
    }
    if (newtab == tabAdvanced) {
	QStringList result;
	QListViewItem *item, *itembelow;
	QCheckListItem *checkitem, *below;
	QStringList checkedBoxes;
	result = QStringList::split (" ", call_program_and_get_last_line_of_output ("mount | grep '^/dev/' | cut -d' ' -f1 | tr '\n' ' '; echo"));
	for (item = listExcludeDevs->firstChild(); item; item = itembelow) {
	    if ((checkitem = dynamic_cast <QCheckListItem*> (item))) {
		if (checkitem->isOn()) {
		    if (checkedBoxes.grep (checkitem->text()).empty()) {
			checkedBoxes += checkitem->text();
		    }
		}
	    }
	    itembelow = item->itemBelow();
	    listExcludeDevs->takeItem (item);
	    delete item;
	}
	for (QStringList::iterator it = result.begin(); it != result.end(); ++it) {
	    checkitem = new QCheckListItem (listExcludeDevs, *it, QCheckListItem::CheckBox);
	    for (itembelow = listImageDevs->firstChild(); itembelow; itembelow = itembelow->itemBelow()) {
		if (itembelow->text (0) == checkitem->text (0) && (below = dynamic_cast <QCheckListItem*> (itembelow)) && below->isOn()) {
		    delete checkitem;
		    checkitem = 0;
		    (void) new QListViewItem (listExcludeDevs, QString ("%1 is imaged (can't be both)").arg (*it));
		    break;
		}
	    }
	    if (checkedBoxes.grep (QRegExp (*it + "$")).count() && checkitem) {
		checkitem->setOn (true);
	    }
	}
    }

}
