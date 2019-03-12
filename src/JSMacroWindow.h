#pragma once
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "MQWidget.h"

#define PRESET_SCRIPT_COUNT 5

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
	BOOL Execute(MQDocument doc, int preset = -1);

private:
	WindowCallback &m_callback;

	MQEdit *m_FilePathEdit;
	MQEdit *m_PresetEdit[PRESET_SCRIPT_COUNT];
	MQListBox *m_MessageList;
	static BOOL MQAPICALL ExecuteProc(MQDocument doc, void *option);
	BOOL OnHide(MQWidgetBase *sender, MQDocument doc);
	BOOL onDrawListItem(MQWidgetBase * sender, MQDocument doc, MQListBoxDrawItemParam & param);
	BOOL OnClearClick(MQWidgetBase *sender, MQDocument doc);
	BOOL OnOpenScriptClick(MQWidgetBase *sender, MQDocument doc);
	BOOL OnExecuteClick(MQWidgetBase *sender, MQDocument doc);
	BOOL OnSavePresetClick(MQWidgetBase *sender, MQDocument doc);
};
