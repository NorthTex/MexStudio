#ifndef Header_PDF_SplitMergePDFPages
#define Header_PDF_SplitMergePDFPages


#include "PDF/SplitMerge.hpp"


using PageRange = QPair<int,int>;


class PDFSplitMergePDFPages : public PDFSplitMerge {

	public:

		virtual void splitMerge(
            const QString & outputFile,
            const QList<QPair<QString,QList<PageRange>>> & inputs
        );
};


#endif
