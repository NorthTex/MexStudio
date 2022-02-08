
#include <qtsingleapplication.h>
#include "TexStudio.hpp"
#include <functional>
#include <QSplashScreen>


class App : public QtSingleApplication {

    public:

        QString fileToLoadOnceRunning;
        Texstudio * studio = nullptr;

        bool running = false;

    private:

        void showSplashScreen(std::function<void(QSplashScreen *)>);
        void handleFileOpen(QFileOpenEvent *);

        void registerShutdown();
        void registerIntercom();


    protected:

        bool event(QEvent * event);

    public:

        App(int & count,char ** arguments);
        ~App();

    public:

        void startup(QStringList & commandLine);
        void instructOthers(const QStringList & commandLine);
        int run();
};

