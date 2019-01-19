#ifndef __ageditor__
#define __ageditor__

#ifndef __aeffeditor__
#include "aeffeditor.h"
#endif

#include <windows.h>

class AGEditor : public AEffEditor
{
public:
	AGEditor(AudioEffect *effect);
	virtual ~AGEditor();

	virtual bool open(void *ptr);
	virtual void close();
	virtual bool getRect(ERect **ppErect);

private:
	LPSTR getEditorProcArg(void *ptr);
	LPCSTR getEditorPath();
	bool createPipeThread(); /* Receive UI value changed from editor */

private:
	ERect m_rect;
	PROCESS_INFORMATION m_cefEditorProcInfo;
	HANDLE m_pipeThreadHandle;
};

#endif