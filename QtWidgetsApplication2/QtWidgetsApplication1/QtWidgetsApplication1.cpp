#include "QtWidgetsApplication1.h"
#include "LoopbackCaptureWrapper.h"

QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	connect(ui.btnStart, &QPushButton::clicked, this, [this]() {
		START_CAPTURE();
	});
	connect(ui.btnStop, &QPushButton::clicked, this, [this]() {
		STOP_CAPTURE();
	});
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{}

