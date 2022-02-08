#ifndef Header_BibTex_UI
#define Header_BibTex_UI


#include <QDialog>
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>


namespace BibTex { class UI; }


class BibTex::UI {

    private:

        using Label = QLabel;
        using Grid = QGridLayout;
        using List = QListWidget;
        using Table = QTableWidget;
        using Item = QTableWidgetItem;
        using Size = QSizePolicy;
        using Checkbox = QCheckBox;
        using Button = QDialogButtonBox;

    public:

        QDialog * dialog;

        Label
            * label_insert,
            * label_fields,
            * label_type;

        List
            * files,
            * types;

        Item
            * item_field,
            * item_value;

        Checkbox * checkbox;
        Table * fields;

    public:

        void setup(QDialog *);
        void translate();

};


#endif
