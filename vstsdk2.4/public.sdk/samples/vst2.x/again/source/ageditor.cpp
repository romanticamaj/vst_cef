#ifndef __ageditor__
#include "ageditor.h"
#endif

#include <stdio.h>
#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>
#include <direct.h>
#include <string>
#include <sstream>

const int BUFSIZE = 512;

typedef struct Param {
	HANDLE pipeHandle;
	AGEditor *pEditor;
} ParamType;

DWORD WINAPI InstanceThread(LPVOID);

DWORD WINAPI PipeThreadWork(LPVOID lpvParam)
{	
	BOOL blConnected = FALSE;
	DWORD dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hThread = NULL;
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\vstvolumecontrolpipe");
	AGEditor *pEditor = static_cast<AGEditor*>(lpvParam);

	while (true) {
		//printf("\nPipe Server: Main thread awaiting client connection on %s\n", lpszPipename);
		const DWORD dwPipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
		
		hPipe = CreateNamedPipe(lpszPipename, PIPE_ACCESS_DUPLEX, dwPipeMode, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			printf("Failed to CreateNamedPipe. Error=%d.\n", GetLastError());
			return -1;
		} 

		/* Wait for pipe to connect */
		blConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (blConnected) {
			//printf("Client connected, creating a processing thread.\n");
			ParamType *Param = new ParamType{ hPipe, pEditor };

			hThread = CreateThread(NULL, 0, InstanceThread, static_cast<LPVOID>(Param), 0, &dwThreadId);

			if (hThread == NULL) {
				printf("Failed to CreateThread, Error=%d.\n", GetLastError());
				return -1;
			} else {
				CloseHandle(hThread);
			}
		} else {
			CloseHandle(hPipe);
		}
	}

	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	if (lpvParam == NULL) {
		printf("Error: The thread param is null.\n");
		return -1;
	}

	ParamType *Param = static_cast<ParamType*>(lpvParam);
	AGEditor *pEditor = static_cast<AGEditor*>(Param->pEditor);
	AudioEffectX* effect = dynamic_cast<AudioEffectX*>(pEditor->getEffect());
	HANDLE hPipe = Param->pipeHandle;

	if (effect == nullptr) {
		return -1;
	}

	DWORD cbBytesRead = 0;
	DWORD cbWritten = 0;
	BOOL blSuccess = FALSE;

	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = static_cast<TCHAR*>(HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR)));

	while (true) {
		blSuccess = ReadFile(hPipe, pchRequest, BUFSIZE * sizeof(TCHAR), &cbBytesRead, NULL);

		if (!blSuccess || cbBytesRead == 0) {
			if (GetLastError() == ERROR_BROKEN_PIPE) {
				//printf("Client disconnected.\n");
			} else {
				printf("Failed to ReadFile, Error=%d.\n", GetLastError());
			}
			break;
		}

		int vol = std::stoi(pchRequest);
		float perc = static_cast<float>(vol) / 100;

		effect->setParameterAutomated(0, perc);
	}

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	HeapFree(hHeap, 0, pchRequest);
	delete Param;

	//printf("InstanceThread exitting.\n");
	return 1;
}

/////////////////////////////////////////////////////

AGEditor::AGEditor(AudioEffect* effect)
	: AEffEditor(effect), m_pipeThreadHandle(NULL)
{
	effect->setEditor(this);
	ZeroMemory(&m_cefEditorProcInfo, sizeof(m_cefEditorProcInfo));

	m_rect.left = 0;
	m_rect.top = 0;
	m_rect.right = 90;
	m_rect.bottom = 260;
}

AGEditor::~AGEditor() {}

bool AGEditor::open(void *ptr)
{
	LPCSTR strEditorPath = getEditorPath();
	LPSTR strEditorArg = getEditorProcArg(ptr);
	STARTUPINFO si;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if (!CreateProcess(strEditorPath, strEditorArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &m_cefEditorProcInfo)) {
		printf("Failed to CreateProcess. Error=%d.\n", GetLastError());
		return false;
	}

	if (!createPipeThread()) {
		printf("Failed to Create pipe thread.\n");
		return false;
	}

	AEffEditor::open(ptr);

	return true;
}

void AGEditor::close()
{
	TerminateProcess(m_cefEditorProcInfo.hProcess, 0);
	//CloseHandle(m_cefEditorProcInfo.hProcess);
	//CloseHandle(m_cefEditorProcInfo.hThread);
}

bool AGEditor::getRect(ERect **ppErect)
{
	*ppErect = &m_rect;
	return true;
}

LPSTR AGEditor::getEditorProcArg(void *ptr)
{
	std::ostringstream sStream;
	sStream << "Dummy --hwnd=\"" << (int)ptr << "\"";
	LPSTR editorArg = _strdup(sStream.str().c_str());

	//printf("getEditorProcArg=[%s]\n", editorArg);

	return editorArg;
}

LPCSTR AGEditor::getEditorPath()
{
	HMODULE hModule = GetModuleHandle(_T("again.dll"));
	TCHAR dllPath[_MAX_PATH];
	GetModuleFileName(hModule, dllPath, _MAX_PATH);
	std::string strFullPath(dllPath);

	strFullPath = strFullPath.substr(0, strFullPath.find_last_of("\\/"));
	strFullPath += "\\AgainEditor\\message_router.exe";

	//strFullPath = "D:\\vst_cef\\cef-project\\build\\Release\\message_router.exe";
	//printf("getEditorPath=[%s]\n", strFullPath.c_str());

	return _strdup(strFullPath.c_str());
}

bool AGEditor::createPipeThread()
{
	DWORD dwThreadId = 0;
	LPVOID lpvParam = static_cast<LPVOID>(this);

	m_pipeThreadHandle = CreateThread(NULL, 0, PipeThreadWork, lpvParam, 0, &dwThreadId);

	if (m_pipeThreadHandle == NULL) {
		printf("Failed to CreateThread. Error=%d.\n", GetLastError());
		return false;
	}
	
	CloseHandle(m_pipeThreadHandle);
	
	return true;
}

