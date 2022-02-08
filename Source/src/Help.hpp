#ifndef Header_Help
#define Header_Help


#include <QObject>
#include <QProcess>


#define String const QString &
#define Package String
#define CommandLine String
#define Packages const QStringList &
#define ExitStatus QProcess::ExitStatus


class TexHelp : public QObject {

    Q_OBJECT

    private:

        int texDocSystem;

    private:

        QString runTexdoc(QString args);

        bool runTexdocAsync(QString args,const char * finishedCMD);

   public:

        explicit TexHelp(QObject * parent = nullptr);

   public:

        QString packageDocFile(Package,bool silent = false);

        bool isTexdocExpectedToFinish();
        bool isMiktexTexdoc();

   public slots:

        void texdocAvailableRequestFinished(int,QProcess::ExitStatus);
        void texdocAvailableRequest(Package);
        void execTexdocDialog(Packages,Package defaultPack);
        void viewTexdoc(QString package);

   signals:

        void texdocAvailableReply(Package,bool available,QString error);
        void runCommandAsync(CommandLine,const char * returnCommand);
        void statusMessage(String);
        void runCommand(CommandLine,QString * output);

};


#undef String
#undef Package
#undef CommandLine
#undef Packages
#undef ExitStatus


#endif
