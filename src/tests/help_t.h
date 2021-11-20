#ifndef Test_Help
#define Test_Help
#ifndef QT_NO_DEBUG


#include "mostQtHeaders.h"
#include "Help.hpp"
#include "testutil.h"
#include "buildmanager.h"
#include <QtTest/QtTest>
#include "Test.hpp"


testclass(Help){

	Q_OBJECT

	private:

		TexHelp helper;
		BuildManager * builder;

	public:

		Help(BuildManager * builder)
			: builder(builder){

			connect(& helper,
				SIGNAL(runCommand(QString,QString *)),this,
				SLOT(runCommand(QString,QString *)));
		}

	public slots:

		bool runCommand(QString commandline,QString * buffer){

			commandline.replace('@', "@@");
			commandline.replace('%', "%%");
			commandline.replace('?', "??");

			// path is used to force changing into the tmp dir (otherwise git status does not work properly)
			return builder -> runCommand(commandline,QFileInfo(),QFileInfo(),0,buffer,nullptr,buffer);
		}

	private slots:

		testcase( packageDocFile_data );
		testcase( packageDocFile );

};


#endif
#endif
