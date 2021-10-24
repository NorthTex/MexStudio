#ifndef Header_Bibtex_Dialog
#define Header_Bibtex_Dialog

#include "mostQtHeaders.h"
#include "BibTex/Type.hpp"

namespace Ui { class BibTeXDialog; }

//struct BibTeXType;

using StringMap = const QMap<QString,QString> &;
using StringList = const QStringList &;
using String = const QString &;
using Type = const BibTex::Type &;
using BibTeXType = BibTex::Type;


//struct BibTeXType {

//	QString name;
//	QString description;
//	QStringList mandatoryFields, optionalFields;

//	BibTeXType(Type other)
//		: name(other.name)
//		, description(other.description)
//		, mandatoryFields(other.mandatoryFields)
//		, optionalFields(other.optionalFields) {}

//	BibTeXType(String description,String name,StringList mandatory,StringList optionals)
//		: name(name)
//		, description(description)
//		, mandatoryFields(mandatory)
//		, optionalFields(optionals) {}

//	BibTeXType & operator = (Type other) = default; // Silence -Wdeprecated-copy

//};


using Types = QList<BibTex::Type>;


class BibTeXDialog : public QDialog {

	Q_OBJECT
	Q_DISABLE_COPY(BibTeXDialog)

	public:

		enum BibType {BIBTEX, BIBLATEX};

		explicit BibTeXDialog(QWidget * parent = 0,StringList fileList = QStringList(),int curFile = -1,String defId = "");
		virtual ~BibTeXDialog();

		QString resultString;
		int resultFileId; //-1 for new, 0..n for files from fileList, -2 for none

		static void setBibType(BibType type);
		static QString textToInsert(Type entry,bool keepOptionalFields,StringMap fields);
		static QString textToInsert(String entryName);
		static QList<BibTex::Type> getPossibleEntryTypes(BibType type);


	protected:

		virtual void changeEvent(QEvent * event);
		virtual void accept();


	private slots:

		void typeSelectionChanged();


	private:

		Ui::BibTeXDialog * m_ui;

		static void needEntryTypes();
		static void generateBibtexEntryTypes(bool forceRecreate = false);
		static void generateBiblatexEntryTypes(bool forceRecreate = false);

		static Types * entryTypes;
		static Types bibtexEntryTypes;
		static Types biblatexEntryTypes;

		static BibType bibType;

};

#endif
