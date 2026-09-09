#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class NesScene;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private:
	void savePalette();
	void openPaletteEditor();

	NesScene *scene = nullptr;
};
#endif // MAINWINDOW_H
