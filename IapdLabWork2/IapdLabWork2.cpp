// IapdLabWork2.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include "HDDInfo.h"
using namespace std;

void showVector(vector<string> v) {
	if (v.size() == 0) {
		cout << "\tNone information" << endl;
		return;
	}
	for (int i = 0; i < v.size(); i++) {
		cout << '\t' << v[i] << endl;
	}
}

int main()
{
	try {
		HDDinfo hi;
		cout << "Model number: " << hi.getModelNumber() << endl;
		cout << "Firmware revision: " << hi.getFirmwareRevision() << endl;
		cout << "Serial number: " << hi.getSerialNumber() << endl;
		cout << "Total space:\t" << setprecision(5) << ((double)hi.getTotalSpace()) / 1024 / 1024 / 1024 << " GB" << endl;
		cout << "Free space:\t" << setprecision(5) << ((double)hi.getFreeSpace()) / 1024 / 1024 / 1024 << " GB" << endl;
		cout << "Occupied space:\t" << setprecision(5) << ((double)hi.getOccupiedSpace()) / 1024 / 1024 / 1024 << " GB" << endl;
		cout << endl;
		cout << "ATA support: " << endl;
		showVector(hi.getATASupport());
		cout << "SATA support: " << endl;
		showVector(hi.getSATASupport());
		string res = hi.isPIO() ? "PIO" : "DMA";
		cout << endl;
		cout << "Transfer mode: " << res << endl;
		cout << "PIO support: " << endl;
		showVector(hi.getPIOSupport());
		cout << "DMA support: " << endl;
		showVector(hi.getDMASupport());
		cout << "MultiwordDMA support: " << endl;
		showVector(hi.getMultiwordDMASupport());
		cout << "UltraDMA support: " << endl;
		showVector(hi.getUltraDMASupport());
	}
	catch (const runtime_error& re)
	{
		cerr << "Runtime error: " << re.what() << endl;
		cout << "Try to open the application with administrator rights." << endl;
	}
	catch (const exception& ex)
	{
		cerr << "Error occurred: " << ex.what() << endl;
		cout << "Try to open the application with administrator rights." << endl;
	}
	catch (...)
	{
		cerr << "Unknown failure occurred. Possible memory corruption" << endl;
		cout << "Try to open the application with administrator rights." << endl;
	}
	getchar();
	return 0;
}

