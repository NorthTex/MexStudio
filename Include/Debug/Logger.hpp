#ifndef Debug_Logger
#define Debug_Logger
#ifdef DEBUG_LOGGER


class DebugLogger {

    private:

        DebugLogger(void);

        static FILE * m_logHandle;

        #ifndef QT_NO_DEBUG
            static void qMessageHandler(QtMsgType,const QMessageLogContext &,const QString & message);
        #endif

    public:

        static bool start(const QString & path);
        static bool isLogging(void);
        static bool stop(void);

        static void logMessage(QtMsgType,const char * file,unsigned int line,const QString & message);
        static void logMessage(QtMsgType,const char * file,unsigned int line,const char * message);

};


#endif
#endif