#pragma once
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "MQWidget.h"



class WindowCallback {
public:
	virtual void OnCloseWindow(MQDocument doc) {};
	virtual void ExecuteFile(const std::string &path, MQDocument doc) {};
	virtual void ExecuteString(const std::string &code, MQDocument doc) {};
};

class JSMacroWindow : public MQWindow {
public:
	JSMacroWindow(MQWindowBase &parent, WindowCallback &callback);

	void AddMessage(const std::string &message, int tag = 0);

private:
	WindowCallback &m_callback;

	MQListBox *m_MessageList;
	MQEdit *m_FilePathEdit;

	BOOL OnHide(MQWidgetBase *sender, MQDocument doc);
	BOOL onDrawListItem(MQWidgetBase * sender, MQDocument doc, MQListBoxDrawItemParam & param);
	BOOL OnClearClick(MQWidgetBase *sender, MQDocument doc);
	BOOL OnOpenScriptClick(MQWidgetBase *sender, MQDocument doc);
	BOOL OnExecuteClick(MQWidgetBase *sender, MQDocument doc);
};
