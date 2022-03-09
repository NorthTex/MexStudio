#ifndef Header_PDF_SplitTool
#define Header_PDF_SplitTool


#include <QDialog>
#include <QFileInfo>


namespace Ui { class PDFSplitTool; }


class PDFSplitMergeTool : public QDialog {

	Q_OBJECT

	public:

		explicit PDFSplitMergeTool(QWidget * parent = 0,const QString & infile = QString());
		
		~PDFSplitMergeTool();

	private slots:

		void addPageRange(QLayout * parentLayout = 0);

		void moveDownPageRange();
		void outputFileDialog();
		void inputFileDialog();
		void removePageRange();
		void moveUpPageRange();
		void moveDownInput();
		void removeInput();
		void moveUpInput();
		void addInput();
		void go();

	private:

		void movePageRange(int delta);
		void moveInput(int delta);

		void resyncRows();

		Ui::PDFSplitTool * ui;

	signals:

		void runCommand(
			const QString & command,
			const QFileInfo & masterFile,
			const QFileInfo & currentFile,
			int linenr
		);

};


#endif
