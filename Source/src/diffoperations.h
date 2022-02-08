#ifndef Header_Diff_Operations
#define Header_Diff_Operations


#include "diff/diff_match_patch.h"

#include "Latex/Document.hpp"

class DiffOp {

	public:
		DiffOp();

		enum DiffType {
			Insert,
			Delete,
			Replace,
			Deleted,
			Inserted,
			Replaced,
			NotInserted,
			NotDeleted,
			NotReplaced
		};

		int start, length;
		bool lineWasModified;

		QDocumentLineHandle * dlh;
		DiffType type;
		QString text;

};


using DiffList = QList<DiffOp>;


Q_DECLARE_METATYPE(DiffList)


class QDocumentCursor;


void diffRemoveMarkers(LatexDocument *,bool theirs);
void diffChangeOpType(QDocumentCursor range,DiffOp::DiffType);
void diffChange(LatexDocument *,int ln,int col,bool theirs);
void diffMerge(LatexDocument *);
void diffDocs(LatexDocument *,LatexDocument *,bool dontAddLines = false);

QDocumentCursor diffSearchBoundaries(LatexDocument *,int ln,int col,int fid,int direction = 0);
QString diffCollectText(QDocumentCursor range);


#endif
