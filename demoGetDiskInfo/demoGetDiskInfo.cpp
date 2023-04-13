#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#define DISK_PATH_LEN 256
using namespace std;
struct diskInfo
{
	string parNum; //分区号
	int diskNum;   //磁盘号
	string serivalNum;
	string productId;
	string vendId;
	double parTotal; //分区总量
	double parFree;  //分区可用
};

void QueryPropertyForDevice(HANDLE, diskInfo& );
void getParInfo(PSTORAGE_DEVICE_DESCRIPTOR, diskInfo&);

int GetPhysicalNumFromPartition(char letter);
void getParInfo(char letter, diskInfo& );

	
int main(int argc, char *argv[])
{

	DWORD dwDriveList = ::GetLogicalDrives();
	vector<diskInfo> vec;
	for (int i = 2; i < 26; i++)
	{
		if (dwDriveList & (1 << i))
		{
			char letter = (char)('A' + i);

			diskInfo info;
			info.parNum = letter;
			DWORD num = GetPhysicalNumFromPartition(letter);
			info.diskNum = num;
			getParInfo(letter , info);
			char path[DISK_PATH_LEN];
			memset(path, 0, DISK_PATH_LEN);
			sprintf_s(path, "\\\\.\\PhysicalDrive%d", num);
			HANDLE hDevice = CreateFile(path,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if (hDevice == INVALID_HANDLE_VALUE)
			{
				return 0;
			}
			QueryPropertyForDevice(hDevice, info);
			CloseHandle(hDevice);
			vec.push_back(move(info));
		}
	}
	printf("------------disk info---------\n");
	printf("分区号	磁盘	产品ID	供应商ID	分区总量	分区可用	磁盘序列号	\n");
	for (auto v : vec)
	{
		printf("%s	磁盘%d	%s	%s	%.2fG		%.2fG		%s\n",
			v.parNum.c_str(),
			v.diskNum,
			v.productId.c_str(),
			v.vendId.c_str(),
			v.parTotal,
			v.parFree,
			v.serivalNum.c_str()
		);
	}
	return 0;
}
void getParInfo(char letter , diskInfo& info)
{
	ULARGE_INTEGER uliTotalNumBytes;
	ULARGE_INTEGER uliTotalFreeBytes;
	ULARGE_INTEGER uliTotalAvailableToCaller;

	CHAR path[DISK_PATH_LEN];
	sprintf_s(path, "%c:", letter);
	BOOL fResult = GetDiskFreeSpaceEx(path,
		&uliTotalAvailableToCaller, &uliTotalNumBytes, &uliTotalFreeBytes);
	if (fResult)
	{
		// since version 10200521, send size MB
		info.parTotal = (double)uliTotalNumBytes.QuadPart / 1024 / 1024 / 1024;
		info.parFree = (double)uliTotalFreeBytes.QuadPart / 1024 / 1024 / 1024;
	}
	else
	{
		DWORD err = GetLastError();
	}
}


void QueryPropertyForDevice(HANDLE hDevice, diskInfo& info)
{
	bool result;

	STORAGE_PROPERTY_QUERY query;
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;



	STORAGE_DESCRIPTOR_HEADER device_des_head = { 0 };
	DWORD readed;
	device_des_head.Size = sizeof(STORAGE_DESCRIPTOR_HEADER);
	result = DeviceIoControl(hDevice,     // device to be queried
		IOCTL_STORAGE_QUERY_PROPERTY,     // operation to perform
		&query, sizeof(query),               // no input buffer
		&device_des_head, sizeof(STORAGE_DESCRIPTOR_HEADER),     // output buffer
		&readed,                 // # bytes returned
		NULL);      // synchronous I/O
	if (!result)
	{
		return;
	}


	vector<char> outpu_buf(device_des_head.Size);

	result = DeviceIoControl(hDevice,     // device to be queried
		IOCTL_STORAGE_QUERY_PROPERTY,     // operation to perform
		&query, sizeof query,               // no input buffer
		&outpu_buf[0], device_des_head.Size,     // output buffer
		&readed,                 // # bytes returned
		NULL);      // synchronous I/O
	if (!result)
	{
		CloseHandle(hDevice);
		return;
	}
	STORAGE_DEVICE_DESCRIPTOR * device_descriptor = reinterpret_cast <STORAGE_DEVICE_DESCRIPTOR *> (&outpu_buf[0]);
	getParInfo(device_descriptor ,info);

}

void  getParInfo(PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor, diskInfo& info)
{
	LPCSTR vendorId = "";
	LPCSTR productId = "";
	LPCSTR productRevision = "";
	LPCSTR serialNumber = "";



	if ((DeviceDescriptor->ProductIdOffset != 0) &&
		(DeviceDescriptor->ProductIdOffset != -1)) {
		productId = (LPCSTR)(DeviceDescriptor);
		productId += (ULONG_PTR)DeviceDescriptor->ProductIdOffset;
	}
	if ((DeviceDescriptor->VendorIdOffset != 0) &&
		(DeviceDescriptor->VendorIdOffset != -1)) {
		vendorId = (LPCSTR)(DeviceDescriptor);
		vendorId += (ULONG_PTR)DeviceDescriptor->VendorIdOffset;
	}
	if ((DeviceDescriptor->SerialNumberOffset != 0) &&
		(DeviceDescriptor->SerialNumberOffset != -1))
	{
		serialNumber = (LPCSTR)(DeviceDescriptor);
		serialNumber += (ULONG_PTR)DeviceDescriptor->SerialNumberOffset;
	}

	info.serivalNum = serialNumber;
	info.productId = productId;
	info.vendId = vendorId;
}


int GetPhysicalNumFromPartition(char letter)
{
	HANDLE hDevice;
	BOOL result;
	DWORD readed;
	STORAGE_DEVICE_NUMBER number;

	CHAR path[DISK_PATH_LEN];
	sprintf_s(path, "\\\\.\\%c:", letter);
	hDevice = CreateFile(path,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "CreateFile() Error: %ld\n", GetLastError());
		return -1;
	}

	result = DeviceIoControl(
		hDevice,
		IOCTL_STORAGE_GET_DEVICE_NUMBER,
		NULL,
		0,
		&number,
		sizeof(number),
		&readed,
		NULL
	);
	if (!result)
	{
		CloseHandle(hDevice);
		return -1;
	}

	CloseHandle(hDevice);
	return number.DeviceNumber;
}
