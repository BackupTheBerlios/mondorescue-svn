/***************************************************************************
                          xmondobackup.h  -  description
                             -------------------
    begin                : Mon Apr 28 2003
    copyright            : (C) 2003 by Joshua Oreman
    email                : oremanj@get-linux.org
    cvsid                : $Id: xmondobackup.h,v 1.1 2004/06/10 16:13:06 hugo Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef XMONDOBACKUP_H
#define XMONDOBACKUP_H

#include <qdialog.h>
#include "xmondo.h"

/**The class that does the nuts and bolts of the backup routine.
  *@author Joshua Oreman
  */

class BackupThread;
class XMondoBackup : public QObject {
   Q_OBJECT
public: 
	XMondoBackup();
	~XMondoBackup();
	int run (struct s_bkpinfo *bkpinfo);
	int compare (struct s_bkpinfo *bkpinfo);

public slots:
    void abortBackup(); 

private:
    BackupThread *th;
};

#endif
