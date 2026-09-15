#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QString>

#include "Nes.h"

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
	void openPatternTableDialog();
	void openRom();
	void closeRom();
	void runSimulationFrame();
	void updateWindowTitle();

	Nes nes;
	NesScene *scene = nullptr;
	QTimer simulationTimer;
	QString currentRomPath;
};
#endif // MAINWINDOW_H
