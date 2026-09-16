#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
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
	QElapsedTimer performanceTimer;
	quint64 performanceFrameCount = 0;
	quint64 performanceCoreNanoseconds = 0;
	quint64 performanceSceneNanoseconds = 0;
	quint64 performanceDirtyTiles = 0;
	quint64 performanceDecodedTiles = 0;
	quint64 performanceUpdatedTiles = 0;
	QString currentRomPath;
};
#endif // MAINWINDOW_H
