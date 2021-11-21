#include "tests/UserMacro.hpp"
#include "usermacro.h"


using QTest::addColumn;
using QTest::newRow;
using Test::UserMacro;


UserMacro::UserMacro(QObject * parent) : QObject(parent){
    fileName = "/tmp/testMacro.txsMacro";
}

void UserMacro::saveRead_data(){

    addColumn<QString>("name");
    addColumn<QString>("type");
    addColumn<QString>("tag");
    addColumn<QString>("abbrev");
    addColumn<QString>("trigger");
    addColumn<QString>("shortcut");
    addColumn<QString>("menu");
    addColumn<QString>("description");

    newRow("trivial")
        << "abcd"
        << "Snippet"
        << "abc"
        << "dfg"
        << "rth"
        << "dfsdf"
        << "sdfsdf"
        << "fgh";

    newRow("env")
        << "abcd"
        << "Environment"
        << "abc"
        << ""
        << ""
        << ""
        << ""
        << "";

    newRow("script")
        << "abcd"
        << "Script"
        << "abc\ncde\ndef"
        << ""
        << ""
        << ""
        << ""
        << "";
    newRow("quotes")
        << "abcd"
        << "Script"
        << "abc\nc\"de\"\ndef"
        << ""
        << ""
        << ""
        << ""
        << "";
    newRow("quotes plus backslash")
        << "abcd"
        << "Script"
        << "abc\nc\\\"de\\\"\ndef"
        << ""
        << ""
        << ""
        << ""
        << "";
    newRow("brackets")
        << "abcd"
        << "Script"
        << "abc\ncd]fsd\nd[ef"
        << ""
        << ""
        << ""
        << ""
        << "";

    newRow("name with backslash at end")
        << "abcd\\"
        << "Script"
        << "abc\ncd]fsd\nd[ef"
        << ""
        << ""
        << ""
        << ""
        << "";

    newRow("name/tag with backslash at end")
        << "abcd\\"
        << "Script"
        << "abc\ncd]fsd\nd[ef\\"
        << ""
        << ""
        << ""
        << ""
        << "";


}

void UserMacro::saveRead(){

    Q_ASSERT(!fileName.isEmpty());
    
    QFETCH(QString,name);
    QFETCH(QString,type);
    QFETCH(QString,tag);
    QFETCH(QString,abbrev);
    QFETCH(QString,trigger);
    QFETCH(QString,shortcut);
    QFETCH(QString,menu);
    QFETCH(QString,description);
    
    Macro::Type tp = Macro::Snippet;

    if(type == "Script")
        tp = Macro::Script;

    if(type=="Environment")
        tp = Macro::Environment;

    Macro macro(name,tp,tag,abbrev,trigger);
    macro.setShortcut(shortcut);
    macro.menu = menu;
    macro.description = description;
    macro.save(fileName);

    Macro macro2;
    macro2.load(fileName);

    QCOMPARE(macro2.name,name);
    QCOMPARE(macro2.type,tp);
    QCOMPARE(macro2.trigger,trigger);
    QCOMPARE(macro2.shortcut(),shortcut);
    QCOMPARE(macro2.menu,menu);
    QCOMPARE(macro2.description,description);
}
