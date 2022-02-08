#ifndef Test_UserMacro
#define Test_UserMacro
#ifndef QT_NO_DEBUG

#include "mostQtHeaders.h"
#include <QtTest/QtTest>
#include "Test.hpp"


testclass(UserMacro){

    Q_OBJECT;

    private:

        QString fileName;

    public:

        explicit UserMacro(QObject * parent = nullptr);

    testcase( saveRead_data );
    testcase( saveRead );
};


#endif
#endif