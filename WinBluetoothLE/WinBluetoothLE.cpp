// WinBluetoothLE.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include "pch.h"
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <SetupAPI.h>
#include <RegStr.h>
#include <bthdef.h>
//#include <bluetoothapis.h>
#include <bluetoothleapis.h>

//Leave a comment in the generated object file. The linker can read it, 
//adding the library to the list of library dependencies.
#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothApis.lib")

//NOTE: change here to change the service...
//#define TO_SEARCH_DEVICE_UUID "{00000000-0000-1000-8000-00805F9B34FB}" //Base Bluetooth 
//#define TO_SEARCH_DEVICE_UUID "{00001800-0000-1000-8000-00805F9B34FB}" //Generic Access
#define TO_SEARCH_DEVICE_UUID "{00001801-0000-1000-8000-00805F9B34FB}" //GATT (Generic attributes)
//#define TO_SEARCH_DEVICE_UUID "{0000180D-0000-1000-8000-00805F9B34FB}" //Hart Rate



//this is the notification function
//the way ValueChangedEventParameters is utilized is shown in
//HealthHeartRateService::HeartRateMeasurementEvent()
//a function in Windows Driver Kit (WDK) 8.0 Samples.zip\C++\WDK 8.0 Samples\Bluetooth Low Energy (LE) Generic Attribute (GATT) Profile Drivers\Solution\WpdHealthHeartRate\HealthHeartRateService.cpp
void SomethingHappened(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	printf("notification obtained ");
	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

	HRESULT hr;
	if (0 == ValueChangedEventParameters->CharacteristicValue->DataSize) {
		hr = E_FAIL;
		printf("datasize 0\n");
	}
	else {
		printf("HR ");
		//for(int i=0; i<ValueChangedEventParameters->CharacteristicValue->DataSize;i++) {
		//	printf("%0x",ValueChangedEventParameters->CharacteristicValue->Data[i]);
		//}
		// if the first bit is set, then the value is the next 2 bytes.  If it is clear, the value is in the next byte
		//The Heart Rate Value Format bit (bit 0 of the Flags field) indicates if the data format of 
		//the Heart Rate Measurement Value field is in a format of UINT8 or UINT16. 
		//When the Heart Rate Value format is sent in a UINT8 format, the Heart Rate Value 
		//Format bit shall be set to 0. When the Heart Rate Value format is sent in a UINT16 
		//format, the Heart Rate Value Format bit shall be set to 1
		//from this PDF https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=239866
		unsigned heart_rate;
		if (0x01 == (ValueChangedEventParameters->CharacteristicValue->Data[0] & 0x01)) {
			heart_rate = ValueChangedEventParameters->CharacteristicValue->Data[1] * 256 + ValueChangedEventParameters->CharacteristicValue->Data[2];
		}
		else {
			heart_rate = ValueChangedEventParameters->CharacteristicValue->Data[1];
		}
		printf("%d\n", heart_rate);
	}
}

HANDLE GetBLEHandle(__in GUID AGuid) {
	HDEVINFO hDI;
	SP_DEVICE_INTERFACE_DATA did;
	SP_DEVINFO_DATA dd;
	GUID BluetoothInterfaceGUID = AGuid;
	HANDLE hComm = NULL;

	hDI = SetupDiGetClassDevs(&BluetoothInterfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (hDI == INVALID_HANDLE_VALUE) {
		printf("ERROR: Invalid handle value...\r\n");
		return NULL;
	}

	did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	dd.cbSize = sizeof(SP_DEVINFO_DATA);

	DWORD dword=0;
	BOOL _bval = SetupDiEnumDeviceInterfaces(hDI, NULL, &BluetoothInterfaceGUID, dword, &did);


	for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDI, NULL, &BluetoothInterfaceGUID, i, &did); i++)
	{
		printf("LOG: dentro il for#1 ...\r\n");
		SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

		DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		DWORD size = 0;

		if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, NULL, 0, &size, 0))
		{
			int err = GetLastError();

			if (err == ERROR_NO_MORE_ITEMS) break;
			//if (err == ERROR_INSUFFICIENT_BUFFER) {
				//err=122 the Area of the data transferred on a system call, it is too small. 
			//	printf("ERROR INSUFFICIENT BUFFER");
			//	break;
			//}

			PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, size);

			pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, pInterfaceDetailData, size, &size, &dd))
				break;
					
			hComm = CreateFile(
				pInterfaceDetailData->DevicePath,
				GENERIC_WRITE | GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			err = GetLastError();
			if (err == ERROR_NOT_SUPPORTED) {
				printf("ERROR_NOT_SUPPORTED\r\n");
			}

			

			GlobalFree(pInterfaceDetailData);
		}
	}

	SetupDiDestroyDeviceInfoList(hDI);
	return hComm;
}//EndFunction.

int main()
{
	GUID AGuid;
	CLSIDFromString(TEXT(TO_SEARCH_DEVICE_UUID), &AGuid);

	//Get the device handle.
	HANDLE hLEDevice = GetBLEHandle(AGuid);

	if (hLEDevice == INVALID_HANDLE_VALUE) {
		printf("ERROR: Invalid handle ...");
		return -1;
	}

	//Check the handle.
	std::cout << "Hello World!\n"; 
	
	//Step 2: Get a list of services that the device advertises
		// first send 0,NULL as the parameters to BluetoothGATTServices inorder to get the number of
		// services in serviceBufferCount
	USHORT serviceBufferCount;
	////////////////////////////////////////////////////////////////////////////
	// Determine Services Buffer Size
	////////////////////////////////////////////////////////////////////////////

	HRESULT hr = BluetoothGATTGetServices(
		hLEDevice,
		0,
		NULL,
		&serviceBufferCount,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		printf("BluetoothGATTGetServices - Buffer Size %d", hr);
	}

	PBTH_LE_GATT_SERVICE pServiceBuffer = (PBTH_LE_GATT_SERVICE)
		malloc(sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);

	if (NULL == pServiceBuffer) {
		printf("pServiceBuffer out of memory\r\n");
	}
	else {
		RtlZeroMemory(pServiceBuffer,
			sizeof(BTH_LE_GATT_SERVICE) * serviceBufferCount);
	}

	////////////////////////////////////////////////////////////////////////////
	// Retrieve Services
	////////////////////////////////////////////////////////////////////////////

	USHORT numServices;
	hr = BluetoothGATTGetServices(
		hLEDevice,
		serviceBufferCount,
		pServiceBuffer,
		&numServices,
		BLUETOOTH_GATT_FLAG_NONE);

	if (S_OK != hr) {
		printf("BluetoothGATTGetServices - Buffer Size %d\r\n", hr);
	}


	//Step 3: now get the list of charactersitics. note how the pServiceBuffer is required from step 2
	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Buffer Size
	////////////////////////////////////////////////////////////////////////////

	USHORT charBufferSize;
	hr = BluetoothGATTGetCharacteristics(
		hLEDevice,
		pServiceBuffer,
		0,
		NULL,
		&charBufferSize,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		printf("BluetoothGATTGetCharacteristics - Buffer Size %d", hr);
	}

	PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = NULL;
	if (charBufferSize > 0) {
		pCharBuffer = (PBTH_LE_GATT_CHARACTERISTIC)
			malloc(charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));

		if (NULL == pCharBuffer) {
			printf("pCharBuffer out of memory\r\n");
		}
		else {
			RtlZeroMemory(pCharBuffer,
				charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));
		}

		////////////////////////////////////////////////////////////////////////////
		// Retrieve Characteristics
		////////////////////////////////////////////////////////////////////////////
		USHORT numChars;
		hr = BluetoothGATTGetCharacteristics(
			hLEDevice,
			pServiceBuffer,
			charBufferSize,
			pCharBuffer,
			&numChars,
			BLUETOOTH_GATT_FLAG_NONE);

		if (S_OK != hr) {
			printf("BluetoothGATTGetCharacteristics - Actual Data %d", hr);
		}

		if (numChars != charBufferSize) {
			printf("buffer size and buffer size actual size mismatch\r\n");
		}
	}


	//Step 4: now get the list of descriptors. note how the pCharBuffer is required from step 3
	//descriptors are required as we descriptors that are notification based will have to be written
	//once IsSubcribeToNotification set to true, we set the appropriate callback function
	//need for setting descriptors for notification according to
	//http://social.msdn.microsoft.com/Forums/en-US/11d3a7ce-182b-4190-bf9d-64fefc3328d9/windows-bluetooth-le-apis-event-callbacks?forum=wdk
	PBTH_LE_GATT_CHARACTERISTIC currGattChar;
	for (int ii = 0; ii < charBufferSize; ii++) {
		currGattChar = &pCharBuffer[ii];
		USHORT charValueDataSize;
		PBTH_LE_GATT_CHARACTERISTIC_VALUE pCharValueBuffer;


		///////////////////////////////////////////////////////////////////////////
		// Determine Descriptor Buffer Size
		////////////////////////////////////////////////////////////////////////////
		USHORT descriptorBufferSize;
		hr = BluetoothGATTGetDescriptors(
			hLEDevice,
			currGattChar,
			0,
			NULL,
			&descriptorBufferSize,
			BLUETOOTH_GATT_FLAG_NONE);

		if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
			printf("BluetoothGATTGetDescriptors - Buffer Size %d", hr);
		}

		PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;
		if (descriptorBufferSize > 0) {
			pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)
				malloc(descriptorBufferSize
					* sizeof(BTH_LE_GATT_DESCRIPTOR));

			if (NULL == pDescriptorBuffer) {
				printf("pDescriptorBuffer out of memory\r\n");
			}
			else {
				RtlZeroMemory(pDescriptorBuffer, descriptorBufferSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve Descriptors
			////////////////////////////////////////////////////////////////////////////

			USHORT numDescriptors;
			hr = BluetoothGATTGetDescriptors(
				hLEDevice,
				currGattChar,
				descriptorBufferSize,
				pDescriptorBuffer,
				&numDescriptors,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTGetDescriptors - Actual Data %d", hr);
			}

			if (numDescriptors != descriptorBufferSize) {
				printf("buffer size and buffer size actual size mismatch\r\n");
			}

			for (int kk = 0; kk < numDescriptors; kk++) {
				PBTH_LE_GATT_DESCRIPTOR  currGattDescriptor = &pDescriptorBuffer[kk];
				////////////////////////////////////////////////////////////////////////////
				// Determine Descriptor Value Buffer Size
				////////////////////////////////////////////////////////////////////////////
				USHORT descValueDataSize;
				hr = BluetoothGATTGetDescriptorValue(
					hLEDevice,
					currGattDescriptor,
					0,
					NULL,
					&descValueDataSize,
					BLUETOOTH_GATT_FLAG_NONE);

				if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
					printf("BluetoothGATTGetDescriptorValue - Buffer Size %d", hr);
				}

				PBTH_LE_GATT_DESCRIPTOR_VALUE pDescValueBuffer = (PBTH_LE_GATT_DESCRIPTOR_VALUE)malloc(descValueDataSize);

				if (NULL == pDescValueBuffer) {
					printf("pDescValueBuffer out of memory\r\n");
				}
				else {
					RtlZeroMemory(pDescValueBuffer, descValueDataSize);
				}

				////////////////////////////////////////////////////////////////////////////
				// Retrieve the Descriptor Value
				////////////////////////////////////////////////////////////////////////////

				hr = BluetoothGATTGetDescriptorValue(
					hLEDevice,
					currGattDescriptor,
					(ULONG)descValueDataSize,
					pDescValueBuffer,
					NULL,
					BLUETOOTH_GATT_FLAG_NONE);
				if (S_OK != hr) {
					printf("BluetoothGATTGetDescriptorValue - Actual Data %d", hr);
				}
				//you may also get a descriptor that is read (and not notify) andi am guessing the attribute handle is out of limits
				// we set all descriptors that are notifiable to notify us via IsSubstcibeToNotification
				if (currGattDescriptor->AttributeHandle < 255) {
					BTH_LE_GATT_DESCRIPTOR_VALUE newValue;

					RtlZeroMemory(&newValue, sizeof(newValue));

					newValue.DescriptorType = ClientCharacteristicConfiguration;
					newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;

					hr = BluetoothGATTSetDescriptorValue(
						hLEDevice,
						currGattDescriptor,
						&newValue,
						BLUETOOTH_GATT_FLAG_NONE);
					if (S_OK != hr) {
						printf("BluetoothGATTGetDescriptorValue - Actual Data %d", hr);
					}
					else {
						printf("setting notification for serivice handle %d\n", currGattDescriptor->ServiceHandle);
					}

				}

			}
		}


		//set the appropriate callback function when the descriptor change value
		BLUETOOTH_GATT_EVENT_HANDLE EventHandle;

		if (currGattChar->IsNotifiable) {
			printf("Setting Notification for ServiceHandle %d\n", currGattChar->ServiceHandle);
			BTH_LE_GATT_EVENT_TYPE EventType = CharacteristicValueChangedEvent;

			BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION EventParameterIn;
			EventParameterIn.Characteristics[0] = *currGattChar;
			EventParameterIn.NumCharacteristics = 1;
			hr = BluetoothGATTRegisterEvent(
				hLEDevice,
				EventType,
				&EventParameterIn,
				(PFNBLUETOOTH_GATT_EVENT_CALLBACK) SomethingHappened,
				NULL,
				&EventHandle,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTRegisterEvent - Actual Data %d", hr);
			}
		}


		if (currGattChar->IsReadable) {//currGattChar->IsReadable
			////////////////////////////////////////////////////////////////////////////
			// Determine Characteristic Value Buffer Size
			////////////////////////////////////////////////////////////////////////////
			hr = BluetoothGATTGetCharacteristicValue(
				hLEDevice,
				currGattChar,
				0,
				NULL,
				&charValueDataSize,
				BLUETOOTH_GATT_FLAG_NONE);

			if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Buffer Size %d", hr);
			}

			pCharValueBuffer = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(charValueDataSize);

			if (NULL == pCharValueBuffer) {
				printf("pCharValueBuffer out of memory\r\n");
			}
			else {
				RtlZeroMemory(pCharValueBuffer, charValueDataSize);
			}

			////////////////////////////////////////////////////////////////////////////
			// Retrieve the Characteristic Value
			////////////////////////////////////////////////////////////////////////////

			hr = BluetoothGATTGetCharacteristicValue(
				hLEDevice,
				currGattChar,
				(ULONG)charValueDataSize,
				pCharValueBuffer,
				NULL,
				BLUETOOTH_GATT_FLAG_NONE);

			if (S_OK != hr) {
				printf("BluetoothGATTGetCharacteristicValue - Actual Data %d", hr);
			}

			//print the characeteristic Value
			//for an HR monitor this might be the body sensor location
			printf("\n Printing a read (not notifiable) characterstic (maybe) body sensor value");
			for (int iii = 0; iii < pCharValueBuffer->DataSize; iii++) {// ideally check ->DataSize before printing
				printf("%d", pCharValueBuffer->Data[iii]);
			}
			printf("\n");


			// Free before going to next iteration, or memory leak.
			free(pCharValueBuffer);
			pCharValueBuffer = NULL;
		}

	}

	//go into an inf loop that sleeps. you will ideally see notifications from the HR device
	while (1) {
		Sleep(1000);
		//printf("look for notification");
	}

	CloseHandle(hLEDevice);



	if (GetLastError() != NO_ERROR &&
		GetLastError() != ERROR_NO_MORE_ITEMS)
	{
		// Insert error handling here.
		return 1;
	}

	return 0;
}

// Per eseguire il programma: CTRL+F5 oppure Debug > Avvia senza eseguire debug
// Per eseguire il debug del programma: F5 oppure Debug > Avvia debug

// Suggerimenti per iniziare: 
//   1. Usare la finestra Esplora soluzioni per aggiungere/gestire i file
//   2. Usare la finestra Team Explorer per connettersi al controllo del codice sorgente
//   3. Usare la finestra di output per visualizzare l'output di compilazione e altri messaggi
//   4. Usare la finestra Elenco errori per visualizzare gli errori
//   5. Passare a Progetto > Aggiungi nuovo elemento per creare nuovi file di codice oppure a Progetto > Aggiungi elemento esistente per aggiungere file di codice esistenti al progetto
//   6. Per aprire di nuovo questo progetto in futuro, passare a File > Apri > Progetto e selezionare il file con estensione sln
