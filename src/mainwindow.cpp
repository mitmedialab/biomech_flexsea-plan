/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'plan-gui' Graphical User Interface
	Copyright (C) 2016 Dephy, Inc. <http://dephy.com/>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************
	[Lead developper] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] mainwindow.h: Main GUI Window - connects all the modules
	together
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-09 | jfduval | Initial GPL-3.0 release
	* 2016-09-12 | jfduval | create() RIC/NU view
****************************************************************************/

//****************************************************************************
// Include(s)
//****************************************************************************

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QFileDialog>
#include "main.h"

//****************************************************************************
// Constructor & Destructor:
//****************************************************************************

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	QMainWindow::showMaximized();

	setWindowTitle("FlexSEA Plan GUI v2.0 (Alpha Release - 12/2016)");
	ui->statusBar->showMessage("Program launched. COM: Not Connected. \
								Stream status: N/A", 0);
	setWindowIcon(QIcon(":icons/d_logo_small.png"));

	//Prepare FlexSEA Stack:
	init_flexsea_payload_ptr();

	W_Execute::setMaxWindow(EX_VIEW_WINDOWS_MAX);
	W_Manage::setMaxWindow(MN_VIEW_WINDOWS_MAX);
	W_Config::setMaxWindow(CONFIG_WINDOWS_MAX);
	W_SlaveComm::setMaxWindow(SLAVECOMM_WINDOWS_MAX);
	W_AnyCommand::setMaxWindow(ANYCOMMAND_WINDOWS_MAX);
	W_Converter::setMaxWindow(CONVERTER_WINDOWS_MAX);
	W_Calibration::setMaxWindow(CALIB_WINDOWS_MAX);
	W_Control::setMaxWindow(CONTROL_WINDOWS_MAX);
	W_2DPlot::setMaxWindow(PLOT2D_WINDOWS_MAX);
	W_Ricnu::setMaxWindow(RICNU_VIEW_WINDOWS_MAX);
	W_Battery::setMaxWindow(BATT_WINDOWS_MAX);
	W_LogKeyPad::setMaxWindow(LOGKEYPAD_WINDOWS_MAX);
	W_Gossip::setMaxWindow(GOSSIP_WINDOWS_MAX);
	W_Strain::setMaxWindow(STRAIN_WINDOWS_MAX);
	W_UserRW::setMaxWindow(USERRW_WINDOWS_MAX);

	W_Execute::setDescription("Execute");
	W_Manage::setDescription("Manage - Barebone");
	W_Config::setDescription("Configuration");
	W_SlaveComm::setDescription("Slave Communication");
	W_AnyCommand::setDescription("Any Command");
	W_Converter::setDescription("Converter");
	W_Calibration::setDescription("Hardware Calibration Tool");
	W_Control::setDescription("Control");
	W_2DPlot::setDescription("2D Plot");
	W_Ricnu::setDescription("RIC/NU Knee");
	W_Battery::setDescription("Battery - Barebone");
	W_LogKeyPad::setDescription("Read & Display Log File");
	W_Gossip::setDescription("Gossip - Barebone");
	W_Strain::setDescription("6ch StrainAmp - Barebone");
	W_UserRW::setDescription("User R/W");

	//SerialDriver:
	mySerialDriver = new SerialDriver;

	//Datalogger:
	myDataLogger = new DataLogger;

	//Create default objects:
	createConfig();
	createSlaveComm();

	//Disable options that are not implemented:
	ui->menuFile->actions().at(3)->setEnabled(false);		//Load configuration
	ui->menuFile->actions().at(4)->setEnabled(false);		//Save configuration
	ui->menuControl->actions().at(1)->setEnabled(false);	//In Control

	//Log and MainWindow
	connect(myDataLogger, SIGNAL(setStatusBarMessage(QString)), \
			this, SLOT(setStatusBar(QString)));

	//SerialDriver and MainWindow
	connect(mySerialDriver, SIGNAL(setStatusBarMessage(QString)), \
			this, SLOT(setStatusBar(QString)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

//****************************************************************************
// Public function(s):
//****************************************************************************

//****************************************************************************
// Public slot(s):
//****************************************************************************

//Transfer the signal from config to the
void MainWindow::translatorUpdateDataSourceStatus(DataSource status)
{
	if(status == FromLogFile)
	{
		emit connectorUpdateDisplayMode(DisplayLogData);
	}
	else
	{
		emit connectorUpdateDisplayMode(DisplayLiveData);
	}

}

void MainWindow::manageLogKeyPad(DataSource status)
{

	if(status == FromLogFile)
	{
		createLogKeyPad();
	}
	else
	{
		if(W_LogKeyPad::howManyInstance() > 0)
		{
				myViewLogKeyPad[0]->parentWidget()->close();
		}
	}
}

//Creates a new View Execute window
void MainWindow::createViewExecute(void)
{
	int objectCount = W_Execute::howManyInstance();

	//Limited number of windows:
	if(objectCount < EX_VIEW_WINDOWS_MAX)
	{
		DisplayMode status = DisplayLiveData;
		if(W_Config::howManyInstance() > 0)
		{
			if(myViewConfig[0]->getDataSourceStatus() == FromLogFile)
			{
				status = DisplayLogData;
			}
		}

		myViewExecute[objectCount] = \
				new W_Execute(this, myDataLogger->getLogFilePtr(), status);
		ui->mdiArea->addSubWindow(myViewExecute[objectCount]);
		myViewExecute[objectCount]->show();

		sendWindowCreatedMsg(W_Execute::getDescription(), objectCount,
							 W_Execute::getMaxWindow() - 1);

		//Link SerialDriver and Execute:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewExecute[objectCount], SLOT(refresh()));

		//Link to MainWindow for the close signal:
		connect(myViewExecute[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewExecute()));

		// Link to the slider of 2DPlot. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int)), \
				myViewExecute[objectCount], SLOT(displayLogData(int)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode)), \
				myViewExecute[objectCount], SLOT(updateDisplayMode(DisplayMode)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Execute::getDescription(),
								   W_Execute::getMaxWindow());
	}
}

void MainWindow::closeViewExecute(void)
{
	sendCloseWindowMsg(W_Execute::getDescription());
}

//Creates a new View Manage window
void MainWindow::createViewManage(void)
{
	int objectCount = W_Manage::howManyInstance();

	//Limited number of windows:
	if(objectCount < (MN_VIEW_WINDOWS_MAX))
	{
		myViewManage[objectCount] = new W_Manage(this);
		ui->mdiArea->addSubWindow(myViewManage[objectCount]);
		myViewManage[objectCount]->show();

		sendWindowCreatedMsg(W_Manage::getDescription(), objectCount,
							 W_Manage::getMaxWindow() - 1);

		//Link SerialDriver and Manage:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewManage[objectCount], SLOT(refreshDisplayManage()));

		//Link to MainWindow for the close signal:
		connect(myViewManage[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewManage()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Manage::getDescription(),
								   W_Manage::getMaxWindow());
	}
}

void MainWindow::closeViewManage(void)
{
	sendCloseWindowMsg(W_Manage::getDescription());
}

//Creates a new Config window
void MainWindow::createConfig(void)
{
	int objectCount = W_Config::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CONFIG_WINDOWS_MAX))
	{
		myViewConfig[objectCount] = new W_Config(this);
		ui->mdiArea->addSubWindow(myViewConfig[objectCount]);
		myViewConfig[objectCount]->show();

		sendWindowCreatedMsg(W_Config::getDescription(), objectCount,
							 W_Config::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewConfig[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeConfig()));

		//Link to DataLogger
		connect(myViewConfig[0], SIGNAL(openReadingFile(bool *)), \
				myDataLogger, SLOT(openReadingFile(bool *)));
		connect(myViewConfig[0], SIGNAL(closeReadingFile()), \
				myDataLogger, SLOT(closeReadingFile()));

		// Link to SerialDriver
		connect(myViewConfig[0], SIGNAL(openCom(QString,int,int)), \
				mySerialDriver, SLOT(open(QString,int,int)));
		connect(myViewConfig[0], SIGNAL(closeCom()), \
				mySerialDriver, SLOT(close()));
		connect(mySerialDriver, SIGNAL(openProgress(int,int)), \
				myViewConfig[0], SLOT(setComProgress(int,int)));
		connect(myViewConfig[0], SIGNAL(updateDataSourceStatus(DataSource)),
				this, SLOT(translatorUpdateDataSourceStatus(DataSource)));
		connect(myViewConfig[0], SIGNAL(updateDataSourceStatus(DataSource)),
				this, SLOT(manageLogKeyPad(DataSource)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Config::getDescription(),
								   W_Config::getMaxWindow());
	}
}

void MainWindow::closeConfig(void)
{
	sendCloseWindowMsg(W_Config::getDescription());

	if(W_LogKeyPad::howManyInstance() > 0)
	{
		myViewLogKeyPad[0]->parentWidget()->close();
	}
}

//Creates a new Control Control window
void MainWindow::createControlControl(void)
{
	int objectCount = W_Control::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CONTROL_WINDOWS_MAX))
	{
		myViewControl[objectCount] = new W_Control(this);
		ui->mdiArea->addSubWindow(myViewControl[objectCount]);
		myViewControl[objectCount]->show();

		sendWindowCreatedMsg(W_Control::getDescription(), objectCount,
							 W_Control::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewControl[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeControlControl()));

		//Link to SlaveComm to send commands:
		connect(myViewControl[objectCount], SIGNAL(writeCommand(uint8_t,uint8_t*,uint8_t)), \
				this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Control::getDescription(),
								   W_Control::getMaxWindow());
	}
}

void MainWindow::closeControlControl(void)
{
	sendCloseWindowMsg(W_Control::getDescription());
}

//Creates a new View 2DPlot window
void MainWindow::createView2DPlot(void)
{
	int objectCount = W_2DPlot::howManyInstance();

	//Limited number of windows:
	if(objectCount < (PLOT2D_WINDOWS_MAX))
	{
		myView2DPlot[objectCount] = new W_2DPlot(this);
		ui->mdiArea->addSubWindow(myView2DPlot[objectCount]);
		myView2DPlot[objectCount]->show();

		sendWindowCreatedMsg(W_2DPlot::getDescription(), objectCount,
							 W_2DPlot::getMaxWindow() - 1);


		//Fixed rate, not based on serial reply:
		connect(myViewSlaveComm[0], SIGNAL(refresh2DPlot()), \
				myView2DPlot[objectCount], SLOT(refresh2DPlot()));

		//For the trapeze/control tool:
		connect(myViewSlaveComm[0], SIGNAL(masterTimer100Hz()), \
				myView2DPlot[objectCount], SLOT(refreshControl()));

		//Link to MainWindow for the close signal:
		connect(myView2DPlot[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeView2DPlot()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_2DPlot::getDescription(),
								   W_2DPlot::getMaxWindow());
	}
}

void MainWindow::closeView2DPlot(void)
{
	sendCloseWindowMsg(W_2DPlot::getDescription());
}

//Creates a new Slave Comm window
void MainWindow::createSlaveComm(void)
{
	int objectCount = W_SlaveComm::howManyInstance();

	//Limited number of windows:
	if(objectCount < (SLAVECOMM_WINDOWS_MAX))
	{
		myViewSlaveComm[objectCount] = new W_SlaveComm(this);
		ui->mdiArea->addSubWindow(myViewSlaveComm[objectCount]);
		myViewSlaveComm[objectCount]->show();

		sendWindowCreatedMsg(W_SlaveComm::getDescription(), objectCount,
							 W_SlaveComm::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewSlaveComm[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeSlaveComm()));

		//Link SlaveComm and SerialDriver:
		connect(mySerialDriver, SIGNAL(openStatus(bool)), \
				myViewSlaveComm[0], SLOT(receiveComPortStatus(bool)));
		connect(myViewSlaveComm[0], SIGNAL(slaveReadWrite(uint, uint8_t*, uint8_t)), \
				mySerialDriver, SLOT(readWrite(uint, uint8_t*, uint8_t)));
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewSlaveComm[0], SLOT(receiveNewDataReady()));
		connect(mySerialDriver, SIGNAL(dataStatus(int, int)), \
				myViewSlaveComm[0], SLOT(displayDataReceived(int, int)));
		connect(mySerialDriver, SIGNAL(newDataTimeout(bool)), \
				myViewSlaveComm[0], SLOT(updateIndicatorTimeout(bool)));

		//Link SlaveComm and DataLogger
		connect(myViewSlaveComm[0], SIGNAL(openRecordingFile(uint8_t,QString)), \
				myDataLogger, SLOT(openRecordingFile(uint8_t,QString)));
		connect(myViewSlaveComm[0], SIGNAL(writeToLogFile(uint8_t,uint8_t
														  ,uint8_t,uint16_t)), \
				myDataLogger, SLOT(writeToFile(uint8_t,uint8_t,uint8_t,uint16_t)));
		connect(myViewSlaveComm[0], SIGNAL(closeRecordingFile(uint8_t)), \
				myDataLogger, SLOT(closeRecordingFile(uint8_t)));

		//Link SlaveComm and Control Through connector
		connect(this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)), \
				myViewSlaveComm[0], SLOT(externalSlaveReadWrite(uint8_t,uint8_t*,uint8_t)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_SlaveComm::getDescription(),
								   W_SlaveComm::getMaxWindow());
	}
}

void MainWindow::closeSlaveComm(void)
{
	sendCloseWindowMsg(W_SlaveComm::getDescription());
}

//Creates a new Any Command window
void MainWindow::createAnyCommand(void)
{
	int objectCount = W_AnyCommand::howManyInstance();

	//Limited number of windows:
	if(objectCount < (ANYCOMMAND_WINDOWS_MAX))
	{
		myViewAnyCommand[objectCount] = new W_AnyCommand(this);
		ui->mdiArea->addSubWindow(myViewAnyCommand[objectCount]);
		myViewAnyCommand[objectCount]->show();

		sendWindowCreatedMsg(W_AnyCommand::getDescription(), objectCount,
							 W_AnyCommand::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewAnyCommand[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeAnyCommand()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_AnyCommand::getDescription(),
								   W_AnyCommand::getMaxWindow());
	}
}

void MainWindow::closeAnyCommand(void)
{
	sendCloseWindowMsg(W_AnyCommand::getDescription());
}

//Creates a new View RIC/NU window
void MainWindow::createViewRicnu(void)
{
	int objectCount = W_Ricnu::howManyInstance();

	//Limited number of windows:
	if(objectCount < (RICNU_VIEW_WINDOWS_MAX))
	{
		myViewRicnu[objectCount] = new W_Ricnu(this);
		ui->mdiArea->addSubWindow(myViewRicnu[objectCount]);
		myViewRicnu[objectCount]->show();

		sendWindowCreatedMsg(W_Ricnu::getDescription(), objectCount,
							 W_Ricnu::getMaxWindow() - 1);

		//Link SerialDriver and RIC/NU:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewRicnu[objectCount], SLOT(refreshDisplayRicnu()));

		//Link to MainWindow for the close signal:
		connect(myViewRicnu[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewRicnu()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Ricnu::getDescription(),
								   W_Ricnu::getMaxWindow());
	}
}

void MainWindow::closeViewRicnu(void)
{
	sendCloseWindowMsg(W_Ricnu::getDescription());
}

//Creates a new Converter window
void MainWindow::createConverter(void)
{
	int objectCount = W_Converter::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CONVERTER_WINDOWS_MAX))
	{
		my_w_converter[objectCount] = new W_Converter(this);
		ui->mdiArea->addSubWindow(my_w_converter[objectCount]);
		my_w_converter[objectCount]->show();

		sendWindowCreatedMsg(W_Converter::getDescription(), objectCount,
							 W_Converter::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(my_w_converter[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeConverter()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Converter::getDescription(),
								   W_Converter::getMaxWindow());
	}
}

void MainWindow::closeConverter(void)
{
	sendCloseWindowMsg(W_Converter::getDescription());
}

//Creates a new Calibration window
void MainWindow::createCalib(void)
{
	int objectCount = W_Calibration::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CALIB_WINDOWS_MAX))
	{
		myViewCalibration[objectCount] = new W_Calibration(this);
		ui->mdiArea->addSubWindow(myViewCalibration[objectCount]);
		myViewCalibration[objectCount]->show();

		sendWindowCreatedMsg(W_Calibration::getDescription(), objectCount,
							 W_Calibration::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewCalibration[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeCalib()));
	}
	else
	{
		sendWindowCreatedFailedMsg(W_Calibration::getDescription(),
								   W_Calibration::getMaxWindow());
	}
}

void MainWindow::closeCalib(void)
{
	sendCloseWindowMsg(W_Calibration::getDescription());
}

//Creates a new User R/W window
void MainWindow::createUserRW(void)
{
	int objectCount = W_UserRW::howManyInstance();

	//Limited number of windows:
	if(objectCount < W_UserRW::getMaxWindow())
	{
		myUserRW[objectCount] = new W_UserRW(this);
		ui->mdiArea->addSubWindow(myUserRW[objectCount]);
		myUserRW[objectCount]->show();

		sendWindowCreatedMsg(W_UserRW::getDescription(), objectCount,
							 W_UserRW::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myUserRW[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeUserRW()));

		//Link to SlaveComm to send commands:
		connect(myUserRW[objectCount], SIGNAL(writeCommand(uint8_t,\
				uint8_t*,uint8_t)), this, SIGNAL(connectorWriteCommand(uint8_t,\
				uint8_t*, uint8_t)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_UserRW::getDescription(),
								   W_UserRW::getMaxWindow());
	}
}

void MainWindow::closeUserRW(void)
{
	sendCloseWindowMsg(W_UserRW::getDescription());
}

//Creates a new View Gossip window
void MainWindow::createViewGossip(void)
{
	int objectCount = W_Gossip::howManyInstance();

	//Limited number of windows:
	if(objectCount < (GOSSIP_WINDOWS_MAX))
	{
		myViewGossip[objectCount] = new W_Gossip(this);
		ui->mdiArea->addSubWindow(myViewGossip[objectCount]);
		myViewGossip[objectCount]->show();

		sendWindowCreatedMsg(W_Gossip::getDescription(), objectCount,
							 W_Gossip::getMaxWindow() - 1);

		//Link SerialDriver and Gossip:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewGossip[objectCount], SLOT(refreshDisplayGossip()));

		//Link to MainWindow for the close signal:
		connect(myViewGossip[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewGossip()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Gossip::getDescription(),
								   W_Gossip::getMaxWindow());
	}
}

void MainWindow::closeViewGossip(void)
{
	sendCloseWindowMsg(W_Gossip::getDescription());
}

//Creates a new View Strain window
void MainWindow::createViewStrain(void)
{
	int objectCount = W_Strain::howManyInstance();

	//Limited number of windows:
	if(objectCount < (STRAIN_WINDOWS_MAX))
	{
		myViewStrain[objectCount] = new W_Strain(this);
		ui->mdiArea->addSubWindow(myViewStrain[objectCount]);
		myViewStrain[objectCount]->show();

		sendWindowCreatedMsg(W_Strain::getDescription(), objectCount,
							 W_Strain::getMaxWindow() - 1);

		//Link SerialDriver and Strain:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewStrain[objectCount], SLOT(refreshDisplayStrain()));

		//Link to MainWindow for the close signal:
		connect(myViewStrain[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewStrain()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Strain::getDescription(),
								   W_Strain::getMaxWindow());
	}
}

void MainWindow::closeViewStrain(void)
{
	sendCloseWindowMsg(W_Strain::getDescription());
}

//Creates a new View Battery window
void MainWindow::createViewBattery(void)
{
	int objectCount = W_Battery::howManyInstance();

	//Limited number of windows:
	if(objectCount < (BATT_WINDOWS_MAX))
	{
		myViewBatt[objectCount] = new W_Battery(this);
		ui->mdiArea->addSubWindow(myViewBatt[objectCount]);
		myViewBatt[objectCount]->show();

		sendWindowCreatedMsg(W_Battery::getDescription(), objectCount,
							 W_Battery::getMaxWindow() - 1);

		//Link SerialDriver and Battery:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewBatt[objectCount], SLOT(refreshDisplayBattery()));

		//Link to MainWindow for the close signal:
		connect(myViewBatt[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewBattery()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Battery::getDescription(),
								   W_Battery::getMaxWindow());
	}
}

void MainWindow::closeViewBattery(void)
{
	sendCloseWindowMsg(W_Battery::getDescription());
}

//Creates a new LogKeyPad
void MainWindow::createLogKeyPad(void)
{
	int objectCount = W_LogKeyPad::howManyInstance();

	//Limited number of windows:
	if(objectCount < (LOGKEYPAD_WINDOWS_MAX))
	{
		myViewLogKeyPad[objectCount] = new W_LogKeyPad(this, myDataLogger->getLogFilePtr());
		ui->mdiArea->addSubWindow(myViewLogKeyPad[objectCount]);
		myViewLogKeyPad[objectCount]->show();
		myViewLogKeyPad[objectCount]->parentWidget()->setWindowFlags(
					Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

		sendWindowCreatedMsg(W_LogKeyPad::getDescription(), objectCount,
							 W_LogKeyPad::getMaxWindow() - 1);

		// Link for the data slider
		connect(myViewLogKeyPad[objectCount], SIGNAL(logTimeSliderValueChanged(int)), \
				this, SIGNAL(connectorRefreshLogTimeSlider(int)));

		//Link to MainWindow for the close signal:
		connect(myViewLogKeyPad[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeLogKeyPad()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_LogKeyPad::getDescription(),
								   W_LogKeyPad::getMaxWindow());
	}
}

void MainWindow::closeLogKeyPad(void)
{
	sendCloseWindowMsg(W_LogKeyPad::getDescription());
}

void MainWindow::sendWindowCreatedMsg(QString windowName, int index, int maxIndex)
{
	QString msg = "Created '" + windowName + "' window index " +
			QString::number(index) + " (max index = " +
			QString::number(maxIndex)+ ")";
	qDebug() << msg;
	ui->statusBar->showMessage(msg);
}

void MainWindow::sendWindowCreatedFailedMsg(QString windowName, int maxWindow)
{
	QString msg = "Maximum number of " + windowName + " window reached (" \
			+ QString::number(maxWindow) + ")";
	qDebug() << msg;
	ui->statusBar->showMessage(msg);
}

void MainWindow::sendCloseWindowMsg(QString windowName)
{
	QString msg = "View '" + windowName + "' window closed.";
	qDebug() << msg;
	ui->statusBar->showMessage(msg);
}

void MainWindow::displayAbout()
{
	QMessageBox::about(this, tr("About FlexSEA"), \
	tr("<center><u>FlexSEA: <b>Flex</b>ible, <b>S</b>calable <b>E</b>lectronics\
	 <b>A</b>rchitecture.</u><br><br>Project originaly developped at the \
	<a href='http://biomech.media.mit.edu/'>MIT Media Lab Biomechatronics \
	group</a>, now supported by <a href='http://dephy.com/'>Dephy, Inc.</a>\
	<br><br><b>Copyright &copy; Dephy, Inc. 2016</b><br><br>Software released \
	under the GNU GPL-3.0 license</center>"));
}

void MainWindow::displayLicense()
{
	QMessageBox::information(this, tr("Software License Information"), \
	tr("<center><b>Copyright &copy; Dephy, Inc. 2016</b>\
		<br><br>This program is free software: you can redistribute it and/or modify \
		it under the terms of the GNU General Public License as published by \
		the Free Software Foundation, either version 3 of the License, or \
		(at your option) any later version. <br><br>\
		This program is distributed in the hope that it will be useful,\
		but WITHOUT ANY WARRANTY; without even the implied warranty of \
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \
		GNU General Public License for more details. <br><br>\
		You should have received a copy of the GNU General Public License \
		along with this program.  If not, see \
		<a href='http://www.gnu.org/licenses/'>\
		http://www.gnu.org/licenses/</a>.</center>"));
}

void MainWindow::displayDocumentation()
{
	QMessageBox::information(this, tr("Documentation"), \
	tr("<center>Documentation available online: \
		<a href='http://flexsea.media.mit.edu/'>\
		FlexSEA Documentation</a></center>"));
}

void MainWindow::setStatusBar(QString msg)
{
	ui->statusBar->showMessage(msg);
}
