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
	[This file] w_calibration.h: Calibration View Window
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-15 | jfduval | New code
	*
****************************************************************************/

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdint.h>
#include "w_calibration.h"
#include "flexsea_generic.h"
#include "ui_w_calibration.h"
#include "flexsea_system.h"
#include "flexsea_cmd_calibration.h"
#include <QString>
#include <QTextStream>
#include <flexsea_comm.h>
#include <flexsea_board.h>
#include <QDebug>
#include "cmd-ActPack.h"

//****************************************************************************
// Constructor & Destructor:
//****************************************************************************

W_Calibration::W_Calibration(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::W_Calibration)
{
	ui->setupUi(this);

	setWindowTitle(this->getDescription());
	setWindowIcon(QIcon(":icons/d_logo_small.png"));

	init();
}

W_Calibration::~W_Calibration()
{
	emit windowClosed();
	delete ui;
}

//****************************************************************************
// Public function(s):
//****************************************************************************


//****************************************************************************
// Public slot(s):
//****************************************************************************

void W_Calibration::on_pbFindPoles_clicked()
{
	active_slave_index = ui->comboBox_slave->currentIndex();
	active_slave = FlexSEA_Generic::getSlaveID(SL_BASE_ALL, active_slave_index);

	uint16_t numBytes = 0;
	uint8_t info[2] = {PORT_USB, PORT_USB};

	//we should check first that active slave is an execute
	tx_cmd_calibration_mode_rw(TX_N_DEFAULT, CALIBRATION_FIND_POLES);
	pack(P_AND_S_DEFAULT, active_slave, info, &numBytes, comm_str_usb);
	emit writeCommand(numBytes, comm_str_usb, WRITE);
}
//****************************************************************************
// Private function(s):
//****************************************************************************

void W_Calibration::init(void)
{
	//Populates Slave list:
	FlexSEA_Generic::populateSlaveComboBox(ui->comboBox_slave, SL_BASE_ALL, \
											SL_LEN_ALL);

	//Variables:
	active_slave_index = ui->comboBox_slave->currentIndex();
	active_slave = FlexSEA_Generic::getSlaveID(SL_BASE_ALL, active_slave_index);

	//Disable current zero (not yet implemented):
	ui->dispCurrentZero->setText("0");
	ui->lineEditCurrentFind->setText("0");
	ui->dispCurrentZero->setEnabled(0);
	ui->pbCurrentFind->setEnabled(0);
	ui->pbCurrentSave->setEnabled(0);
	ui->pbCurrentRead->setEnabled(0);
	ui->lineEditCurrentFind->setEnabled(0);

	//Rigid/Actuator Package:
	initActPack();
	ui->comboBoxActPackFSM2->addItem("Enabled (default)");
	ui->comboBoxActPackFSM2->addItem("Disabled");

	//Indicate the end of the calibration:
	calibration = 0;
}

//****************************************************************************
// Private slot(s):
//****************************************************************************

void W_Calibration::on_comboBox_slave_currentIndexChanged(int index)
{
	active_slave_index = ui->comboBox_slave->currentIndex();
	active_slave = FlexSEA_Generic::getSlaveID(SL_BASE_ALL, active_slave_index);

	if((index >= SL_BASE_MN) && (index < (SL_BASE_MN + SL_LEN_MN)))
	{
		ui->comboBoxActPackFSM2->setEnabled(true);
	}
	else
	{
		ui->comboBoxActPackFSM2->setEnabled(false);
	}
}

void W_Calibration::on_comboBoxActPackFSM2_currentIndexChanged(int index)
{
	if(!calibration)
	{
		qDebug() << "FSM2 state changed (" << index << ")";

		if(!index)
		{
			//Enabled (default):
			ActPack.system = SYS_NORMAL;
		}
		else
		{
			ActPack.system = SYS_DISABLE_FSM2;
		}
	}

	sendActPack();
}

//Initialize ActPack
void W_Calibration::initActPack(void)
{
	ActPack.controller = CTRL_NONE;
	ActPack.setpoint = 0;
	ActPack.setGains = KEEP;
	ActPack.g0 = 0;
	ActPack.g1 = 0;
	ActPack.g2 = 0;
	ActPack.g3 = 0;
	ActPack.system = 0;
}

//Send the ActPack command. It will use the ActPack structure values.
void W_Calibration::sendActPack(void)
{
	uint8_t info[2] = {PORT_USB, PORT_USB};
	uint16_t numb = 0;
	uint8_t offset = 0;

	//Debugging only:
	qDebug() << "[ActPack]";
	qDebug() << "Controller: " << ActPack.controller;
	qDebug() << "Setpoint: " << ActPack.setpoint;
	qDebug() << "Set Gains: " << ActPack.setGains;
	qDebug() << "g0: " << ActPack.g0;
	qDebug() << "g1: " << ActPack.g1;
	qDebug() << "g2: " << ActPack.g2;
	qDebug() << "g3: " << ActPack.g3;
	qDebug() << "system: " << ActPack.system;

	//Send command:
	tx_cmd_actpack_rw(TX_N_DEFAULT, offset, ActPack.controller, ActPack.setpoint, \
					  ActPack.setGains, ActPack.g0, ActPack.g1, ActPack.g2, \
					  ActPack.g3, ActPack.system);
	pack(P_AND_S_DEFAULT, active_slave, info, &numb, comm_str_usb);
	emit writeCommand(numb, comm_str_usb, WRITE);

	if(ActPack.setGains == CHANGE)
	{
		ActPack.setGains = KEEP;
	}
}
