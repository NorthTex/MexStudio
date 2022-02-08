#ifndef Tests_Prepare
#define Tests_Prepare


#define testclass(name)                 \
    namespace Test { class name; };     \
    class Test:: name : public QObject

#define testcase(name)                  \
    private slots:                      \
        void name (void);

#endif
