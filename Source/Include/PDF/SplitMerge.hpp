#ifndef Header_PDF_SplitMerge
#define Header_PDF_SplitMerge


#include "PDF/MultiProcessX.hpp"


using PageRange = QPair<int,int>;


class PDFSplitMerge: public MultiProcessX {

	public:

		virtual void splitMerge(
            const QString & outputFile,
            const QList<QPair<QString,QList<PageRange>>> & inputs
        );

		virtual void split(
            const QString & outputFile,
            const QString & inputFile,
            const QList<PageRange> & range
        );
		
        virtual void split(
            const QString & outputFile,
            const QString & inputFile,
            const PageRange & range
        );

		virtual void merge(
            const QString & outputFile,
            const QStringList & inputFiles
        );
};


#endif
