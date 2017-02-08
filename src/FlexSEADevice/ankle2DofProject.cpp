/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'plan-gui' Graphical User Interface
	Copyright (C) 2017 Dephy, Inc. <http://dephy.com/>

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
	[This file] ankle2DofProject: ankle2Dof Project Data Class
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2017-01-16 | sbelanger | Initial GPL-3.0 release
	*
****************************************************************************/

//****************************************************************************
// Include(s)
//****************************************************************************

#include "ankle2DofProject.h"
#include <QDebug>
#include <QTextStream>
#include "executeDevice.h"

//****************************************************************************
// Constructor & Destructor:
//****************************************************************************

Ankle2DofProject::Ankle2DofProject(void): FlexseaDevice()
{
	if(header.length() != headerDecoded.length())
	{
		qDebug() << "Mismatch between header lenght Ankle2DOF!";
	}

	this->dataSource = LogDataFile;
	serializedLength = header.length();
	slaveType = "ankle2Dof";
}

Ankle2DofProject::Ankle2DofProject(execute_s *ex1Ptr, execute_s *ex2Ptr): FlexseaDevice()
{
	if(header.length() != headerDecoded.length())
	{
		qDebug() << "Mismatch between header lenght Ankle2DOF!";
	}

	this->dataSource = LiveDataFile;
	timeStamp.append(TimeStamp());
	akList.append(new ankle2Dof_s_plan());
	akList.last()->ex1 = ex1Ptr;
	akList.last()->ex2 = ex2Ptr;
	serializedLength = header.length();
	slaveType = "ankle2Dof";
}

//****************************************************************************
// Public function(s):
//****************************************************************************

QString Ankle2DofProject::getHeaderStr(void)
{
	return header.join(',');
}

QStringList Ankle2DofProject::header = QStringList()
								<< "Timestamp"
								<< "Timestamp (ms)"
								<< "ex1 Accel X"
								<< "ex1 Accel Y"
								<< "ex1 Accel Z"
								<< "ex1 Gyro X"
								<< "ex1 Gyro Y"
								<< "ex1 Gyro Z"
								<< "ex1 Strain"
								<< "ex1 Analog[0]"
								<< "ex1 Analog[1]"
								<< "ex1 Current"
								<< "ex1 Encoder Display"
								<< "ex1 Encoder Control"
								<< "ex1 Encoder Commutation"
								<< "ex1 Battery Voltage"
								<< "ex1 Int. Voltage"
								<< "ex1 Temperature"
								<< "ex1 Status1"
								<< "ex1 Status2"
								<< "ex2 Accel X"
								<< "ex2 Accel Y"
								<< "ex2 Accel Z"
								<< "ex2 Gyro X"
								<< "ex2 Gyro Y"
								<< "ex2 Gyro Z"
								<< "ex2 Strain"
								<< "ex2 Analog[0]"
								<< "ex2 Analog[1]"
								<< "ex2 Current"
								<< "ex2 Encoder Display"
								<< "ex2 Encoder Control"
								<< "ex2 Encoder Commutation"
								<< "ex2 Battery Voltage"
								<< "ex2 Int. Voltage"
								<< "ex2 Temperature"
								<< "ex2 Status1"
								<< "ex2 Status2";

QStringList Ankle2DofProject::headerDecoded = QStringList()
								<< "Raw Value Only"
								<< "Raw Value Only"
								<< "Decoded: mg"
								<< "Decoded: mg"
								<< "Decoded: mg"
								<< "Decoded: deg/s"
								<< "Decoded: deg/s"
								<< "Decoded: deg/s"
								<< "Decoded: ±100%"
								<< "Decoded: mV"
								<< "Decoded: mV"
								<< "Decoded: mA"
								<< "Raw Value Only"
								<< "Raw Value Only"
								<< "Raw Value Only"
								<< "Decoded: mV"
								<< "Decoded: mV"
								<< "Decoded: 10x C"
								<< "Raw value only"
								<< "Raw value only"
								<< "Decoded: mg"
								<< "Decoded: mg"
								<< "Decoded: mg"
								<< "Decoded: deg/s"
								<< "Decoded: deg/s"
								<< "Decoded: deg/s"
								<< "Decoded: ±100%"
								<< "Decoded: mV"
								<< "Decoded: mV"
								<< "Decoded: mA"
								<< "Raw Value Only"
								<< "Raw Value Only"
								<< "Raw Value Only"
								<< "Decoded: mV"
								<< "Decoded: mV"
								<< "Decoded: 10x C"
								<< "Raw value only"
								<< "Raw value only";

QString Ankle2DofProject::getLastSerializedStr(void)
{
	QString str;
	QTextStream(&str) <<	timeStamp.last().date			<< ',' << \
							timeStamp.last().ms				<< ',' << \
							akList.last()->ex1->accel.x		<< ',' << \
							akList.last()->ex1->accel.y		<< ',' << \
							akList.last()->ex1->accel.z		<< ',' << \
							akList.last()->ex1->gyro.x		<< ',' << \
							akList.last()->ex1->gyro.y		<< ',' << \
							akList.last()->ex1->gyro.z		<< ',' << \
							akList.last()->ex1->strain		<< ',' << \
							akList.last()->ex1->analog[0]	<< ',' << \
							akList.last()->ex1->analog[1]	<< ',' << \
							akList.last()->ex1->current		<< ',' << \
							akList.last()->ex1->enc_display	<< ',' << \
							akList.last()->ex1->enc_control	<< ',' << \
							akList.last()->ex1->enc_commut	<< ',' << \
							akList.last()->ex1->volt_batt	<< ',' << \
							akList.last()->ex1->volt_int	<< ',' << \
							akList.last()->ex1->temp		<< ',' << \
							akList.last()->ex1->status1		<< ',' << \
							akList.last()->ex1->status2		<< ',' << \
							akList.last()->ex2->accel.x		<< ',' << \
							akList.last()->ex2->accel.y		<< ',' << \
							akList.last()->ex2->accel.z		<< ',' << \
							akList.last()->ex2->gyro.x		<< ',' << \
							akList.last()->ex2->gyro.y		<< ',' << \
							akList.last()->ex2->gyro.z		<< ',' << \
							akList.last()->ex2->strain		<< ',' << \
							akList.last()->ex2->analog[0]	<< ',' << \
							akList.last()->ex2->analog[1]	<< ',' << \
							akList.last()->ex2->current		<< ',' << \
							akList.last()->ex2->enc_display	<< ',' << \
							akList.last()->ex2->enc_control	<< ',' << \
							akList.last()->ex2->enc_commut	<< ',' << \
							akList.last()->ex2->volt_batt	<< ',' << \
							akList.last()->ex2->volt_int	<< ',' << \
							akList.last()->ex2->temp		<< ',' << \
							akList.last()->ex2->status1		<< ',' << \
							akList.last()->ex2->status2;;
	return str;
}

void Ankle2DofProject::appendSerializedStr(QStringList *splitLine)
{
	//Check if data line contain the number of data expected
	if(splitLine->length() >= serializedLength)
	{
		// Because of the pointer architecture of ankle2Dof_s_plan , we also
		// need to also add data structure
		appendEmptyLineWithStruct();

		timeStamp.last().date				= (*splitLine)[0];
		timeStamp.last().ms					= (*splitLine)[1].toInt();
		akList.last()->ex1->accel.x			= (*splitLine)[2].toInt();
		akList.last()->ex1->accel.y			= (*splitLine)[3].toInt();
		akList.last()->ex1->accel.z			= (*splitLine)[4].toInt();
		akList.last()->ex1->gyro.x			= (*splitLine)[5].toInt();
		akList.last()->ex1->gyro.y			= (*splitLine)[6].toInt();
		akList.last()->ex1->gyro.z			= (*splitLine)[7].toInt();
		akList.last()->ex1->strain			= (*splitLine)[8].toInt();
		akList.last()->ex1->analog[0]		= (*splitLine)[9].toInt();
		akList.last()->ex1->analog[1]		= (*splitLine)[10].toInt();
		akList.last()->ex1->current			= (*splitLine)[11].toInt();
		akList.last()->ex1->enc_display		= (*splitLine)[12].toInt();
		akList.last()->ex1->enc_control		= (*splitLine)[13].toInt();
		akList.last()->ex1->enc_commut		= (*splitLine)[14].toInt();
		akList.last()->ex1->volt_batt		= (*splitLine)[15].toInt();
		akList.last()->ex1->volt_int		= (*splitLine)[16].toInt();
		akList.last()->ex1->temp			= (*splitLine)[17].toInt();
		akList.last()->ex1->status1			= (*splitLine)[18].toInt();
		akList.last()->ex1->status2			= (*splitLine)[19].toInt();
		akList.last()->ex2->accel.x			= (*splitLine)[20].toInt();
		akList.last()->ex2->accel.y			= (*splitLine)[21].toInt();
		akList.last()->ex2->accel.z			= (*splitLine)[22].toInt();
		akList.last()->ex2->gyro.x			= (*splitLine)[23].toInt();
		akList.last()->ex2->gyro.y			= (*splitLine)[24].toInt();
		akList.last()->ex2->gyro.z			= (*splitLine)[25].toInt();
		akList.last()->ex2->strain			= (*splitLine)[26].toInt();
		akList.last()->ex2->analog[0]		= (*splitLine)[27].toInt();
		akList.last()->ex2->analog[1]		= (*splitLine)[28].toInt();
		akList.last()->ex2->current			= (*splitLine)[29].toInt();
		akList.last()->ex2->enc_display		= (*splitLine)[30].toInt();
		akList.last()->ex2->enc_control		= (*splitLine)[31].toInt();
		akList.last()->ex2->enc_commut		= (*splitLine)[32].toInt();
		akList.last()->ex2->volt_batt		= (*splitLine)[33].toInt();
		akList.last()->ex2->volt_int		= (*splitLine)[34].toInt();
		akList.last()->ex2->temp			= (*splitLine)[35].toInt();
		akList.last()->ex2->status1			= (*splitLine)[36].toInt();
		akList.last()->ex2->status2			= (*splitLine)[37].toInt();
	}
}

struct std_variable Ankle2DofProject::getSerializedVar(int parameter)
{
	return getSerializedVar(parameter, 0);
}

struct std_variable Ankle2DofProject::getSerializedVar(int parameter, int index)
{
	struct std_variable var;

	if(index >= akList.length())
	{
		parameter = INT_MAX;
	}

	//Assign pointer:
	switch(parameter)
	{
		/*Format: (every Case except Unused)
		 * Line 1: data format, raw variable
		 * Line 2: raw variable
		 * Line 3: decoded variable (always int32),
					null if not decoded  */
		case 0: //"TimeStamp"
			var.format = FORMAT_QSTR;
			var.rawGenPtr = &timeStamp[index].date;
			var.decodedPtr = nullptr;
			break;
		case 1: //"TimeStamp (ms)"
			var.format = FORMAT_32S;
			var.rawGenPtr = &timeStamp[index].ms;
			var.decodedPtr = nullptr;
			break;
		case 2: //"Ex1 Accel X"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->accel.x;
			var.decodedPtr = &akList[index]->ex1->decoded.accel.x;
			break;
		case 3: //"Ex1 Accel Y"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->accel.y;
			var.decodedPtr = &akList[index]->ex1->decoded.accel.y;
			break;
		case 4: //"Ex1 Accel Z"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->accel.z;
			var.decodedPtr = &akList[index]->ex1->decoded.accel.z;
			break;
		case 5: //"Ex1 Gyro X"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->gyro.x;
			var.decodedPtr = &akList[index]->ex1->decoded.gyro.x;
			break;
		case 6: //"Ex1 Gyro Y"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->gyro.y;
			var.decodedPtr = &akList[index]->ex1->decoded.gyro.y;
			break;
		case 7: //"Ex1 Gyro Z"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->gyro.z;
			var.decodedPtr = &akList[index]->ex1->decoded.gyro.z;
			break;
		case 8: //"Ex1 Strain"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex1->strain;
			var.decodedPtr = &akList[index]->ex1->decoded.strain;
			break;
		case 9: //"Ex1 Analog[0]"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex1->analog[0];
			var.decodedPtr = &akList[index]->ex1->decoded.analog[0];
			break;
		case 10: //"Ex1 Analog[1]"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex1->analog[1];
			var.decodedPtr = &akList[index]->ex1->decoded.analog[1];
			break;
		case 11: //"Ex1 Current"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex1->current;
			var.decodedPtr = &akList[index]->ex1->decoded.current;
			break;
		case 12: //"Ex1 Enc_Display"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex1->enc_display;
			var.decodedPtr = nullptr;
			break;
		case 13: //"Ex1 Encoder Control"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex1->enc_control;
			var.decodedPtr = nullptr;
			break;
		case 14: //"Ex1 Encoder Commut"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex1->enc_commut;
			var.decodedPtr = nullptr;
		case 15: //"Ex1 Battery Voltage"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex1->volt_batt;
			var.decodedPtr = &akList[index]->ex1->decoded.volt_batt;
		case 16: //"Ex1 Internal Voltage"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex1->volt_int;
			var.decodedPtr = &akList[index]->ex1->decoded.volt_int;
		case 17: //"Ex1 Temperature"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex1->temp;
			var.decodedPtr = &akList[index]->ex1->decoded.temp;
			break;
		case 18: //"Ex1 Status1"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex1->status1;
			var.decodedPtr = nullptr;
			break;
		case 19: //"Ex1 Status2"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex1->status2;
			var.decodedPtr = nullptr;
			break;
		case 20: //"Ex2 Accel X"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->accel.x;
			var.decodedPtr = &akList[index]->ex2->decoded.accel.x;
			break;
		case 21: //"Ex2 Accel Y"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->accel.y;
			var.decodedPtr = &akList[index]->ex2->decoded.accel.y;
			break;
		case 22: //"Ex2 Accel Z"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->accel.z;
			var.decodedPtr = &akList[index]->ex2->decoded.accel.z;
			break;
		case 23: //"Ex2 Gyro X"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->gyro.x;
			var.decodedPtr = &akList[index]->ex2->decoded.gyro.x;
			break;
		case 24: //"Ex2 Gyro Y"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->gyro.y;
			var.decodedPtr = &akList[index]->ex2->decoded.gyro.y;
			break;
		case 25: //"Ex2 Gyro Z"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->gyro.z;
			var.decodedPtr = &akList[index]->ex2->decoded.gyro.z;
			break;
		case 26: //"Ex2 Strain"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex2->strain;
			var.decodedPtr = &akList[index]->ex2->decoded.strain;
			break;
		case 27: //"Ex2 Analog[0]"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex2->analog[0];
			var.decodedPtr = &akList[index]->ex2->decoded.analog[0];
			break;
		case 28: //"Ex2 Analog[1]"
			var.format = FORMAT_16U;
			var.rawGenPtr = &akList[index]->ex2->analog[1];
			var.decodedPtr = &akList[index]->ex2->decoded.analog[1];
			break;
		case 29: //"Ex2 Current"
			var.format = FORMAT_16S;
			var.rawGenPtr = &akList[index]->ex2->current;
			var.decodedPtr = &akList[index]->ex2->decoded.current;
			break;
		case 30: //"Ex2 Enc_Display"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex2->enc_display;
			var.decodedPtr = nullptr;
			break;
		case 31: //"Ex2 Encoder Control"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex2->enc_control;
			var.decodedPtr = nullptr;
			break;
		case 32: //"Ex2 Encoder Commut"
			var.format = FORMAT_32S;
			var.rawGenPtr = &akList[index]->ex2->enc_commut;
			var.decodedPtr = nullptr;
		case 33: //"Ex2 Battery Voltage"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex2->volt_batt;
			var.decodedPtr = &akList[index]->ex2->decoded.volt_batt;
		case 34: //"Ex2 Internal Voltage"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex2->volt_int;
			var.decodedPtr = &akList[index]->ex2->decoded.volt_int;
		case 35: //"Ex2 Temperature"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex2->temp;
			var.decodedPtr = &akList[index]->ex2->decoded.temp;
			break;
		case 36: //"Ex2 Status1"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex2->status1;
			var.decodedPtr = nullptr;
			break;
		case 37: //"Ex2 Status2"
			var.format = FORMAT_8U;
			var.rawGenPtr = &akList[index]->ex2->status2;
			var.decodedPtr = nullptr;
			break;
		default:
			var.format = NULL_PTR;
			var.rawGenPtr = nullptr;
			var.decodedPtr = nullptr;
			break;
	}

	return var;
}

void Ankle2DofProject::clear(void)
{
	FlexseaDevice::clear();
	akList.clear();
	timeStamp.clear();
}

void Ankle2DofProject::appendEmptyLine(void)
{
	timeStamp.append(TimeStamp());
	akList.append(new ankle2Dof_s_plan());
}

void Ankle2DofProject::appendEmptyLineWithStruct(void)
{
	appendEmptyLine();
	akList.last()->ex1 = new execute_s();
	akList.last()->ex2 = new execute_s();
}

void Ankle2DofProject::decodeLastLine(void)
{
	decode(akList.last());
}

void Ankle2DofProject::decodeAllLine(void)
{
	for(int i = 0; i < akList.size(); ++i)
	{
		decode(akList[i]);
	}
}

void Ankle2DofProject::decode(struct ankle2Dof_s_plan *akPtr)
{
	ExecuteDevice::decode(akPtr->ex1);
	ExecuteDevice::decode(akPtr->ex2);
}

QString Ankle2DofProject::getStatusStr(int index)
{
	(void)index;	//Unused

	return QString("No decoding available for this board");
}

//****************************************************************************
// Public slot(s):
//****************************************************************************


//****************************************************************************
// Private function(s):
//****************************************************************************


//****************************************************************************
// Private slot(s):
//****************************************************************************

