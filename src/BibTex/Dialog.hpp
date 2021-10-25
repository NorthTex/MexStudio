#ifndef Header_BibTex_Dialog
#define Header_BibTex_Dialog


#include "BibTex/Type.hpp"
#include "BibTex/UI.hpp"
#include <QDialog>


namespace BibTex { class Dialog; }


class BibTex::Dialog : public QDialog {

    Q_OBJECT
    Q_DISABLE_COPY(Dialog)

    private:

        using TexType = BibTex::Type;
        using Types = QList<BibTex::Type>;
        using string = const QString &;
        using strings = const QStringList &;
        using StringMap = const QMap<QString,QString> &;

    public:

        enum Type { BibTex , BibLatex };

    private:

        UI * ui;

        static void generateBiblatexEntryTypes(bool forceRecreate = false);
        static void generateBibtexEntryTypes(bool forceRecreate = false);
        static void needEntryTypes();

        inline static Type type;
        inline static Types * entries;
        inline static Types bibtex , biblatex;

    private slots:

        void typeSelectionChanged();

    protected:

        virtual void changeEvent(QEvent *);
        virtual void accept();

    public:

        static void setType(Type);

        static QString textToInsert(const TexType & entry,bool keepOptional,StringMap fields);
        static QString textToInsert(string entryName);

        static QList<BibTex::Type> entryTypesFor(Type);

    public:

        explicit Dialog(QWidget * parent = 0,
                        strings files = QStringList(),
                        int currentFile = -1,
                        string defaultId = "");

        virtual ~ Dialog();

    public:

        QString result;
        int fileId; // -1 : new , -2 : none

};


#endif
