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
#include <QTextStream>
#include <flexsea_system.h>
#include <passthroughprovider.h>
#include <sineprovider3.h>

//****************************************************************************
// Constructor & Destructor:
//****************************************************************************

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	QMainWindow::showMaximized();

	initMdiState();

	activeSlaveNameStreaming = "None";

	//Header contains timestamp:
	QString winHeader = "FlexSEA-Plan GUI v2.1 (Beta) PRIVATE BUILD [Last full build: ";
	winHeader = winHeader + QString(__DATE__) + QString(' ') + QString(__TIME__) \
				+ QString(']');
	setWindowTitle(winHeader);

	//Save application's path:
	appPath = QDir::currentPath();

	ui->statusBar->showMessage("Program launched. COM: Not Connected. \
								Stream status: N/A", 0);
	setWindowIcon(QIcon(":icons/d_logo_small_outlined.png"));

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
	W_TestBench::setMaxWindow(TESTBENCH_WINDOWS_MAX);
	W_CommTest::setMaxWindow(COMMTEST_WINDOWS_MAX);
	W_InControl::setMaxWindow(INCONTROL_WINDOWS_MAX);
	W_Event::setMaxWindow(EVENT_WINDOWS_MAX);
	W_Rigid::setMaxWindow(RIGID_WINDOWS_MAX);
	W_CycleTester::setMaxWindow(CYCLE_TESTER_WINDOWS_MAX);
	W_UserTesting::setMaxWindow(USER_TESTING_WINDOWS_MAX);

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
	W_Battery::setDescription("Battery Board");
	W_LogKeyPad::setDescription("Read & Display Log File");
	W_Gossip::setDescription("Gossip - Barebone");
	W_Strain::setDescription("6ch StrainAmp - Barebone");
	W_UserRW::setDescription("User R/W");
	W_TestBench::setDescription("Test Bench");
	W_CommTest::setDescription("Communication Test");
	W_InControl::setDescription("Controller Tuning");
	W_Event::setDescription("Event Flag");
	W_Rigid::setDescription("FlexSEA-Rigid");
	W_CycleTester::setDescription("FlexSEA-Rigid Cycle Tester");
	W_AnkleTorque::setDescription("Ankle Torque Tool");
	W_UserTesting::setDescription("User Testing");

	initFlexSeaDeviceObject();
	//SerialDriver:
	mySerialDriver = new SerialDriver();
	streamManager = new StreamManager(nullptr, mySerialDriver);
	//Datalogger:
	myDataLogger = new DataLogger(this,
								  &executeLog,
								  &manageLog,
								  &gossipLog,
								  &batteryLog,
								  &strainLog,
								  &ricnuLog,
								  &ankle2DofLog,
								  &testBenchLog,
								  &rigidLog);

	initSerialComm(mySerialDriver, streamManager);
	userDataManager = new DynamicUserDataManager(this);
	connect(mySerialDriver, &SerialDriver::newDataReady, userDataManager, &DynamicUserDataManager::handleNewMessage);

	//Create default objects:
	createConfig();
	createSlaveComm();

	//Populate the QAction menu lists
	initMenus();

	//Log and MainWindow
	connect(myDataLogger, SIGNAL(setStatusBarMessage(QString)), \
			this, SLOT(setStatusBar(QString)));

	comPortStatus = false;

	initializeDataProviders();
	chartController = new ChartController(this);
	for(int i = 0; i < dataProviders.size(); i++)
	{
		chartController->addDataProvider(dataProviders.at(i));
	}

	initializeCreateWindowFctPtr();
	loadCSVconfigFile();	//By default we load the last saved settings
}

MainWindow::~MainWindow()
{
	delete ui;

	delete mySerialDriver;
	delete streamManager;

	int num2d = W_2DPlot::howManyInstance();
	for(int i = 0; i < num2d; i++)
	{
		delete myView2DPlot[i];
	}
}

//Populates the QAction menu lists
void MainWindow::initMenus(void)
{
	//File:
	ui->menuFile->addAction("Configuration", this, &MainWindow::createConfig);
	ui->menuFile->addAction("Slave Communication", this, &MainWindow::createSlaveComm);
	ui->menuFile->addSeparator();
	ui->menuFile->addAction("Load Settings", this, &MainWindow::loadConfig);
	ui->menuFile->addAction("Save Settings", this, &MainWindow::saveConfig);
	ui->menuFile->addAction("Default Settings", this, &MainWindow::defaultConfig);

	//Add View:
	ui->menuView->addAction("Battery", this, &MainWindow::createViewBattery);
	ui->menuView->addAction("Execute", this, &MainWindow::createViewExecute);
	ui->menuView->addAction("Gossip", this, &MainWindow::createViewGossip);
	ui->menuView->addAction("Manage", this, &MainWindow::createViewManage);
	ui->menuView->addAction("Rigid", this, &MainWindow::createViewRigid);
	ui->menuView->addAction("Strain", this, &MainWindow::createViewStrain);
	ui->menuView->addSeparator();
	ui->menuView->addAction("2D Plot", this, &MainWindow::createView2DPlot);

	//Add Control:
	ui->menuControl->addAction("Control Loop", this, &MainWindow::createControlControl);
	ui->menuControl->addAction("In Control Tool", this, &MainWindow::createInControl);
	ui->menuControl->addAction("Any Command", this, &MainWindow::createAnyCommand);

	//User:
	ui->menuUser->addAction("User Testing", this, &MainWindow::createUserTesting);
	ui->menuUser->addAction("Event Flags", this, &MainWindow::createToolEvent);
	ui->menuUser->addAction("User R/W", this, &MainWindow::createUserRW);
	ui->menuUser->addSeparator();
	ui->menuUser->addAction("RIC/NU Knee", this, &MainWindow::createViewRicnu);

	//Tools:
	ui->menuTools->addAction("Calibration", this, &MainWindow::createCalib);
	ui->menuTools->addAction("Communication Test", this, &MainWindow::createViewCommTest);
	ui->menuTools->addAction("Converter", this, &MainWindow::createConverter);
	ui->menuTools->addAction("Test Bench", this, &MainWindow::createViewTestBench);
	ui->menuTools->addAction("Cycle Tester", this, &MainWindow::createCycleTester);

	//Gait Lab:
	ui->menuGL->addAction("Ankle Torque Tool", this, &MainWindow::createAnkleTorqueTool);
	ui->menuGL->addAction("Chart Window", this, &MainWindow::triggerChartView);

	//Help:
	ui->menuHelp->addAction("Documentation", this, &MainWindow::displayDocumentation);
	ui->menuHelp->addAction("License", this, &MainWindow::displayLicense);
	ui->menuHelp->addAction("About", this, &MainWindow::displayAbout);
}

void MainWindow::initializeDataProviders()
{
	dataProviders.append(new SineProvider3(90));
	dataProviders.append(new SineProvider3());
	dataProviders.append(new SineProvider3(180));

	PassThroughProvider* ptp = new PassThroughProvider(33);
	ptp->setDataPointer(&(manag1.accel.x));
	ptp->dataSignSize = PassThroughProvider::S_16;
	ptp->setLabel("Manage X Accel");

	dataProviders.append(ptp);
}

void MainWindow::initFlexSeaDeviceObject(void)
{
	executeDevList.append(ExecuteDevice(&exec1));
	executeDevList.last().slaveName = "Execute 1";
	executeDevList.last().slaveID = FLEXSEA_EXECUTE_1;
	flexseaPtrlist.append(&executeDevList.last());
	executeFlexList.append(&executeDevList.last());

	executeDevList.append(ExecuteDevice(&exec2));
	executeDevList.last().slaveName = "Execute 2";
	executeDevList.last().slaveID = FLEXSEA_EXECUTE_2;
	flexseaPtrlist.append(&executeDevList.last());
	executeFlexList.append(&executeDevList.last());

	executeDevList.append(ExecuteDevice(&exec3));
	executeDevList.last().slaveName = "Execute 3";
	executeDevList.last().slaveID = FLEXSEA_EXECUTE_3;
	flexseaPtrlist.append(&executeDevList.last());
	executeFlexList.append(&executeDevList.last());

	executeDevList.append(ExecuteDevice(&exec4));
	executeDevList.last().slaveName = "Execute 4";
	executeDevList.last().slaveID = FLEXSEA_EXECUTE_4;
	flexseaPtrlist.append(&executeDevList.last());
	executeFlexList.append(&executeDevList.last());

	manageDevList.append(ManageDevice(&manag1));
	manageDevList.last().slaveName = "Manage 1";
	manageDevList.last().slaveID = FLEXSEA_MANAGE_1;
	flexseaPtrlist.append(&manageDevList.last());
	manageFlexList.append(&manageDevList.last());

	manageDevList.append(ManageDevice(&manag2));
	manageDevList.last().slaveName = "Manage 2";
	manageDevList.last().slaveID = FLEXSEA_MANAGE_2;
	flexseaPtrlist.append(&manageDevList.last());
	manageFlexList.append(&manageDevList.last());

	gossipDevList.append(GossipDevice(&gossip1));
	gossipDevList.last().slaveName = "Gossip 1";
	gossipDevList.last().slaveID = FLEXSEA_GOSSIP_1;
	flexseaPtrlist.append(&gossipDevList.last());
	gossipFlexList.append(&gossipDevList.last());

	gossipDevList.append(GossipDevice(&gossip2));
	gossipDevList.last().slaveName = "Gossip 2";
	gossipDevList.last().slaveID = FLEXSEA_GOSSIP_2;
	flexseaPtrlist.append(&gossipDevList.last());
	gossipFlexList.append(&gossipDevList.last());

	batteryDevList.append(BatteryDevice(&batt1));
	batteryDevList.last().slaveName = "Battery";
	batteryDevList.last().slaveID = FLEXSEA_VIRTUAL_PROJECT;
	flexseaPtrlist.append(&batteryDevList.last());
	batteryFlexList.append(&batteryDevList.last());

	strainDevList.append(StrainDevice(&strain1));
	strainDevList.last().slaveName = "Strain 1";
	strainDevList.last().slaveID = FLEXSEA_STRAIN_1;
	flexseaPtrlist.append(&strainDevList.last());
	strainFlexList.append(&strainDevList.last());

	ricnuDevList.append(RicnuProject(&exec1, &strain1, &batt1));
	ricnuDevList.last().slaveName = "RIC/NU";
	ricnuDevList.last().slaveID = FLEXSEA_VIRTUAL_PROJECT;
	flexseaPtrlist.append(&ricnuDevList.last());
	ricnuFlexList.append(&ricnuDevList.last());

	ankle2DofDevList.append(Ankle2DofProject(&exec1, &exec2));
	ankle2DofDevList.last().slaveName = "Ankle 2 DoF";
	ankle2DofDevList.last().slaveID = FLEXSEA_VIRTUAL_PROJECT;
	flexseaPtrlist.append(&ankle2DofDevList.last());
	ankle2DofFlexList.append(&ankle2DofDevList.last());

	testBenchDevList.append(TestBenchProject(&exec1, &exec2, &motortb, &batt1));
	testBenchDevList.last().slaveName = "Test Bench";
	testBenchDevList.last().slaveID = FLEXSEA_VIRTUAL_PROJECT;
	// TODO: Does it make sense?
	// Answer: It does not make sense to use the same FlexseaDevice type for both:
	//				a) slaves
	//				b) projects / experiments
	flexseaPtrlist.append(&testBenchDevList.last());
	testBenchFlexList.append(&testBenchDevList.last());

	dynamicDeviceList.append(userDataManager->getDevice());
	flexseaPtrlist.append(userDataManager->getDevice());

	rigidDevList.append(RigidDevice(&rigid1));
	rigidDevList.last().slaveName = "Rigid 1";
	rigidDevList.last().slaveID = FLEXSEA_VIRTUAL_PROJECT;
	flexseaPtrlist.append(&rigidDevList.last());
	rigidFlexList.append(&rigidDevList.last());

	return;
}

void MainWindow::initSerialComm(SerialDriver *driver, StreamManager *manager)
{
//	serialThread = new QThread(this);
//	driver->moveToThread(serialThread);
//	manager->moveToThread(serialThread);
//	serialThread->start(QThread::HighestPriority);

	connect(driver, &SerialDriver::aboutToClose, manager, &StreamManager::onComPortClosing, Qt::DirectConnection);

	//Link StreamManager/SerialDriver and DataLogger
	connect(manager, SIGNAL(openRecordingFile(FlexseaDevice *)), \
			myDataLogger, SLOT(openRecordingFile(FlexseaDevice *)));
	connect(driver, SIGNAL(writeToLogFile(FlexseaDevice *)), \
			myDataLogger, SLOT(writeToFile(FlexseaDevice *)));
	connect(manager, SIGNAL(closeRecordingFile(FlexseaDevice*)), \
			myDataLogger, SLOT(closeRecordingFile(FlexseaDevice*)));
	connect(this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)), \
		manager, SLOT(enqueueCommand(uint8_t,uint8_t*)));

	//SerialDriver and MainWindow
	connect(driver, SIGNAL(setStatusBarMessage(QString)), \
			this, SLOT(setStatusBar(QString)));
	connect(driver, SIGNAL(openStatus(bool)), \
			this, SLOT(saveComPortStatus(bool)));
}

void MainWindow::initializeCreateWindowFctPtr(void)
{
	//By default, point to empty function:
	for(int i = 0; i < WINDOWS_TYPES; i++)
	{
		mdiCreateWinPtr[i] = &MainWindow::emptyWinFct;
	}

	mdiCreateWinPtr[CONFIG_WINDOWS_ID] = &MainWindow::createConfig;
	//mdiCreateWinPtr[LOGKEYPAD_WINDOWS_ID] = &MainWindow::createLogKeyPad();
	mdiCreateWinPtr[SLAVECOMM_WINDOWS_ID] = &MainWindow::createSlaveComm;
	mdiCreateWinPtr[PLOT2D_WINDOWS_ID] = &MainWindow::createView2DPlot;
	mdiCreateWinPtr[CONTROL_WINDOWS_ID] = &MainWindow::createControlControl;
	mdiCreateWinPtr[INCONTROL_WINDOWS_ID] = &MainWindow::createInControl;
	mdiCreateWinPtr[USERRW_WINDOWS_ID] = &MainWindow::createUserRW;
	mdiCreateWinPtr[EVENT_WINDOWS_ID] = &MainWindow::createToolEvent;
	mdiCreateWinPtr[ANYCOMMAND_WINDOWS_ID] = &MainWindow::createAnyCommand;
	mdiCreateWinPtr[CONVERTER_WINDOWS_ID] = &MainWindow::createConverter;
	mdiCreateWinPtr[CALIB_WINDOWS_ID] = &MainWindow::createCalib;
	mdiCreateWinPtr[COMMTEST_WINDOWS_ID] = &MainWindow::createViewCommTest;
	mdiCreateWinPtr[EX_VIEW_WINDOWS_ID] = &MainWindow::createViewExecute;
	mdiCreateWinPtr[MN_VIEW_WINDOWS_ID] = &MainWindow::createViewManage;
	mdiCreateWinPtr[BATT_WINDOWS_ID] = &MainWindow::createViewBattery;
	mdiCreateWinPtr[GOSSIP_WINDOWS_ID] = &MainWindow::createViewGossip;
	mdiCreateWinPtr[STRAIN_WINDOWS_ID] = &MainWindow::createViewStrain;
	mdiCreateWinPtr[RICNU_VIEW_WINDOWS_ID] = &MainWindow::createViewRicnu;
	mdiCreateWinPtr[TESTBENCH_WINDOWS_ID] = &MainWindow::createViewTestBench;
	mdiCreateWinPtr[ANKLE_TORQUE_WINDOWS_ID] = &MainWindow::createAnkleTorqueTool;
	mdiCreateWinPtr[RIGID_WINDOWS_ID] = &MainWindow::createViewRigid;
	mdiCreateWinPtr[CYCLE_TESTER_WINDOWS_ID] = &MainWindow::createCycleTester;
	mdiCreateWinPtr[USER_TESTING_WINDOWS_ID] = &MainWindow::createUserTesting;
}

/*
void MainWindow::initializeCloseWindowFctPtr(void)
{
	//By default, point to empty function:
	for(int i = 0; i < WINDOWS_TYPES; i++)
	{
		mdiCloseWinPtr[i] = &MainWindow::emptyWinFct;
	}

	mdiCloseWinPtr[CONFIG_WINDOWS_ID] = &MainWindow::closeConfig;
	//mdiCloseWinPtr[LOGKEYPAD_WINDOWS_ID] = &MainWindow::closeLogKeyPad();
	mdiCloseWinPtr[SLAVECOMM_WINDOWS_ID] = &MainWindow::closeSlaveComm;
	mdiCloseWinPtr[PLOT2D_WINDOWS_ID] = &MainWindow::closeView2DPlot;
	mdiCloseWinPtr[CONTROL_WINDOWS_ID] = &MainWindow::closeControlControl;
	mdiCloseWinPtr[INCONTROL_WINDOWS_ID] = &MainWindow::closeInControl;
	mdiCloseWinPtr[USERRW_WINDOWS_ID] = &MainWindow::closeUserRW;
	mdiCloseWinPtr[EVENT_WINDOWS_ID] = &MainWindow::closeToolEvent;
	mdiCloseWinPtr[ANYCOMMAND_WINDOWS_ID] = &MainWindow::closeAnyCommand;
	mdiCloseWinPtr[CONVERTER_WINDOWS_ID] = &MainWindow::closeConverter;
	mdiCloseWinPtr[CALIB_WINDOWS_ID] = &MainWindow::closeCalib;
	mdiCloseWinPtr[COMMTEST_WINDOWS_ID] = &MainWindow::closeViewCommTest;
	mdiCloseWinPtr[EX_VIEW_WINDOWS_ID] = &MainWindow::closeViewExecute;
	mdiCloseWinPtr[MN_VIEW_WINDOWS_ID] = &MainWindow::closeViewManage;
	mdiCloseWinPtr[BATT_WINDOWS_ID] = &MainWindow::closeViewBattery;
	mdiCloseWinPtr[GOSSIP_WINDOWS_ID] = &MainWindow::closeViewGossip;
	mdiCloseWinPtr[STRAIN_WINDOWS_ID] = &MainWindow::closeViewStrain;
	mdiCloseWinPtr[RICNU_VIEW_WINDOWS_ID] = &MainWindow::closeViewRicnu;
	mdiCloseWinPtr[TESTBENCH_WINDOWS_ID] = &MainWindow::closeViewTestBench;
	mdiCloseWinPtr[ANKLE_TORQUE_WINDOWS_ID] = &MainWindow::closeAnkleTorqueTool;
	mdiCloseWinPtr[RIGID_WINDOWS_ID] = &MainWindow::closeViewRigid;
	mdiCloseWinPtr[CYCLE_TESTER_WINDOWS_ID] = &MainWindow::closeCycleTester;
	mdiCloseWinPtr[USER_TESTING_WINDOWS_ID] = &MainWindow::closeUserTesting;
}
*/

void MainWindow::emptyWinFct(void)
{
	//Catch all for the function pointer
}

//****************************************************************************
// Public function(s):
//****************************************************************************

//****************************************************************************
// Public slot(s):
//****************************************************************************

void MainWindow::translatorActiveSlaveStreaming(QString slaveName)
{
	activeSlaveNameStreaming = slaveName;
	emit connectorCurrentSlaveStreaming(slaveName);
}

void MainWindow::saveComPortStatus(bool status)
{
	comPortStatus = status;
}

void MainWindow::saveConfig(void)
{
	qDebug() << "Saving settings...";
	writeSettings();

	//To CSV:
	saveCSVconfigFile();
}

void MainWindow::loadConfig(void)
{
	qDebug() << "Loading settings...";
	readSettings();

	//From CSV:
	loadCSVconfigFile();
}

void MainWindow::defaultConfig(void)
{
	qDebug() << "Default settings...";

	//Close all Windows other than the essentials:
	for(int i = PLOT2D_WINDOWS_ID; i < WINDOWS_TYPES; i++)
	{
		for(int j = 0; j < WINDOWS_MAX_INSTANCES; j++)
		{
			if(mdiState[i][j].open == true)
			{
				qDebug() << "Closing" << i << j;
				mdiState[i][j].winPtr->close();
			}
		}
	}
}

//Transfer the signal from config to the
void MainWindow::translatorUpdateDataSourceStatus(DataSource status, FlexseaDevice* devPtr)
{
	currentFlexLog = devPtr;

	if(status == FromLogFile)
	{
		emit connectorUpdateDisplayMode(DisplayLogData, devPtr);
	}
	else
	{
		emit connectorUpdateDisplayMode(DisplayLiveData, devPtr);

		if(W_LogKeyPad::howManyInstance() > 0)
		{
				myViewLogKeyPad[0]->parentWidget()->close();
		}
	}
}

void MainWindow::manageLogKeyPad(DataSource status, FlexseaDevice *devPtr)
{
	if(status == FromLogFile)
	{
		createLogKeyPad(devPtr);
	}
}

void MainWindow::createAnkleTorqueTool(void)
{
	int count = W_AnkleTorque::howManyInstance();
	if(count >= ANKLE_TORQUE_WINDOWS_MAX)
	{
		sendWindowCreatedFailedMsg(W_AnkleTorque::getDescription(),
								   W_AnkleTorque::getMaxWindow());
		return;
	}

	W_AnkleTorque* w = new W_AnkleTorque(this, streamManager);
	myAnkleTorque[count] = w;

	int slaveCommCount = W_SlaveComm::howManyInstance();
	if(slaveCommCount)
	{
		connect(w,									&W_AnkleTorque::getCurrentDevice,
				myViewSlaveComm[slaveCommCount-1],	&W_SlaveComm::getCurrentDevice);
		connect(w,									&W_AnkleTorque::getSlaveId,
				myViewSlaveComm[slaveCommCount-1],	&W_SlaveComm::getSlaveId);
	}
	else
	{
		qDebug() << "Unable to connect ankletorque to slave comm, ankletorque won't work";
		delete w; //this gonna be bad
		myAnkleTorque[count] = nullptr;
		return;
	}

	connect(mySerialDriver, &SerialDriver::newDataReady, w, &W_AnkleTorque::receiveNewData);
	connect(mySerialDriver, &SerialDriver::openStatus, w, &W_AnkleTorque::comStatusChanged);
	connect(w, &W_AnkleTorque::writeCommand, this, &MainWindow::connectorWriteCommand);

	//Link to MainWindow for the close signal:
	connect(myAnkleTorque[count], SIGNAL(windowClosed()), \
			this, SLOT(closeAnkleTorqueTool()));

	mdiState[ANKLE_TORQUE_WINDOWS_ID][count].winPtr = ui->mdiArea->addSubWindow(w);
	mdiState[ANKLE_TORQUE_WINDOWS_ID][count].open = true;
	w->show();
}

void MainWindow::closeAnkleTorqueTool(void)
{
	sendCloseWindowMsg(W_AnkleTorque::getDescription());
	mdiState[ANKLE_TORQUE_WINDOWS_ID][0].open = false;	//ToDo this is wrong!
}

//Creates a new View Execute window
void MainWindow::createViewExecute(void)
{
	int objectCount = W_Execute::howManyInstance();

	//Limited number of windows:
	if(objectCount < EX_VIEW_WINDOWS_MAX)
	{
		myViewExecute[objectCount] = \
				new W_Execute(this,
							  currentFlexLog,
							  &executeLog,
							  &ankle2DofLog,
							  &testBenchLog,
							  getDisplayMode(),
							  &executeDevList);

		mdiState[EX_VIEW_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewExecute[objectCount]);
		mdiState[EX_VIEW_WINDOWS_ID][objectCount].open = true;
		myViewExecute[objectCount]->show();

		sendWindowCreatedMsg(W_Execute::getDescription(), objectCount,
							 W_Execute::getMaxWindow() - 1);

		//Link SerialDriver and Execute:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewExecute[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewExecute[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewExecute()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewExecute[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewExecute[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Execute::getDescription(),
								   W_Execute::getMaxWindow());
	}
}

void MainWindow::triggerChartView()
{
	chartController->createChartView(ui->mdiArea);
}

void MainWindow::closeViewExecute(void)
{
	sendCloseWindowMsg(W_Execute::getDescription());
	mdiState[EX_VIEW_WINDOWS_ID][W_Execute::howManyInstance()-1].open = false;	//ToDo this is wrong!
}

//Creates a new View Manage window
void MainWindow::createViewManage(void)
{
	int objectCount = W_Manage::howManyInstance();

	//Limited number of windows:
	if(objectCount < (MN_VIEW_WINDOWS_MAX))
	{
		myViewManage[objectCount] = new W_Manage(this, &manageLog,
												 getDisplayMode(), &manageDevList);
		mdiState[MN_VIEW_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewManage[objectCount]);
		mdiState[MN_VIEW_WINDOWS_ID][objectCount].open = true;
		myViewManage[objectCount]->show();

		sendWindowCreatedMsg(W_Manage::getDescription(), objectCount,
							 W_Manage::getMaxWindow() - 1);

		//Link SerialDriver and Manage:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewManage[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewManage[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewManage()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewManage[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewManage[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[MN_VIEW_WINDOWS_ID][W_Manage::howManyInstance()].open = false;
}

//Creates a new Config window
void MainWindow::createConfig(void)
{
	int objectCount = W_Config::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CONFIG_WINDOWS_MAX))
	{
		myViewConfig[objectCount] = new W_Config(this);
		mdiState[CONFIG_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewConfig[objectCount]);
		mdiState[CONFIG_WINDOWS_ID][objectCount].open = true;
		myViewConfig[objectCount]->show();

		myViewConfig[objectCount]->serialDriver = mySerialDriver;

		sendWindowCreatedMsg(W_Config::getDescription(), objectCount,
							 W_Config::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewConfig[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeConfig()));

		//Link to DataLogger
		connect(myViewConfig[0], SIGNAL(openReadingFile(bool *, FlexseaDevice **)), \
				myDataLogger, SLOT(openReadingFile(bool *, FlexseaDevice **)));
		connect(myViewConfig[0], SIGNAL(closeReadingFile()), \
				myDataLogger, SLOT(closeReadingFile()));

		// Link to SerialDriver
		connect(myViewConfig[0], SIGNAL(openCom(QString,int,int, bool*)), \
				mySerialDriver, SLOT(open(QString,int,int, bool*)));
		connect(myViewConfig[0], SIGNAL(closeCom()), \
				mySerialDriver, SLOT(close()));
		connect(mySerialDriver, SIGNAL(openProgress(int)), \
				myViewConfig[0], SLOT(setComProgress(int)));
		connect(myViewConfig[0], SIGNAL(updateDataSourceStatus(DataSource, FlexseaDevice *)),
				this, SLOT(translatorUpdateDataSourceStatus(DataSource, FlexseaDevice *)));
		connect(myViewConfig[0], SIGNAL(createLogKeypad(DataSource, FlexseaDevice *)),
				this, SLOT(manageLogKeyPad(DataSource, FlexseaDevice *)));

		/*connect(myViewConfig[0], SIGNAL(writeCommand(uint8_t,uint8_t*,uint8_t)), \
				this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t))); */
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
	mdiState[CONFIG_WINDOWS_ID][0].open = false;	//ToDo shouldn't be 0

	if(W_LogKeyPad::howManyInstance() > 0)
	{
		myViewLogKeyPad[0]->parentWidget()->close();
		mdiState[LOGKEYPAD_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
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
		mdiState[CONTROL_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewControl[objectCount]);
		mdiState[CONTROL_WINDOWS_ID][objectCount].open = true;
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
	mdiState[CONTROL_WINDOWS_ID][0].open = false; //ToDo wrong, shouldn't be 0!
}

//Creates a new View 2DPlot window
void MainWindow::createView2DPlot(void)
{
	int objectCount = W_2DPlot::howManyInstance();

	//Limited number of windows:
	if(objectCount < (PLOT2D_WINDOWS_MAX))
	{
		myView2DPlot[objectCount] = new W_2DPlot(nullptr,
												 currentFlexLog,
												 getDisplayMode(),
												 &flexseaPtrlist,
												 activeSlaveNameStreaming);

		mdiState[PLOT2D_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myView2DPlot[objectCount]);
		mdiState[PLOT2D_WINDOWS_ID][objectCount].open = true;
		myView2DPlot[objectCount]->show();

		sendWindowCreatedMsg(W_2DPlot::getDescription(), objectCount,
							 W_2DPlot::getMaxWindow() - 1);

		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myView2DPlot[objectCount], SLOT(receiveNewData()));

		//Link to MainWindow for the close signal:
		connect(myView2DPlot[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeView2DPlot()));

		connect(this, SIGNAL(connectorCurrentSlaveStreaming(QString)), \
				myView2DPlot[objectCount], SLOT(activeSlaveStreaming(QString)));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myView2DPlot[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myView2DPlot[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[PLOT2D_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

void MainWindow::createSlaveComm(void)
{
	int objectCount = W_SlaveComm::howManyInstance();

	//Limited number of windows:
	if(objectCount < (SLAVECOMM_WINDOWS_MAX))
	{
		myViewSlaveComm[objectCount] = new W_SlaveComm(this,
													   &executeFlexList,
													   &manageFlexList,
													   &gossipFlexList,
													   &batteryFlexList,
													   &strainFlexList,
													   &ricnuFlexList,
													   &ankle2DofFlexList,
													   &testBenchFlexList,
													   &dynamicDeviceList,
													   &rigidFlexList,
													   streamManager);

		mdiState[SLAVECOMM_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewSlaveComm[objectCount]);
		mdiState[SLAVECOMM_WINDOWS_ID][objectCount].open = true;
		myViewSlaveComm[objectCount]->show();

		sendWindowCreatedMsg(W_SlaveComm::getDescription(), objectCount,
							 W_SlaveComm::getMaxWindow() - 1);

		connect(myViewSlaveComm[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeSlaveComm()));

		connect(myViewSlaveComm[objectCount], SIGNAL(activeSlaveStreaming(QString)), \
				this, SLOT(translatorActiveSlaveStreaming(QString)));

		connect(mySerialDriver, SIGNAL(openStatus(bool)), \
				myViewSlaveComm[0], SLOT(receiveComPortStatus(bool)));
		connect(mySerialDriver, SIGNAL(dataStatus(int, int)), \
				myViewSlaveComm[0], SLOT(displayDataReceived(int, int)));
		connect(mySerialDriver, SIGNAL(newDataTimeout(bool)), \
				myViewSlaveComm[0], SLOT(updateIndicatorTimeout(bool)));

		//myViewSlaveComm[objectCount]->addExperiment(&dynamicDeviceList, userDataManager->getCommandCode());
		myViewSlaveComm[objectCount]->addExperiment(&executeFlexList, W_AnkleTorque::getCommandCode());
		//myViewSlaveComm[objectCount]->addExperiment(&rigidFlexList, W_Rigid::getCommandCode());
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
	mdiState[SLAVECOMM_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new Any Command window
void MainWindow::createAnyCommand(void)
{
	int objectCount = W_AnyCommand::howManyInstance();

	//Limited number of windows:
	if(objectCount < (ANYCOMMAND_WINDOWS_MAX))
	{
		myViewAnyCommand[objectCount] = new W_AnyCommand(this);
		mdiState[ANYCOMMAND_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewAnyCommand[objectCount]);
		mdiState[ANYCOMMAND_WINDOWS_ID][objectCount].open = true;
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
	mdiState[ANYCOMMAND_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

void MainWindow::createInControl(void)
{
	int objectCount = W_InControl::howManyInstance();

	//Limited number of windows:
	if(objectCount < INCONTROL_WINDOWS_MAX)
	{
		myViewInControl[objectCount] = new W_InControl(this);
		mdiState[INCONTROL_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow((myViewInControl[objectCount]));
		mdiState[INCONTROL_WINDOWS_ID][objectCount].open = true;
		myViewInControl[objectCount]->show();
		QRect currRect = myViewInControl[objectCount]->geometry();
		currRect.setWidth(619);
		currRect.setHeight(639);
		myViewInControl[objectCount]->setGeometry(currRect);

		sendWindowCreatedMsg(W_InControl::getDescription(), objectCount,
		W_InControl::getMaxWindow() - 1);

		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewInControl[objectCount], SLOT(updateUIData()));
	}
	else
	{
		sendWindowCreatedFailedMsg(W_InControl::getDescription(),
		W_InControl::getMaxWindow());
	}
}

void MainWindow::closeInControl(void)
{
	sendCloseWindowMsg(W_InControl::getDescription());
	mdiState[INCONTROL_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new View RIC/NU window
void MainWindow::createViewRicnu(void)
{
	int objectCount = W_Ricnu::howManyInstance();

	//Limited number of windows:
	if(objectCount < (RICNU_VIEW_WINDOWS_MAX))
	{
		myViewRicnu[objectCount] = new W_Ricnu(this, &ricnuLog,
											   getDisplayMode(), &ricnuDevList);
		mdiState[RICNU_VIEW_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewRicnu[objectCount]);
		mdiState[RICNU_VIEW_WINDOWS_ID][objectCount].open = true;
		myViewRicnu[objectCount]->show();

		sendWindowCreatedMsg(W_Ricnu::getDescription(), objectCount,
							 W_Ricnu::getMaxWindow() - 1);

		//Link SerialDriver and RIC/NU:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewRicnu[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewRicnu[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewRicnu()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewRicnu[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewRicnu[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[RICNU_VIEW_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new Converter window
void MainWindow::createConverter(void)
{
	int objectCount = W_Converter::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CONVERTER_WINDOWS_MAX))
	{
		my_w_converter[objectCount] = new W_Converter(this);
		mdiState[CONVERTER_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(my_w_converter[objectCount]);
		mdiState[CONVERTER_WINDOWS_ID][objectCount].open = true;
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
	mdiState[CONVERTER_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new Calibration window
void MainWindow::createCalib(void)
{
	int objectCount = W_Calibration::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CALIB_WINDOWS_MAX))
	{
		myViewCalibration[objectCount] = new W_Calibration(this);
		mdiState[CALIB_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewCalibration[objectCount]);
		mdiState[CALIB_WINDOWS_ID][objectCount].open = true;
		myViewCalibration[objectCount]->show();

		sendWindowCreatedMsg(W_Calibration::getDescription(), objectCount,
							 W_Calibration::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewCalibration[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeCalib()));

		//Link to SlaveComm to send commands:
		connect(myViewCalibration[objectCount], SIGNAL(writeCommand(uint8_t,uint8_t*,uint8_t)), \
				this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)));
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
	mdiState[CALIB_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new User R/W window
void MainWindow::createUserRW(void)
{
	int objectCount = W_UserRW::howManyInstance();

	//Limited number of windows:
	if(objectCount < W_UserRW::getMaxWindow())
	{
		W_UserRW* userRW = new W_UserRW(this, userDataManager);
		myUserRW[objectCount] = userRW;
		mdiState[USERRW_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myUserRW[objectCount]);
		mdiState[USERRW_WINDOWS_ID][objectCount].open = true;
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

		connect(userDataManager, &DynamicUserDataManager::writeCommand, this, &MainWindow::connectorWriteCommand);
		connect(mySerialDriver, &SerialDriver::openStatus, userRW, &W_UserRW::comStatusChanged);
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
	mdiState[USERRW_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new View Gossip window
void MainWindow::createViewGossip(void)
{
	int objectCount = W_Gossip::howManyInstance();

	//Limited number of windows:
	if(objectCount < (GOSSIP_WINDOWS_MAX))
	{
		myViewGossip[objectCount] = new W_Gossip(this, &gossipLog,
												 getDisplayMode(), &gossipDevList);
		mdiState[GOSSIP_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewGossip[objectCount]);
		mdiState[GOSSIP_WINDOWS_ID][objectCount].open = true;
		myViewGossip[objectCount]->show();

		sendWindowCreatedMsg(W_Gossip::getDescription(), objectCount,
							 W_Gossip::getMaxWindow() - 1);

		//Link SerialDriver and Gossip:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewGossip[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewGossip[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewGossip()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewGossip[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewGossip[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[GOSSIP_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new View Rigid window
void MainWindow::createViewRigid(void)
{
	int objectCount = W_Rigid::howManyInstance();

	//Limited number of windows:
	if(objectCount < (RIGID_WINDOWS_MAX))
	{
		myViewRigid[objectCount] = new W_Rigid(this,
											   currentFlexLog,
											   &rigidLog,
											   getDisplayMode(),
											   &rigidDevList);
		mdiState[RIGID_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewRigid[objectCount]);
		mdiState[RIGID_WINDOWS_ID][objectCount].open = true;
		myViewRigid[objectCount]->show();

		sendWindowCreatedMsg(W_Rigid::getDescription(), objectCount,
							 W_Rigid::getMaxWindow() - 1);

		//Link SerialDriver and Rigid:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewRigid[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewRigid[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewGossip()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewRigid[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewRigid[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Rigid::getDescription(),
								   W_Rigid::getMaxWindow());
	}
}

void MainWindow::closeViewRigid(void)
{
	sendCloseWindowMsg(W_Rigid::getDescription());
	mdiState[RIGID_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new View Strain window
void MainWindow::createViewStrain(void)
{
	int objectCount = W_Strain::howManyInstance();

	//Limited number of windows:
	if(objectCount < (STRAIN_WINDOWS_MAX))
	{
		myViewStrain[objectCount] = new W_Strain(this, &strainLog,
												 getDisplayMode(), &strainDevList);
		mdiState[STRAIN_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewStrain[objectCount]);
		mdiState[STRAIN_WINDOWS_ID][objectCount].open = true;
		myViewStrain[objectCount]->show();

		sendWindowCreatedMsg(W_Strain::getDescription(), objectCount,
							 W_Strain::getMaxWindow() - 1);

		//Link SerialDriver and Strain:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewStrain[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewStrain[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewStrain()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewStrain[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewStrain[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[STRAIN_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new View Battery window
void MainWindow::createViewBattery(void)
{
	int objectCount = W_Battery::howManyInstance();

	//Limited number of windows:
	if(objectCount < (BATT_WINDOWS_MAX))
	{
		myViewBatt[objectCount] = new W_Battery(this,
												currentFlexLog,
												&batteryLog,
												&testBenchLog,
												getDisplayMode(),
												&batteryDevList);
		mdiState[BATT_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewBatt[objectCount]);
		mdiState[BATT_WINDOWS_ID][objectCount].open = true;
		myViewBatt[objectCount]->show();

		sendWindowCreatedMsg(W_Battery::getDescription(), objectCount,
							 W_Battery::getMaxWindow() - 1);

		//Link SerialDriver and Battery:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewBatt[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewBatt[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewBattery()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewBatt[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewBatt[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
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
	mdiState[BATT_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new LogKeyPad
void MainWindow::createLogKeyPad(FlexseaDevice *devPtr)
{
	int objectCount = W_LogKeyPad::howManyInstance();

	//Limited number of windows:
	if(objectCount < (LOGKEYPAD_WINDOWS_MAX))
	{
		myViewLogKeyPad[objectCount] = new W_LogKeyPad(this, devPtr);
		mdiState[LOGKEYPAD_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewLogKeyPad[objectCount]);
		mdiState[LOGKEYPAD_WINDOWS_ID][objectCount].open = true;
		myViewLogKeyPad[objectCount]->show();
		myViewLogKeyPad[objectCount]->parentWidget()->setWindowFlags(
					Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

		sendWindowCreatedMsg(W_LogKeyPad::getDescription(), objectCount,
							 W_LogKeyPad::getMaxWindow() - 1);

		// Link for the data slider
		connect(myViewLogKeyPad[objectCount], SIGNAL(logTimeSliderValueChanged(int, FlexseaDevice *)), \
				this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice*)));

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
	mdiState[LOGKEYPAD_WINDOWS_MAX][0].open = false;	//ToDo wrong, shouldn't be 0!
}

DisplayMode MainWindow::getDisplayMode(void)
{
	DisplayMode status = DisplayLiveData;
	if(W_Config::howManyInstance() > 0)
	{
		if(myViewConfig[0]->getDataSourceStatus() == FromLogFile)
		{
			status = DisplayLogData;
		}
	}
	return status;
}

//Creates a new View TestBench window
void MainWindow::createViewTestBench(void)
{
	int objectCount = W_TestBench::howManyInstance();

	//Limited number of windows:
	if(objectCount < (TESTBENCH_WINDOWS_MAX))
	{
		myViewTestBench[objectCount] = new W_TestBench(this,
													   &testBenchLog,
														getDisplayMode(),
													   &testBenchDevList);
		mdiState[TESTBENCH_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewTestBench[objectCount]);
		mdiState[TESTBENCH_WINDOWS_ID][objectCount].open = true;
		myViewTestBench[objectCount]->show();

		sendWindowCreatedMsg(W_TestBench::getDescription(), objectCount,
							 W_TestBench::getMaxWindow() - 1);

		//Link SerialDriver and Battery:
		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewTestBench[objectCount], SLOT(refreshDisplay()));

		//Link to MainWindow for the close signal:
		connect(myViewTestBench[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewBattery()));

		// Link to the slider of logKeyPad. Intermediate signal (connector) to
		// allow opening of window asynchroniously
		connect(this, SIGNAL(connectorRefreshLogTimeSlider(int, FlexseaDevice *)), \
				myViewTestBench[objectCount], SLOT(refreshDisplayLog(int, FlexseaDevice *)));
		connect(this, SIGNAL(connectorUpdateDisplayMode(DisplayMode, FlexseaDevice*)), \
				myViewTestBench[objectCount], SLOT(updateDisplayMode(DisplayMode, FlexseaDevice*)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_TestBench::getDescription(),
								   W_TestBench::getMaxWindow());
	}
}

void MainWindow::closeViewTestBench(void)
{
	sendCloseWindowMsg(W_TestBench::getDescription());
	mdiState[TESTBENCH_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new Comm. Test window
void MainWindow::createViewCommTest(void)
{
	int objectCount = W_CommTest::howManyInstance();

	//Limited number of windows:
	if(objectCount < W_CommTest::getMaxWindow())
	{
		myViewCommTest[objectCount] = new W_CommTest(this,
													 comPortStatus);

		myViewCommTest[objectCount]->serialDriver = mySerialDriver;

		mdiState[COMMTEST_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myViewCommTest[objectCount]);
		mdiState[COMMTEST_WINDOWS_ID][objectCount].open = true;
		myViewCommTest[objectCount]->show();

		sendWindowCreatedMsg(W_CommTest::getDescription(), objectCount,
							 W_CommTest::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myViewCommTest[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeViewCommTest()));

		//Link to SerialDriver to know when we receive data:
		connect(mySerialDriver, SIGNAL(openStatus(bool)), \
				myViewCommTest[objectCount], SLOT(receiveComPortStatus(bool)));

		connect(mySerialDriver, SIGNAL(newDataReady()), \
				myViewCommTest[objectCount], SLOT(receivedData()));

		//Link to SlaveComm to send commands:
		connect(myViewCommTest[objectCount], SIGNAL(writeCommand(uint8_t,\
				uint8_t*,uint8_t)), this, SIGNAL(connectorWriteCommand(uint8_t,\
				uint8_t*, uint8_t)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_CommTest::getDescription(),
								   W_CommTest::getMaxWindow());
	}
}

void MainWindow::closeViewCommTest(void)
{
	sendCloseWindowMsg(W_CommTest::getDescription());
	mdiState[COMMTEST_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new Event window
void MainWindow::createToolEvent(void)
{
	int objectCount = W_Event::howManyInstance();

	//Limited number of windows:
	if(objectCount < (EVENT_WINDOWS_MAX))
	{
		myEvent[objectCount] = new W_Event(this);
		mdiState[EVENT_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myEvent[objectCount]);
		mdiState[EVENT_WINDOWS_ID][objectCount].open = true;
		myEvent[objectCount]->show();

		sendWindowCreatedMsg(W_Event::getDescription(), objectCount,
							 W_Event::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myEvent[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeToolEvent()));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_Event::getDescription(),
								   W_Event::getMaxWindow());
	}
}

void MainWindow::closeToolEvent(void)
{
	sendCloseWindowMsg(W_Event::getDescription());
	mdiState[EVENT_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
}

//Creates a new CycleTester window
void MainWindow::createCycleTester(void)
{
	int objectCount = W_CycleTester::howManyInstance();

	//Limited number of windows:
	if(objectCount < (CYCLE_TESTER_WINDOWS_MAX))
	{
		myCycleTester[objectCount] = new W_CycleTester(this, &rigidFlexList, streamManager);
		mdiState[CYCLE_TESTER_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myCycleTester[objectCount]);
		mdiState[CYCLE_TESTER_WINDOWS_ID][objectCount].open = true;
		myCycleTester[objectCount]->show();

		sendWindowCreatedMsg(W_CycleTester::getDescription(), objectCount,
							 W_CycleTester::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myCycleTester[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeControlControl()));

		//Link to SlaveComm to send commands:
		connect(myCycleTester[objectCount], SIGNAL(writeCommand(uint8_t,uint8_t*,uint8_t)), \
				this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)));
	}
	else
	{
		sendWindowCreatedFailedMsg(W_CycleTester::getDescription(),
								   W_CycleTester::getMaxWindow());
	}
}

void MainWindow::closeCycleTester(void)
{
	sendCloseWindowMsg(W_CycleTester::getDescription());
	mdiState[CYCLE_TESTER_WINDOWS_ID][0].open = false; //ToDo wrong, shouldn't be 0!
}

//Creates a new User Testing window
void MainWindow::createUserTesting(void)
{
	int objectCount = W_UserTesting::howManyInstance();

	//Limited number of windows:
	if(objectCount < (USER_TESTING_WINDOWS_MAX))
	{
		myUserTesting[objectCount] = new W_UserTesting(this, appPath);
		mdiState[USER_TESTING_WINDOWS_ID][objectCount].winPtr = ui->mdiArea->addSubWindow(myUserTesting[objectCount]);
		mdiState[USER_TESTING_WINDOWS_ID][objectCount].open = true;
		myUserTesting[objectCount]->show();

		sendWindowCreatedMsg(W_UserTesting::getDescription(), objectCount,
							 W_UserTesting::getMaxWindow() - 1);

		//Link to MainWindow for the close signal:
		connect(myUserTesting[objectCount], SIGNAL(windowClosed()), \
				this, SLOT(closeUserTesting()));
		//Link to SlaveComm for experiment control:
		connect(myUserTesting[objectCount], SIGNAL(startExperiment(int, bool, bool, QString, QString)), \
				myViewSlaveComm[0] , SLOT(startExperiment(int, bool, bool, QString, QString)));
		connect(myUserTesting[objectCount], SIGNAL(stopExperiment()), \
				myViewSlaveComm[0] , SLOT(stopExperiment()));
		connect(myUserTesting[objectCount], SIGNAL(writeCommand(uint8_t,uint8_t*,uint8_t)), \
				this, SIGNAL(connectorWriteCommand(uint8_t,uint8_t*,uint8_t)));
		//Link to Datalogger to get filenames:
		connect(myDataLogger, SIGNAL(logFileName(QString, QString)), \
				myUserTesting[objectCount] , SLOT(logFileName(QString, QString)));

		//If there is no Event Flag window we create one:
		if(W_Event::howManyInstance() == 0)
		{
			createToolEvent();
		}

		//Link to Event to add flags to the log. Not that clean, but it gets the job done...:
		connect(myUserTesting[objectCount], SIGNAL(userFlags(int)), \
				myEvent[0] , SLOT(externalButtonClick(int)));
		connect(myEvent[0], SIGNAL(buttonClick(int)), \
				 myUserTesting[objectCount], SLOT(extFlags(int)));
	}

	else
	{
		sendWindowCreatedFailedMsg(W_UserTesting::getDescription(),
								   W_UserTesting::getMaxWindow());
	}
}

void MainWindow::closeUserTesting(void)
{
	sendCloseWindowMsg(W_UserTesting::getDescription());
	mdiState[USER_TESTING_WINDOWS_ID][0].open = false;	//ToDo wrong, shouldn't be 0!
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
	if(ui && ui->statusBar)
		ui->statusBar->showMessage(msg);
}

void MainWindow::displayAbout()
{
	QMessageBox::about(this, tr("About FlexSEA"), \
	tr("<center><u>FlexSEA: <b>Flex</b>ible, <b>S</b>calable <b>E</b>lectronics\
	 <b>A</b>rchitecture.</u><br><br>Project originaly developped at the \
	<a href='http://biomech.media.mit.edu/'>MIT Media Lab Biomechatronics \
	group</a>, now supported by <a href='http://dephy.com/'>Dephy, Inc.</a>\
	<br><br><b>Copyright &copy; Dephy, Inc. 2017</b><br><br>Software released \
	under the GNU GPL-3.0 license</center>"));
}

void MainWindow::displayLicense()
{
	QMessageBox::information(this, tr("Software License Information"), \
	tr("<center><b>Copyright &copy; Dephy, Inc. 2017</b>\
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

void MainWindow::writeSettings()
{
	QSettings settings("Dephy, Inc.", "Plan-GUI");

	settings.beginGroup("MainWindow");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.endGroup();
}

void MainWindow::readSettings()
{
	QSettings settings("Dephy, Inc.", "Plan-GUI");

	settings.beginGroup("MainWindow");
	resize(settings.value("size", QSize(400, 400)).toSize());
	move(settings.value("pos", QPoint(200, 200)).toPoint());
	restoreGeometry(settings.value("myWidget/geometry").toByteArray());
	restoreState(settings.value("myWidget/windowState").toByteArray());
	settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	qDebug() << "Closing, see you soon!";
	//writeSettings();
	event->accept();
}

void MainWindow::loadCSVconfigFile(void)
{
	QFile configFile;

	QString path = QDir::currentPath();
	QString filename = path + "/config.csv";

	//Now we open it:
	configFile.setFileName(filename);

	//Check if the file was successfully opened
	if(configFile.open(QIODevice::ReadOnly) == false)
	{
		qDebug() << "Couldn't open file " << filename;
		return;
	}

	qDebug() << "Opened:" << filename;

	if(configFile.size() == 0)
	{
		qDebug() << "Empty file.";
		return;
	}

	//Read and save the file information.
	QString line;
	QStringList splitLine;

	int on = 0, obj = 0, id = 0, x = 0, y = 0, w = 0, h = 0;
	line = configFile.readLine();	//Get rid of header
	while(!configFile.atEnd())
	{
		line = configFile.readLine();
		splitLine = line.split(',', QString::KeepEmptyParts);
		qDebug() << splitLine;

		id = splitLine.at(1).toInt();
		obj = splitLine.at(2).toInt();
		on = splitLine.at(3).toInt();
		x = splitLine.at(4).toInt();
		y = splitLine.at(5).toInt();
		w = splitLine.at(6).toInt();
		h = splitLine.at(7).toInt();
		if(on == 1)
		{
			if(id != CONFIG_WINDOWS_ID && id != SLAVECOMM_WINDOWS_ID)
			{
				//Create any extra windows:
				(this->*mdiCreateWinPtr[id])();	//Create window
			}
			setWinGeo(id, obj, x, y, w, h);	//Position it
		}
	}
}

void MainWindow::saveCSVconfigFile(void)
{
	QFile configFile;
	QTextStream cfStream;

	QString path = QDir::currentPath();
	QString filename = path + "/config.csv";

	//Now we open it:
	configFile.setFileName(filename);

	//Check if the file was successfully opened
	if(configFile.open(QIODevice::ReadWrite) == false)
	{
		qDebug() << "Couldn't RW open file " << filename;
		return;
	}

	qDebug() << "Opened:" << filename;
	cfStream.setDevice(&configFile);

	//CLear all data:
	configFile.resize(0);

	cfStream << "Nickname,id,objectCnt,visible,x,y,w,h" << endl;

	//We scan the list, and we save the needed info:
	for(int i = 0; i < WINDOWS_TYPES; i++)
	{
		for(int j = 0; j < WINDOWS_MAX_INSTANCES; j++)
		{
			if(mdiState[i][j].open == true)
			{
				//Window is open, we save its info:
				QRect rect = mdiState[i][j].winPtr->geometry();
				int x = rect.x();
				int y = rect.y();
				int w = rect.width();
				int h = rect.height();
				cfStream << "nickname," << QString::number(i) << ',' << QString::number(j) \
						 << ",1," <<  QString::number(x) << ',' << QString::number(y) \
						 << ',' << QString::number(w) << ',' << QString::number(h) << endl;
			}
		}
	}

	//Close file:
	configFile.close();
}

void MainWindow::setWinGeo(int id, int obj, int x, int y, int w, int h)
{
	mdiState[id][obj].winPtr->setGeometry(x,y,w,h);
}

void MainWindow::initMdiState(void)
{
	for(int i = 0; i < WINDOWS_TYPES; i++)
	{
		for(int j = 0; j < WINDOWS_MAX_INSTANCES; j++)
		{
			mdiState[i][j].open = false;
		}
	}
}
