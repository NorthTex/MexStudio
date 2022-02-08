#ifndef Header_BibTex_Reader
#define Header_BibTex_Reader


#include "smallUsefulFunctions.h"


namespace BibTex { class Reader; }


class BibTex::Reader : public SafeThread {

    Q_OBJECT

    public:

        explicit Reader(QObject * parent = 0);

    signals:

        void sectionFound(QString content);

    public slots:

        void searchSection(QString file,QString id,int truncateLimit = 150);

};


#endif
