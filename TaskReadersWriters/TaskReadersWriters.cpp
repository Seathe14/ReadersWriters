#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <tchar.h>
#define NUMREADERS 5
#define NUMWRITERS 5
#define NUMREPEAT 5
#define TIMEDELAY 2
// global variables
bool bWritersActive = false;
int counter = 1;

// global Handles
HANDLE hSignalEvent;
HANDLE hWriterMutex;
HANDLE hReaderMutex;
CRITICAL_SECTION CriticalSection;	
void CloseHandleArr(HANDLE* arr, int size)
{
	for (int i = 0; i < size; i++)
	{
		CloseHandle(arr[i]);
	}
}

DWORD WINAPI writeToFile(void* ptrID)
{
	int id = (int)ptrID;
	WaitForSingleObject(hSignalEvent, INFINITE);
	WaitForSingleObject(hWriterMutex, INFINITE);
	counter = counter * 2;
	std::cout << "Writer #" << id << " modified counter to  " << counter <<std::endl;
	ReleaseMutex(hWriterMutex);
	SetEvent(hSignalEvent);
	return 0;
}
DWORD WINAPI readFromFile(void* ptrID)
{
	int id = (int)ptrID;
	static int readers = 0;
	WaitForSingleObject(hReaderMutex, INFINITE);
	readers++;
	if (readers == 1)
	{
		WaitForSingleObject(hSignalEvent, INFINITE);
	}
	ReleaseMutex(hReaderMutex);
	EnterCriticalSection(&CriticalSection);
	std::cout << "Reader #" << id << " read " << counter << std::endl;
	LeaveCriticalSection(&CriticalSection);
	WaitForSingleObject(hReaderMutex, INFINITE);
	readers--;
	if (readers == 0)
		SetEvent(hSignalEvent);
	ReleaseMutex(hReaderMutex);
	return 0;
}
DWORD WINAPI runWriters(void *)
{
	HANDLE writers[NUMWRITERS];
	bWritersActive = true;
	for (int i = 0; i < NUMWRITERS; i++)
	{
		Sleep(TIMEDELAY);

		writers[i] = CreateThread(NULL, 0, writeToFile, (void*)i, NULL, 0);;
	}
	WaitForMultipleObjects(NUMWRITERS, writers, true, INFINITE);
	CloseHandleArr(writers, NUMWRITERS);
	return 0;
}
DWORD WINAPI runReaders(void*)
{
	HANDLE readers[NUMREADERS];
	while (!bWritersActive) // to prevent the situation when readers start reading an empty file.
		Sleep(1);
	for (int i = 0; i < NUMREADERS; i++)
	{
		Sleep(TIMEDELAY);
		readers[i] = CreateThread(NULL, 0, readFromFile, (void*)i, NULL, 0);
	}
	WaitForMultipleObjects(NUMREADERS, readers, true, INFINITE);
	CloseHandleArr(readers, NUMREADERS);
	return 0;
}

int main()  
{
	hSignalEvent = CreateEvent(NULL, FALSE, TRUE, _T("SignalEvent"));
	hWriterMutex = CreateMutex(NULL, FALSE, _T("WriterMutex"));
	hReaderMutex = CreateMutex(NULL, FALSE, _T("ReaderMutex"));
	InitializeCriticalSection(&CriticalSection);
	for (int i = 0; i < NUMREPEAT; i++)
	{
		HANDLE RWThreads[] =
		{
			CreateThread(NULL, 0, runReaders, nullptr, 0, 0),
			CreateThread(NULL, 0, runWriters, nullptr, 0, 0)
		};
		WaitForMultipleObjects(2, RWThreads, true, INFINITE);
		std::cout << "This is " << i << " th repeat\n" << std::endl;
		counter = 1;
		Sleep(TIMEDELAY);
		CloseHandleArr(RWThreads, 2);
	}
	DeleteCriticalSection(&CriticalSection);
	if(hReaderMutex != NULL)
		CloseHandle(hReaderMutex);
	if(hSignalEvent != NULL)
		CloseHandle(hSignalEvent);
	if(hWriterMutex != NULL)
		CloseHandle(hWriterMutex);
}