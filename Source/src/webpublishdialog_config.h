#ifndef Header_WebPublish_Dialog_Config
#define Header_WebPublish_Dialog_Config

#include <QSettings>


class WebPublishDialogConfig {

	public:

		int userwidth, compil, tocdepth, startindex, navigation;
		bool noindex;

		QString title, address, browser, contentname, align, lastdir, dviopt;

		void readSettings(QSettings &settings);
		void saveSettings(QSettings &settings);
};


#endif
