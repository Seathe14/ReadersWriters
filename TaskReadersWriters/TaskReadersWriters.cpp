#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <tchar.h>

#define NUMREADERS 5
#define NUMWRITERS 20
#define NUMREPEAT 5
#define TIMEDELAY 50
#define BUFSIZE 128
// global variables
LPCTSTR lpFileName = _T("SharedFile.txt");
bool bWritersActive = false;

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
    DWORD cbWritten;
    HANDLE hFile = CreateFile(lpFileName, FILE_APPEND_DATA , FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "File does not exist\n";
        hFile = CreateFile(lpFileName, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::cout << "File creating failed. The code of the error: " << GetLastError() << std::endl;
            return 1;
        }
    }
	char buf[BUFSIZE] = "";
	for (int i = 0; i < BUFSIZE; i++)
		buf[i] = '\0';
    buf[BUFSIZE - 1] = '\n';
	char toWrite[BUFSIZE];
    for (int i = 0; i < NUMREPEAT;i++)
    {
		std::cout << "Writer #" << id << " writes " << i << std::endl;
		sprintf(toWrite, "This is a %d writer, %d iteration", id,i);
		strcpy(buf, toWrite);
        WriteFile(hFile, buf, sizeof(buf), &cbWritten, NULL);
        Sleep(TIMEDELAY);
    }
	ReleaseMutex(hWriterMutex);
	SetEvent(hSignalEvent);
	CloseHandle(hFile);
	return 0;
}
DWORD WINAPI readFromFile(void* ptrID)
{
	int id = (int)ptrID;
	DWORD cbRead;
	HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "File does not exist\n";
		return 1;
	}
	char buf[BUFSIZE];
	static int readers = 0;
	while(bWritersActive)
	{
		WaitForSingleObject(hReaderMutex, INFINITE);
		readers++;
		if (readers == 1)
		{
			WaitForSingleObject(hSignalEvent, INFINITE);
		}
		ReleaseMutex(hReaderMutex);
		while (ReadFile(hFile, buf, sizeof(buf), &cbRead, NULL))
		{
			if (cbRead == 0)
				break;
			EnterCriticalSection(&CriticalSection);
			std::cout << buf << " read by " << id << " reader thread" << std::endl;
			LeaveCriticalSection(&CriticalSection);
		}
		WaitForSingleObject(hReaderMutex, INFINITE);
		readers--;
		if (readers == 0)
		{
			SetEvent(hSignalEvent);
		}
		ReleaseMutex(hReaderMutex);
	}
	CloseHandle(hFile);

	return 0;
}
DWORD WINAPI runWriters(void *)
{
	HANDLE writers[NUMWRITERS];
	bWritersActive = true;
	for (int i = 0; i < NUMWRITERS; i++)
	{
		writers[i] = CreateThread(NULL, 0, writeToFile, (void*)i, NULL, 0);;
	}
	WaitForMultipleObjects(NUMWRITERS, writers, true, INFINITE);
	bWritersActive = false;
	CloseHandleArr(writers, NUMWRITERS);
	return 0;
}
DWORD WINAPI runReaders(void*)
{
	HANDLE readers[NUMREADERS];
	InitializeCriticalSection(&CriticalSection);
	while (!bWritersActive) // to prevent the situation when readers start reading an empty file.
		Sleep(1);
	for (int i = 0; i < NUMREADERS; i++)
	{
		readers[i] = CreateThread(NULL, 0, readFromFile, (void*)i, NULL, 0);
	}
	WaitForMultipleObjects(NUMREADERS, readers, true, INFINITE);
	DeleteCriticalSection(&CriticalSection);
	CloseHandleArr(readers, NUMREADERS);
	return 0;
}

int main()  
{
	HANDLE hFile = CreateFile(lpFileName, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	hSignalEvent = CreateEvent(NULL, FALSE, TRUE, _T("SignalEvent"));
    hWriterMutex = CreateMutex(NULL, FALSE, _T("WriterMutex"));
	hReaderMutex = CreateMutex(NULL, FALSE, _T("ReaderMutex"));
	HANDLE RWThreads[] =
	{
		CreateThread(NULL, 0, runReaders, nullptr, 0, 0),
		CreateThread(NULL, 0, runWriters, nullptr, 0, 0)
	};	
	WaitForMultipleObjects(2, RWThreads, true, INFINITE);
	CloseHandleArr(RWThreads, 2);
	CloseHandle(hFile);
	if(hReaderMutex != NULL)
		CloseHandle(hReaderMutex);
	if(hSignalEvent != NULL)
		CloseHandle(hSignalEvent);
	if(hWriterMutex != NULL)
		CloseHandle(hWriterMutex);
}