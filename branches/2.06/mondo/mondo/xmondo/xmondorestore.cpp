/***************************************************************************
                         xmondorestore.cpp - restore functions
                         -------------------------------------
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

#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qstringlist.h>
#include <qmessagebox.h>

#include "xmondo.h"
#include "xmondorestore.h"
#include <X-specific.h>
extern "C" {
#define bool int
#include <libmondo-devices-EXT.h>
#include <libmondo-filelist-EXT.h>
#include <libmondo-files-EXT.h>
#include <libmondo-fork-EXT.h>
#include <libmondo-tools-EXT.h>
#undef bool
    int restore_everything (struct s_bkpinfo *bkpinfo, struct s_node *filelist);
    int get_cfg_file_from_archive (struct s_bkpinfo *bkpinfo);
    int read_cfg_file_into_bkpinfo (char *cfg_file, struct s_bkpinfo *bkpinfo);
    int mount_cdrom (struct s_bkpinfo *bkpinfo);
    void setup_MR_global_filenames (struct s_bkpinfo *bkpinfo);
    extern char *g_mondo_cfg_file;
    extern char *g_filelist_full;
    extern int g_current_media_number;
    extern int g_text_mode;
}

extern QProgressBar            *XMondoProgress;
extern QLabel                  *XMondoProgressWhat, *XMondoProgressWhat2, *XMondoProgressWhat3;
extern QCheckBox               *XMondoVerbose;
extern QMultiLineEdit          *XMondoLog;
extern QLabel                  *XMondoStatus;
extern QLabel                  *XMondoTimeTaken, *XMondoTimeToGo, *XMondoProgressPercent;
extern QPushButton             *XMondoCancel;
extern XMEventHolder            events;

class XMCheckListItem : public QCheckListItem
{
public:
    XMCheckListItem (QCheckListItem * parent, const QString & text, Type tt = CheckBox)
	: QCheckListItem (parent, text, tt), _text (text)
    {
    }

    XMCheckListItem (QListViewItem * parent, const QString & text, Type tt = CheckBox)
	: QCheckListItem (parent, text, tt), _text (text)
    {
    }

    XMCheckListItem (QListView * parent, const QString & text, Type tt = CheckBox)
	: QCheckListItem (parent, text, tt), _text (text)
    {
    }

    virtual ~XMCheckListItem() {}

    virtual int rtti() { return 1001; }

    virtual void setOn (bool on) {
	QCheckListItem::setOn (on);
    }

    static QLabel *progressDisplay;
    static bool *doneSetup;
    static bool selecting;
    static int counter;
    static int depth;

protected:
    virtual void stateChange (bool on) {
	if (firstChild()) {
	    if (!selecting) {
		progressDisplay->setText (QString ("%1electing %2...").arg (on? "S" : "Des").arg (_text));
		counter = depth = 0;
		selecting = true;
		if (doneSetup) *doneSetup = false;
	    }

	    for (QListViewItem *lvi = firstChild(); lvi; lvi = lvi->nextSibling()) {
		XMCheckListItem *cli;
		if ((cli = dynamic_cast <XMCheckListItem *> (lvi)) != 0) {
		    if (!(++counter % 10000)) {
			progressDisplay->setText (progressDisplay->text() + ".");
		    }
		    if (!(counter % 1000)) {	
			kapp->processEvents();
		    }
		    depth++;
		    cli->setOn (on);
		    depth--;
		}
	    }

	    if (depth == 0) {
		selecting = false;
		progressDisplay->setText ("Please select files and directories to be restored.");
		if (doneSetup) *doneSetup = true;
	    }
	}
    }

private:
    // Ugly hack, but it works: fix segfault
    QString _text;
};

QLabel *XMCheckListItem::progressDisplay = 0;
bool    XMCheckListItem::selecting = false;
bool   *XMCheckListItem::doneSetup = 0;
int     XMCheckListItem::counter = 0;
int     XMCheckListItem::depth = 0;

struct XM_node 
{
    QString s;
    XM_node *firstChild;
    XM_node *nextSibling;
};

#define XMLF_OK       0
#define XMLF_CANTOPEN 1

// Requirements:
// - File must be sorted
// - Entry for a directory must come directly before entries for its files
void XM_load_filelist_sub (FILE *fp, QStringList first, XMCheckListItem *top, QLabel *status, int numlines)
{
    static int depth = 0;
    static int lino  = 0;
    bool tristated   = false;

    char line[4095];
    while (fgets (line, 4095, fp)) {
	bool chomped = false;

	if (line[strlen (line) - 1] == '\n') {
	    line[strlen (line) - 1] = 0;
	    chomped = true;
	}
	
	QStringList dirComponents = QStringList::split ("/", line);
	if (dirComponents.size() == 0) continue;
	if (depth > 0) {
	    bool good = true;
	    for (int i = 0; (i < dirComponents.size()) && (i < first.size()) && (i < depth); ++i)
		if (dirComponents[i] != first[i])
		    good = false;
	    if (!good) {
		fseek (fp, -(strlen (line) + chomped), SEEK_CUR); // equivalent of "pushback"
		return;
	    }
	}
	XMCheckListItem *newitem = new XMCheckListItem (top, dirComponents[dirComponents.size() - 1]);
	if (!(++lino % 1111)) {
	    status->setText (QString ("Loading filelist - %1% done").arg (lino * 100 / numlines));
	}

	depth++;
	XM_load_filelist_sub (fp, dirComponents, newitem, status, numlines);
	depth--;
    }
}

int XM_load_filelist (const char *filelist_fname, QListView *list, QLabel *status, QStringList first = QStringList(), FILE *fp = 0)
{
    fp = fopen (filelist_fname, "r");
    if (!fp) return XMLF_CANTOPEN;
    
    XMCheckListItem *top = new XMCheckListItem (list, "/");

    status->setText ("Loading filelist - 0% done");
    XM_load_filelist_sub (fp, QStringList(), top, status, atoi (call_program_and_get_last_line_of_output (const_cast <char*> (QString ("wc -l %1").arg (filelist_fname).ascii()))));

    fclose (fp);
    return XMLF_OK;
}

void *XMondoRestore_preparer_thread (void *arg) 
{
    XMondoRestore *r = static_cast<XMondoRestore*> (arg);
    int i = 0;
    g_current_media_number = 1;
	    
    r->fStatusMsg->setText ("Retrieving mondo-restore.cfg from archive...");

    struct s_bkpinfo *bkpinfo = r->bkpinfo;
    reset_bkpinfo (bkpinfo);
    switch (r->rMediaType->id (r->rMediaType->selected())) {
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
	break;
    case 5:
	bkpinfo->backup_media_type = nfs;
	break;
    case 6:
	bkpinfo->backup_media_type = tape;
	break;
    case 7:
	bkpinfo->backup_media_type = udev;
	break;
    }

    strcpy (bkpinfo->media_device, r->rDevice->text());
    if (bkpinfo->backup_media_type == nfs) strcpy (bkpinfo->nfs_mount, r->rDevice->text());
    if (bkpinfo->backup_media_type == nfs) strcpy (bkpinfo->nfs_remote_dir, r->rNFSRemoteDir->text());
    if (bkpinfo->backup_media_type == iso) strcpy (bkpinfo->isodir, r->rDevice->text());
    strcpy (bkpinfo->tmpdir, r->tempdir.ascii());

    setup_MR_global_filenames (bkpinfo);
    
    if (get_cfg_file_from_archive (bkpinfo) != 0) {
	r->fStatusMsg->setText ("Unable to retrieve mondo-restore.cfg. Please choose another archive source.");
	r->ok = false;
	return 0;
    }
    read_cfg_file_into_bkpinfo (g_mondo_cfg_file, bkpinfo);
    if (!does_file_exist (g_mondo_cfg_file)) {
	r->fStatusMsg->setText ("Unable to retrieve mondo-restore.cfg. Internal error.");
	r->ok = false;
	return 0;
    }
    if (!does_file_exist (g_filelist_full)) {
	chdir (bkpinfo->tmpdir);
	r->fStatusMsg->setText ("Retrieving filelist...");
	if ((bkpinfo->backup_media_type == tape) || (bkpinfo->backup_media_type == udev) ||
	    (bkpinfo->backup_media_type == cdstream)) {
	    unlink (g_filelist_full);
	    system (QString ("tar -zxf %1 tmp/filelist.full").arg (bkpinfo->media_device).ascii());
	} else {
	    mount_cdrom (bkpinfo);
	    unlink (g_filelist_full);
	    system ("tar -zxf /mnt/cdrom/images/all.tar.gz tmp/filelist.full");
	}

	if (!does_file_exist (g_filelist_full)) {
	    r->fStatusMsg->setText ("Filelist could not be retrieved!");
	    r->ok = false;
	    return 0;
	}
    }
    if (!r->rFilter->text().isEmpty() && !r->rFilter->text().isNull()) {
	r->fStatusMsg->setText (QString ("Filtering filelist through regexp <tt>%1</tt>...").arg (r->rFilter->text()));
	if (system (QString ("egrep '%2' %1 > %3.FILT").arg (g_filelist_full).arg (r->rFilter->text()).arg (g_filelist_full).ascii()) != 0) {
	    r->fStatusMsg->setText ("Filter failed, using whole filelist");
	    rename (g_filelist_full, QString ("%1.FILT").arg (g_filelist_full).ascii());
	    usleep (500000);
	}   
    } else {
	rename (g_filelist_full, QString ("%1.FILT").arg (g_filelist_full).ascii());
    }

    r->fStatusMsg->setText ("Preparing filelist - 0% done");

    int nlines = atoi (call_program_and_get_last_line_of_output (const_cast<char*> (QString ("wc -l %1.FILT").arg (g_filelist_full).ascii())));
    int curline = 0;
    FILE *fin  = fopen (QString ("%1.FILT").arg (g_filelist_full).ascii(), "r");
    FILE *fout = popen (QString ("sort -u > %1").arg (g_filelist_full).ascii(), "w");

    if (!(fin && fout)) {
	r->fStatusMsg->setText ("Can't open filelist");
	r->ok = false;
	return 0;
    }

    char line[4096], tmp[4096];
    while (fgets (line, 4096, fin)) {
	if (line[strlen (line) - 1] == '\n')
	    line[strlen (line) - 1] = '\0';
	
	for (int pos = 0; line[pos] != '\0'; pos++) {
	    if (line[pos] != '/') continue;
	    strcpy (tmp, line);
	    tmp[pos] = '\0';
	    if (strlen (tmp)) {
		fprintf (fout, "%s\n", tmp);
	    }
	}
	fprintf (fout, "%s\n", line);
	if (!(++curline % 1111)) {
	    r->fStatusMsg->setText (QString ("Preparing filelist - %1% done").arg (curline * 100 / nlines));
	}
    }

    fclose (fin);
    pclose (fout);
    
    if (XM_load_filelist (g_filelist_full, r->fList, r->fStatusMsg) != XMLF_OK) {
	r->fStatusMsg->setText ("Error loading filelist");
	r->ok = false;
	return 0;
    }
    r->fStatusMsg->setText ("Filelist loaded OK");

    r->doneSetup = true;
    r->fList->setEnabled (true);
    r->fRestoreDirLabel->setEnabled (true);
    r->fRestoreDir->setEnabled (true);
    sleep (1);
    r->fStatusMsg->setText ("Please select files and directories to be restored.");
    XMCheckListItem::progressDisplay = r->fStatusMsg;
    XMCheckListItem::doneSetup = &(r->doneSetup);

    return 0;
}

XMondoRestore::XMondoRestore (QWidget *parent, QButtonGroup *mediaType, QLineEdit *device, QLineEdit *nfsRemoteDir, QLineEdit *filelistFilter)
    : QObject (0, 0), rMediaType (mediaType), rDevice (device), rNFSRemoteDir (nfsRemoteDir), rFilter (filelistFilter), ok (true), files (parent), doneSetup (false), th (0)
{
    bkpinfo = new s_bkpinfo;

    char tmp[256];
    strcpy (tmp, "/tmp/xmondo.rstr.XXXXXX");
    mktemp (tmp);
    tempdir = tmp;
    filelistLocation = tempdir + "/filelist.full";
    cfgLocation      = tempdir + "/mondo-restore.cfg";
    cdMountpoint     = "/mnt/cdrom";

    if (!does_file_exist ("/mnt/cdrom")) {
	if (system ("mkdir -p /mnt/cdrom >/dev/null 2>&1") != 0) {
	    popup_and_OK ("Can't create /mnt/cdrom directory. Aborting restore.");
	    ok = false;
	    return;
	}
    }

    QGridLayout *filesGrid;
    filesGrid        = new QGridLayout (files, 3, 2, 5, 5, "filesGrid");
    fStatusMsg       = new QLabel ("", files);
    fList            = new QListView (files);
    fRestoreDirLabel = new QLabel ("Restore to:", files);
    fRestoreDir      = new QLineEdit (files);

    fList->addColumn ("Files to restore:");
    fList->setRootIsDecorated (true);
    fList->setEnabled (false);
    fRestoreDirLabel->setEnabled (false);
    fRestoreDir->setEnabled (false);
    fRestoreDir->setText ("/tmp");
    filesGrid->addMultiCellWidget (fStatusMsg, 0, 0, 0, 1);
    filesGrid->addMultiCellWidget (fList,      1, 1, 0, 1);
    filesGrid->addWidget (fRestoreDirLabel,    2,    0);
    filesGrid->addWidget (fRestoreDir,         2,    1);

    while (system ("mount | grep -q /mnt/cdrom") == 1) {
	if (QMessageBox::warning (0, "XMondo", QString ("CD is mounted. Please unmount it and try again."), "&Retry", "&Cancel") == 1 /* Cancel */) {
	    ok = false;
	    return;
	}
    }

    pthread_create (&preparer_thread, 0, XMondoRestore_preparer_thread, static_cast <void*> (this));
    ok = true;
}

XMondoRestore::~XMondoRestore() 
{
    chdir ("/");
    pthread_cancel (preparer_thread);
    system (QString ("mount | grep xmondo.rstr | cut -d' ' -f3 | xargs umount"));
    system (QString ("umount %1/mount.bootdisk >/dev/null 2>&1").arg (tempdir).ascii());
    if ((tempdir != "") && (tempdir != "/") && (tempdir != "/tmp") && (tempdir != "/tmp/")) {
	system (QString ("rm -rf %1").arg (tempdir).ascii());
    }
    system (QString ("umount %1 >/dev/null 2>&1").arg (cdMountpoint).ascii());
    delete bkpinfo;
}

int XM_save_filelist (QListViewItem *liTop, const char *fname, QString prefix = QString (""), int numlines = 0, int curline = 0, FILE *fp = 0)
{
    static int depth = 0;

    if (!depth) {
	numlines = atoi (call_program_and_get_last_line_of_output (const_cast<char*> (QString ("wc -l %1").arg (fname).ascii())));
	fp = fopen (fname, "w");
	if (!fp) return 1;
	fprintf (fp, "/\n");
	if (!liTop) {
	    popup_and_OK ("Internal error. No entries in filelist.");
	    return 255;
	}
    }

    XMCheckListItem *top = dynamic_cast<XMCheckListItem*> (liTop);
    if (!top) return 2;
    
    for (; top; top = dynamic_cast<XMCheckListItem*> (top->nextSibling())) {
	if (!top) return 2;
	if (top->isOn()) {
	    fprintf (fp, "%s/%s\n", prefix.ascii(), top->text().ascii());
	}
	if (!(++curline % 1000)) {
	    XMondoProgress->setProgress (curline * 100 / numlines);
	    kapp->processEvents();
	}
	if (top->firstChild()) {
	    depth++;
	    XM_save_filelist (top->firstChild(), "", prefix + "/" + top->text(), numlines, curline, fp);
	    depth--;
	}
    }

    if (!depth) {
	fclose (fp);
    }
    return 0;
}


void *RestoreThread__run (void *arg);

class RestoreThread
{
    friend void *RestoreThread__run (void *arg);
public:
    RestoreThread (struct s_bkpinfo *bkpinfo, struct s_node *filelist) : _bkpinfo (bkpinfo), _filelist (filelist),
    _ret (-1), _aborted (false) {}
    int returns() {
	return _ret;
    }
    void start() {
	_running = true;
	pthread_create (&_thr, 0, RestoreThread__run, this);
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
	RestoreThread *rt = static_cast <RestoreThread*> (arg);
	if (!rt) return;
	rt->_running = false;
	rt->_ret = -1;
	rt->_aborted = true;
    }
    void run() {
	pthread_cleanup_push (RestoreThread::setStop, this);

	log_to_screen ("Restore started on %s", call_program_and_get_last_line_of_output ("date"));
	_ret = restore_everything (_bkpinfo, _filelist);
	log_to_screen ("Restore finished on %s", call_program_and_get_last_line_of_output ("date"));
	pthread_cleanup_pop (FALSE);
	_running = false;
    }
    struct s_bkpinfo *_bkpinfo;
    struct s_node *_filelist;
    int _ret;
    pthread_t _thr;
    bool _running;
    bool _aborted;
};

void *RestoreThread__run (void *arg)
{
    RestoreThread *rt = static_cast <RestoreThread *> (arg);
    if (!rt && (sizeof(void*) >= 4)) return (void *) 0xDEADBEEF;
    rt->run();
}

void XMondoRestore::slotAbortRestore() 
{
    if (th) th->terminate();
}

void XM_toggle_everything_on (struct s_node *flist) 
{
    for (; flist; flist = flist->right) {
	if (flist->ch == 0) {
	    flist->selected = 1;
	}
	if (flist->down) {
	    XM_toggle_everything_on (flist->down);
	}
    }
}

void XMondoRestore::go()
{
    if (!doneSetup) {
	popup_and_OK ("Please wait for the setup to be completed.");
	ok = false;
	return;
    }
    pthread_cancel (preparer_thread);


    disconnect (XMondoCancel, SIGNAL(clicked()), 0, 0);
    connect (XMondoCancel, SIGNAL(clicked()), this, SLOT(slotAbortRestore()));

    XMondoProgress->show();
    XMondoProgress->setTotalSteps (100);
    XMondoProgress->setProgress (0);
    XMondoProgressWhat->show();
    XMondoProgressWhat->setText ("Saving your filelist choices.");
    XMondoProgressWhat2->hide();
    XMondoProgressWhat3->hide();
    XMondoTimeTaken->hide();
    XMondoTimeToGo->hide();
    XMondoStatus->setText ("Saving filelist");
    XMondoStatus->show();
    XMondoLog->setText ("");
    XMondoLog->show();

    if (XM_save_filelist (fList->firstChild()->firstChild(), g_filelist_full) != 0) {
	XMondoStatus->setText ("Error saving filelist");
	ok = false;
	return;
    }

    XMondoStatus->setText ("Reloading filelist");
    /*DEBUG*/ system (QString ("cp %1 /home/oremanj/filelist.tested").arg (g_filelist_full).ascii());
    s_node *flist = load_filelist (g_filelist_full);
    if (!flist) {
	XMondoStatus->setText ("Error loading filelist");
	ok = false;
	return;
    }
    XM_toggle_everything_on (flist);

    XMondoProgress->hide();
    XMondoStatus->setText ("Beginning restore");
    XMondoProgressWhat->hide();

    strcpy (bkpinfo->restore_path, fRestoreDir->text().ascii());

    g_text_mode = 1; // avoid crashing on NEWT functions

    th = new RestoreThread (bkpinfo, flist);
    th->start();
    while (th->running()) {
	usleep (100000);
	events.send();
	kapp->processEvents();
    }
    free_filelist (flist);
    if (th->aborted()) {
	/* do nothing */
    } else if (th->returns() == 0) {
	popup_and_OK ("Restore completed with no errors.");
    } else {
	popup_and_OK ("Restore completed; however, there were some errors.");
    }
    delete th; th = 0;
    ok = true; // call destructor in caller
}
