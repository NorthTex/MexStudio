#ifndef Header_CodeSnippet
#define Header_CodeSnippet


#include "mostQtHeaders.h"


class QDocumentCursor;
class QEditor;
class Texstudio;


enum CompletionType { CT_COMMANDS , CT_NORMALTEXT , CT_CITATIONS , CT_LABELS };


/*!
 * \brief store information for placeholder
 */

struct CodeSnippetPlaceHolder {

	int offset /*< column position*/, length /*< lenght*/;
	int id; ///< identification number to match mirror placeholders

	enum Flag {
		None=0,
		AutoSelect = 1,
		Mirrored = 2,
		Mirror = 4,
		PreferredMultilineAutoSelect = 8,
		Persistent = 16,
		Translatable = 32,
		PreferredCutInsertion = 64
	};

	int flags; ///< type of placeholder
	int offsetEnd() const; ///< return end-column of placeholder = offset+lenght

};


/*!
 * \class CodeSnippet
 * \brief contains data for command completion
 *
 * Handles all information for command completion, especially sort-order, placeholder handling including mirror-placeholders and final cursor positioning
 * Usually these information is encoded via special comments, see manual
 */

class CodeSnippet {

	public:

		CodeSnippet(const QString & newWord,bool replacePercentNewline = true); ///< generate codesnippet from text string

		CodeSnippet()
			: cursorLine(-1)
			, cursorOffset(-1)
			, anchorOffset(-1)
			, usageCount(0)
			, index(0)
			, snippetLength(0)
			, score(0)
			, type(none) {} ///< generate empty codesnippet

		CodeSnippet(const CodeSnippet & cw)
			: word(cw.word)
			, sortWord(cw.sortWord)
			, lines(cw.lines)
			, cursorLine(cw.cursorLine)
			, cursorOffset(cw.cursorOffset)
			, anchorOffset(cw.anchorOffset)
			, placeHolders(cw.placeHolders)
			, usageCount(cw.usageCount)
			, index(cw.index)
			, snippetLength(cw.snippetLength)
			, score(cw.score)
			, type(cw.type)
			, name(cw.name) {} ///< copy constructor

		CodeSnippet & operator = (const CodeSnippet &) = default;	// Avoid GCC9 -Wdeprecated-copy warning

		bool operator < (const CodeSnippet &) const; ///< define sorting operator
		bool operator == (const CodeSnippet &) const;

		enum PlaceholderMode { PlacehodersActive , PlaceholdersToPlainText , PlaceholdersRemoved }; ///< configuaration of placeholder handling

		QString word; ///< displayed snippet name
		QString sortWord; ///< for sorting used snippet name, allows change of sort-order by replacement special characters especially braces
		QStringList lines; ///< to be inserted code

		//TODO: Multirow selection
		int cursorLine;  ///< placement of cursor after insert concerning line of codesnippet; -1 => not defined
		int cursorOffset; ///< placement of cursor after insert concerning column; -1 => not defined
		int anchorOffset; ///< placement of cursor anchor after insert

		QList<QList<CodeSnippetPlaceHolder> > placeHolders; //used to draw


		uint index; ///< hash index for usage count identification

		int usageCount; ///< usage count for favourite detection
		int snippetLength; ///< number of characters
		int score;

		enum Type { none , length , userConstruct };

		Type type;

		QString environmentContent(const QString & envName);
		QString expandCode(const QString & code);

		void insertAt(QEditor *,QDocumentCursor *,PlaceholderMode = PlacehodersActive,bool byCompleter = false,bool isKeyVal = false) const;
		void insert(QEditor *) const; ///< insert snippet text into given editor at current cursor position

		void setName(const QString & newName); ///< set codesnippet name, used for usertags
		QString getName() const; ///< get codesnippet name, used for usertags

		static bool autoReplaceCommands;
		static bool debugDisableAutoTranslate;

	private:

		QString name;

		QDocumentCursor getCursor(QEditor *,const CodeSnippetPlaceHolder &,int snippetLine,int baseLine,int baseLineIndent,int lastLineRemainingLength) const;

};


Q_DECLARE_METATYPE(CodeSnippet)
Q_DECLARE_METATYPE(CodeSnippet::PlaceholderMode)


/*!
 * \brief special list class for codesnippets
 *
 * The method for combining are optmized for sorted lists
 */

class CodeSnippetList: public QList<CodeSnippet> {

	public:

		void insert(const QString & elem); ///< insert one element into list at correct position to maintain sorting
		void unite(const QList<CodeSnippet> &); ///< merge two list (both need to be sorted !)
		void unite(CodeSnippetList &); ///< merge two list (both need to be sorted !)

		CodeSnippetList::iterator insert(CodeSnippetList::iterator before,const CodeSnippet & cs){
			return QList<CodeSnippet>::insert(before, cs);
		}
};


#endif
