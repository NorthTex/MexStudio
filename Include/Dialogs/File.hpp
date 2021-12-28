#ifndef Header_File_Dialog
#define Header_File_Dialog


#include <QWidget>
#include <QString>
#include <QStringList>


using String = const QString &;


class FileDialog {

    public:

        #define parameter                           \
            QWidget * parent = nullptr,             \
            String caption = QString(),             \
            String dir = QString(),                 \
            String filter = QString(),              \
            QString * selectedFilter = nullptr,     \
            int options = 0


        static QString getOpenFileName(parameter);
        static QString getSaveFileName(parameter);
        static QStringList getOpenFileNames(parameter);


        #undef parameter

};

#endif
