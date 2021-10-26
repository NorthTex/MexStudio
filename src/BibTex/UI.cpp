
#include "BibTex/UI.hpp"


using BibTex::UI;


void UI::setup(QDialog * dialog){

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

    item_field = new Item();
    fields -> setHorizontalHeaderItem(0,item_field);

    item_value = new Item();
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


template <typename Object> inline void setText(const Object & object,const char * text){
    object -> setText(QCoreApplication::translate("BibTex::Dialog",text,nullptr));
}

void UI::translate(){

    setText(label_insert,"Insert in File:");
    setText(label_fields,"Fields:");
    setText(label_type,"Entry Type:");

    setText(item_field,"Field");
    setText(item_value,"Value");

    setText(checkbox,"Insert Empty, Optional Fields");
}
