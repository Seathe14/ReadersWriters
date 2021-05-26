#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <thread>
#include <tchar.h>
#define NUMREADERS 3
#define NUMWRITERS 10
#define NUMREPEAT 5

#define TIMEDELAY 300
#define BUFSIZE 128
LPCTSTR lpFileName = _T("SharedFile.txt");
std::thread readers[NUMREADERS];
std::thread writers[NUMWRITERS];
HANDLE hEvent;
HANDLE hWriterEvent;
HANDLE hWriterMutex;
HANDLE hReaderMutex;
CRITICAL_SECTION CriticalSection;
bool bWritersActive = true;
void writeToFile(int id)
{
	WaitForSingleObject(hWriterEvent, INFINITE);
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
            return;
        }
    }
	char buf[BUFSIZE] = "";
	for (int i = 0; i < BUFSIZE; i++)
		buf[i] = '\0';
    buf[BUFSIZE - 1] = '\n';
    char toWrite[32];
    for (int i = 0; i < NUMREPEAT;i++)
    {
		std::cout << "Writer #" << id << " writes " << i << std::endl;
		sprintf(toWrite, "This is a %d writer %d", id,i);
		strcpy(buf, toWrite);
        WriteFile(hFile, buf, sizeof(buf), &cbWritten, NULL);
        Sleep(TIMEDELAY);
    }
	ReleaseMutex(hWriterMutex);
	SetEvent(hWriterEvent);

}
void readFromFile(int id)
{
    
	DWORD cbRead;
	std::thread::id t_id = readers[id].get_id();
	HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "File does not exist\n";
		return;
	}
	char buf[BUFSIZE];
	static int readers = 0;
	while(bWritersActive)
	{
		WaitForSingleObject(hReaderMutex, INFINITE);
		readers++;
		if (readers == 1)
		{
			WaitForSingleObject(hWriterEvent, INFINITE);
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
			SetEvent(hWriterEvent);
		}
		ReleaseMutex(hReaderMutex);
	}
}
int main()  
{	
    hEvent = CreateEvent(NULL, FALSE, FALSE, _T("isDoneEvent"));
	hWriterEvent = CreateEvent(NULL, FALSE, TRUE, _T("isWriterDoneEvent"));
    hWriterMutex = CreateMutex(NULL, FALSE, _T("WriterMutex"));
	hReaderMutex = CreateMutex(NULL, FALSE, _T("ReaderMutex"));

    InitializeCriticalSection(&CriticalSection);
	for (int i = 0; i < NUMREADERS; i++)
	{
		readers[i] = std::thread(readFromFile, i);
	}

	for (int i = 0; i < NUMWRITERS; i++)
	{
		writers[i] = std::thread(writeToFile, i);
	}

    for (int i = 0; i < NUMWRITERS; i++)
    {
		writers[i].join();
    }
	bWritersActive = false;
	for (int i = 0; i < NUMREADERS; i++)
	{
		readers[i].join();
	}
    DeleteCriticalSection(&CriticalSection);
}

