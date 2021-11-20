#ifndef Test_Git
#define Test_Git
#ifndef QT_NO_DEBUG


#include "git.h"
#include "buildmanager.h"
#include "Test.hpp"


testclass(Git){

    Q_OBJECT

    private:

        GIT git;
        QString path;
        BuildManager * builder;

        bool useTests;

    public:

        Git(BuildManager * builder,bool useTests)
            : builder(builder)
            , useTests(useTests) {

            connect(& git,
                SIGNAL(runCommand(QString,QString *)),this,
                SLOT(runCommand(QString,QString *)));
        }

    public slots:

        bool runCommand(QString commandline,QString * buffer);

    testcase( basicFunctionality );


};


#endif
#endif
