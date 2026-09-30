#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include <QFile>
#include <QTimer>
#include <QString>
#include <QTextStream>

#include "Nes.h"

class NesScene;
class NesView;

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
	void openPerformanceLog(const QString &romPath);
	void closePerformanceLog();
	void resetPerformanceWindow();
	void runSimulationFrame();
	void updateWindowTitle();
	void setDebugInputMode(bool enabled);

	Nes nes;
	NesScene *scene = nullptr;
	NesView *view = nullptr;
	QTimer simulationTimer;
	QElapsedTimer performanceTimer;
	QElapsedTimer frameTimer;
	QFile performanceLogFile;
	QTextStream performanceLogStream;
	quint64 performanceFrameCount = 0;
	quint64 performanceCoreNanoseconds = 0;
	quint64 performanceSceneNanoseconds = 0;
	quint64 performanceDirtyTiles = 0;
	quint64 performanceDecodedTiles = 0;
	quint64 performanceUpdatedTiles = 0;
	quint64 performanceNametableSetPos = 0;
	QString currentRomPath;
};
#endif // MAINWINDOW_H
