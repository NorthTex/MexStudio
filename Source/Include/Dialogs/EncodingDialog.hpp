#ifndef Header_Encoding_Dialog
#define Header_Encoding_Dialog


#include "mostQtHeaders.h"
#include "ui_encodingdialog.h"
#include "qeditor.h"
#include "Latex/Document.hpp"


class EncodingDialog : public QDialog , private Ui::EncodingDialog {

	Q_OBJECT
	Q_DISABLE_COPY(EncodingDialog)

	public:
	
		explicit EncodingDialog(
			QWidget * parent = 0,
			QEditor * editor = 0);

	private:
	
		QEditor * edit;

	protected:

		virtual void changeEvent(QEvent * event);

	private slots:
	
		void on_change_clicked();
		void on_reload_clicked();
		void on_cancel_clicked();
		void on_view_clicked();
};


#endif
