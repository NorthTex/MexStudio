#ifndef Header_UniversalInput_Dialog
#define Header_UniversalInput_Dialog

#include "mostQtHeaders.h"
#include "configmanagerinterface.h"

/* This class works almost like QInputDialog, except that it can ask for
   multiple values */

using Property = const ManagedProperty &;
using Description = const QString &;

class UniversalInputDialog : public QDialog {

	Q_OBJECT

	protected:

		QGridLayout * gridLayout;
		QList<ManagedProperty> properties;

		void addWidget(QWidget * widget,Description description = "",Property prop = ManagedProperty());

		QDoubleSpinBox * addDoubleSpinBox(Property,Description);
		QCheckBox * addCheckBox(Property,Description);
		QComboBox * addComboBox(Property,Description);
		QLineEdit * addLineEdit(Property,Description);
		QTextEdit * addTextEdit(Property,Description);
		QSpinBox * addSpinBox(Property,Description);

	private slots:

		void myAccept();

	public:

		UniversalInputDialog(QWidget *parent = 0);

		QDoubleSpinBox * addVariable(float * variable,Description description = "");
		QComboBox * addVariable(int * variable,const QStringList & options,Description description = "");
		QCheckBox * addVariable(bool * variable,Description description = "");
		QLineEdit * addVariable(QString * variable,Description description = "");
		QComboBox * addVariable(QStringList * variable,Description description = "");
		QTextEdit * addTextEdit(QString * variable,Description description = "");
		QSpinBox * addVariable(int * variable,Description description = "");

		virtual void showEvent(QShowEvent * event);

};

//seems to be not necessary in qt4.5, but I'm not sure about older versions
Q_DECLARE_METATYPE(void *)

#endif
