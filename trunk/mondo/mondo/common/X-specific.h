/* X-specific.h */

#ifdef __cplusplus
extern "C" {
#endif

	int ask_me_yes_or_no(char *prompt);
	int ask_me_OK_or_cancel(char *prompt);
	void close_evalcall_form(void);
	void close_progress_form();
	void fatal_error(char *error_string);
	void fatal_error_sub(char *error_string);
	void finish(int signal);
	void mvaddstr_and_log_it(int y, int x, char *output);
	void log_file_end_to_screen(char *filename, char *grep_for_me);
	void log_to_screen(const char *op, ...);
	void open_evalcall_form(char *title);
	void open_progress_form(char *title, char *b1, char *b2, char *b3,
							long max_val);
	void popup_and_OK(char *prompt);
	void popup_and_OK_sub(char *prompt);
	int popup_and_get_string(char *title, char *b, char *output,
							 int maxsize);
	int popup_and_get_string_sub(char *title, char *b, char *output,
								 int maxsize);
	int popup_with_buttons(char *p, char *button1, char *button2);
	int popup_with_buttons_sub(char *p, char *button1, char *button2);
	void refresh_log_screen();
	void setup_newt_stuff();
	void update_evalcall_form_ratio(int num, int denom);
	void update_evalcall_form(int curr);
	void update_progress_form(char *blurb3);
	void update_progress_form_full(char *blurb1, char *blurb2,
								   char *blurb3);






	t_bkptype which_backup_media_type(bool);
	int which_compression_level();

	void popup_chaneglist_from_file(char *source_file);

#if __cplusplus && WITH_X

	extern int g_result_of_last_event;

}								/* extern "C" */
#include <qvaluelist.h>
#include <qwidget.h>
#include <qprogressbar.h>
#include <qlabel.h>
#include <qstring.h>
#include <qmultilineedit.h>
/**
 * A class for XMondo to hold events queued by the backup thread until the event thread is able to handle them.
 */ class XMEventHolder
{
  public:
	struct Event {
		enum EventType { None, Show, Hide, New, SetProgress, SetTotal,
			SetText, InsLine, PopupWithButtons, InfoMsg, ErrorMsg,
			GetInfo
		} type;
		QWidget *data;
// union {
		int iParam;
		QString qsParam, title, text, b1, b2;
		char *csParam;
		int len;
// };

		 Event():type(None), data(0) {
		} Event(EventType thetype, QWidget * thedata) {
			this->type = thetype;
			this->data = thedata;
		} Event(EventType thetype, QWidget * thedata, int ip) {
			this->type = thetype;
			this->data = thedata;
			this->iParam = ip;
		}
		Event(EventType thetype, QWidget * thedata, QString qsp) {
			this->type = thetype;
			this->data = thedata;
			this->qsParam = qsp;
		}
		Event(EventType thetype, QWidget * thedata, char *csp) {
			this->type = thetype;
			this->data = thedata;
			this->csParam = csp;
		}
		Event(EventType thetype, QWidget * thedata, char *csp, int len) {
			this->type = thetype;
			this->data = thedata;
			this->csParam = csp;
			this->len = len;
		}
	};

	XMEventHolder() {
	}

	/* Backup thread functions */

	void event(Event::EventType type, QWidget * data) {
		_events.push_back(Event(type, data));
	}

	/**
     * Queue a "show" event for @p data.
     * This is equivalent to a delayed call of @p data->show().
     * @param data The widget to show when the event is processed.
     */
	void show(QWidget * data) {
		_events.push_back(Event(Event::Show, data));
	}

	/**
     * Queue a "hide" event for @p data.
     * This is equivalent to a delayed call of @p data->hide().
     * @param data The widget to hide when the event is processed.
     */
	void hide(QWidget * data) {
		_events.push_back(Event(Event::Hide, data));
	}

	/**
     * Queue a "setProgress" event for @p data.
     * This is equivalent to a delayed call of <tt>data-\>setProgress(progress)</tt>.
     * @param data The progress bar widget to set the progress of when the event is processed.
     * @param progress The progress amount to set it to.
     */
	void setProgress(QProgressBar * data, int progress) {
		_events.push_back(Event(Event::SetProgress, data, progress));
	}

	/**
     * Queue a "setTotalSteps" event for @p data.
     * This is equivalent to a delayed call of <tt>data-\>setTotalSteps(totals)</tt>.
     * @param data The progress bar widget to set the total steps of when the event is processed.
     * @param totals The total number of steps to set.
     */
	void setTotalSteps(QProgressBar * data, int totals) {
		_events.push_back(Event(Event::SetTotal, data, totals));
	}

	/**
     * Queue a "setText" event for @p data.
     * This is equivalent to a delayed call of <tt>data-\>setText(text)</tt>.
     * @param data The label widget to set the text of.
     * @param text The text to set it to.
     */
	void setText(QLabel * data, QString text) {
		_events.push_back(Event(Event::SetText, data, text));
	}

	/**
     * Queue an "insertLine" event for @p data.
     * This is equivalent to a delayed call of <tt>data->insertLine(line)</tt>.
     * @param data The edit box to add the line to.
     * @param line The line to add.
     */
	void insertLine(QMultiLineEdit * data, QString line) {
		_events.push_back(Event(Event::InsLine, data, line));
	}

	/**
     * Queue an alert box with two buttons to be displayed.
     * The button pushed (the return value of popup_with_buttons()) will be stored
     * in @p g_result_of_last_event.
     * @param text The text of the popup dialog box.
     * @param b1 The first button's text.
     * @param b2 The second button's text.
     */
	void popupWithButtons(QString text, QString b1, QString b2) {
		Event e;
		e.type = Event::PopupWithButtons;
		e.text = text;
		e.b1 = b1;
		e.b2 = b2;
		_events.push_back(e);
	}

	/**
     * Queue an info box with one OK button to be displayed.
     * @param text The text of the dialog box.
     */
	void infoMsg(QString text) {
		Event e;
		e.type = Event::InfoMsg;
		e.text = text;
		_events.push_back(e);
	}

	/**
     * Queue a "fatal error" message.
     * @param text The fatal error.
     */
	void errorMsg(QString text) {
		Event e;
		e.type = Event::ErrorMsg;
		e.text = text;
		_events.push_back(e);
	}

	/**
     * Queue a request for some information from the user.
     * If you want to wait until the text is stored, you can set @p g_result_of_last_event
     * to -1 and busy-wait while it's equal to that. If the user pushes OK 1 will be stored,
     * otherwise 0 will be stored.
     * @param title The title of the dialog box.
     * @param text The text of the dialog box.
     * @param out Where to put the user's reply.
     * @param len The size of the buffer allocated for @p out.
     */
	void getInfo(QString title, QString text, char *out, int len) {
		Event e;
		e.type = Event::GetInfo;
		e.title = title;
		e.text = text;
		e.csParam = out;
		e.len = len;
		_events.push_back(e);
	}

	/* These are called in the GUI thread */

	/**
     * Clear all events stored in the queue without executing them.
     */
	void clear() {
		_events.erase(_events.begin(), _events.end());
	}

	/**
     * Process all events stored in the queue and then clear them.
     */
	void send() {
		QProgressBar *pb;
		QLabel *l;
		QMultiLineEdit *mle;
		for (QValueList < Event >::iterator it = _events.begin();
			 it != _events.end(); ++it) {
			switch ((*it).type) {
			case Event::Show:
				((*it).data)->show();
				break;
			case Event::Hide:
				((*it).data)->hide();
				break;
			case Event::SetProgress:
				if ((pb = dynamic_cast < QProgressBar * >((*it).data))) {
					pb->setProgress((*it).iParam);
				}
				break;
			case Event::SetTotal:
				if ((pb = dynamic_cast < QProgressBar * >((*it).data))) {
					pb->setTotalSteps((*it).iParam);
				}
				break;
			case Event::SetText:
				if ((l = dynamic_cast < QLabel * >((*it).data))) {
					l->setText((*it).qsParam);
				}
				break;
			case Event::InsLine:
				if ((mle = dynamic_cast < QMultiLineEdit * >((*it).data))) {
					mle->insertLine((*it).qsParam);
				}
				break;
			case Event::PopupWithButtons:
				g_result_of_last_event =
					popup_with_buttons_sub(const_cast <
										   char *>((*it).text.ascii()),
										   const_cast <
										   char *>((*it).b1.ascii()),
										   const_cast <
										   char *>((*it).b2.ascii()));
				break;
			case Event::InfoMsg:
				popup_and_OK_sub(const_cast < char *>((*it).text.ascii()));
				g_result_of_last_event = 0;
				break;
			case Event::ErrorMsg:
				fatal_error_sub(const_cast < char *>((*it).text.ascii()));
				break;
			case Event::GetInfo:
				g_result_of_last_event =
					popup_and_get_string_sub(const_cast <
											 char *>((*it).title.ascii()),
											 const_cast <
											 char *>((*it).text.ascii()),
											 (*it).csParam, (*it).len);
				break;
			default:
				qDebug("unknown event\n");
			}
		}
		this->clear();
	}

	/* Undocumented */
	QValueList < Event > events() {
		return _events;
	}

  protected:


	QValueList < Event > _events;
};

#endif							/* __cplusplus */
