#pragma once
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "MQWidget.h"


class JSMacroWindow : public MQWindow
{
public:
	MQListBox *m_MessageList;

	JSMacroWindow(MQWindowBase& parent);

	void AddMessage(const char *message);
};
