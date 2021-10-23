#ifndef Header_MacroBrowser_UI
#define Header_MacroBrowser_UI


#include "mostQtHeaders.h"
#include "configmanager.h"
#include <QNetworkAccessManager>
#include <QPlainTextEdit>
#include "usermacro.h"


/*!
 * \brief provide simple UI to browse macros from github.com/texstudio-org/texstudio-macro
 */

class MacroBrowserUI : public QDialog {

    Q_OBJECT

    public:

        MacroBrowserUI(QWidget * parent=nullptr);
        ~MacroBrowserUI();

        QList<Macro> getSelectedMacros();

    private slots:

        void onRequestError();
        void onRequestCompleted();
        void requestMacroList(const QString & path = "",const bool & directURL = false);
        void itemClicked(QTableWidgetItem *);

    protected:

        QDialogButtonBox * buttonBox;
        QPlainTextEdit * teDescription;
        QTableWidget * tableWidget;
        QLineEdit * leName;

        QHash<QString,QList<QTableWidgetItem *>> itemCache;
        QHash<QString,QString> cache;
        QString currentPath;

        ConfigManager *config;

        QNetworkAccessManager * networkManager;

};

#endif
