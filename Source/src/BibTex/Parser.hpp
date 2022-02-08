#ifndef Header_BibTex_Parser
#define Header_BibTex_Parser


#include <QString>
#include <QTextCodec>
#include <QSet>
#include <QFileInfo>
#include <QByteArray>


namespace BibTex { class FileInfo; }


class BibTex::FileInfo {

    private:

        using Info = const QFileInfo &;
        using Bytes = QByteArray &;

    private:

        void load(Info);

    protected:

        void parse(Bytes);

    public:

        QTextCodec * codec;
        QDateTime lateModified;
        QString linksTo;
        QSet<QString> ids;

    public:

        bool loadIfModified(Info);

};


#endif
