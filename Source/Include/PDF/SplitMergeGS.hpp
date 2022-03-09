#ifndef Header_PDF_SplitMergeGS
#define Header_PDF_SplitMergeGS


#include "PDF/SplitMerge.hpp"


using PageRange = QPair<int,int>;


class PDFSplitMergeGS: public PDFSplitMerge {

	public:

		PDFSplitMergeGS();

    public:
    
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
