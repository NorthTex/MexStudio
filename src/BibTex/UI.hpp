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

        Checkbox * checkbox;
        Table * fields;

    public:

        void setup(QDialog * dialog){

            this -> dialog = dialog;

            dialog -> resize(767,469);

            Grid * grid = new Grid(dialog);

            label_insert = new Label(dialog);
            grid -> addWidget(label_insert,1,0,1,1);

            label_type = new Label(dialog);
            grid -> addWidget(label_type,1,1,1,1);

            files = new List(dialog);
            grid -> addWidget(files,3,0,2,1);

            types = new List(dialog);
            grid -> addWidget(types,3,1,2,1);

            Button * button = new Button(dialog);
            button -> setOrientation(Qt::Horizontal);
            button -> setStandardButtons(Button::Cancel | Button::Ok);
            grid -> addWidget(button,6,0,1,3);

            label_fields = new Label(dialog);
            grid -> addWidget(label_fields,1,2,1,1);

            fields = new Table(dialog);
            fields -> setColumnCount(2);

            Item * item_field = new Item();
            fields -> setHorizontalHeaderItem(0,item_field);

            Item * item_value = new Item();
            fields -> setHorizontalHeaderItem(1,item_value);

            Size size(Size::MinimumExpanding,Size::Expanding);
            size.setHorizontalStretch(0);
            size.setVerticalStretch(0);
            size.setHeightForWidth(fields -> sizePolicy().hasHeightForWidth());

            fields -> setSizePolicy(size);
            fields -> horizontalHeader() -> setStretchLastSection(true);

            grid -> addWidget(fields,3,2,1,1);

            checkbox = new Checkbox(dialog);
            grid -> addWidget(checkbox,4,2,1,1);

            grid -> setColumnStretch(0,1);
            grid -> setColumnStretch(1,1);
            grid -> setColumnStretch(2,2);

            translate();

            QObject::connect(button,& Button::accepted,dialog,qOverload<>(& QDialog::accept));
            QObject::connect(button,& Button::rejected,dialog,qOverload<>(& QDialog::reject));

            QMetaObject::connectSlotsByName(dialog);
        }

    void translate(){
        label_insert -> setText(QCoreApplication::translate("BibTex::Dialog","Insert in File:",nullptr));
        label_type -> setText(QCoreApplication::translate("BibTex::Dialog","Entry Type:",nullptr));
        label_fields -> setText(QCoreApplication::translate("BibTex::Dialog","Fields:",nullptr));
        checkbox -> setText(QCoreApplication::translate("BibTex::Dialog","Insert Empty, Optional Fields",nullptr));
    }

};


#endif
