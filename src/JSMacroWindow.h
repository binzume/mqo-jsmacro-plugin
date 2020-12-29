#pragma once

#include <windows.h>

#include "MQWidget.h"
#include "preference.h"

class WindowCallback {
 public:
  virtual void OnCloseWindow(MQDocument doc){};
  virtual void ExecuteScript(const std::string &path, MQDocument doc){};
  virtual void OnSettingUpdated(){};
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
  MQEdit *m_EditorCommandEdit;
  MQEdit *m_LogFilePathEdit;
  MQListBox *m_MessageList;
  static BOOL MQAPICALL ExecuteProc(MQDocument doc, void *option);
  BOOL OnHide(MQWidgetBase *sender, MQDocument doc);
  BOOL onDrawListItem(MQWidgetBase *sender, MQDocument doc,
                      MQListBoxDrawItemParam &param);
  BOOL OnClearClick(MQWidgetBase *sender, MQDocument doc);
  BOOL OnOpenScriptClick(MQWidgetBase *sender, MQDocument doc);
  BOOL OnEditScriptClick(MQWidgetBase *sender, MQDocument doc);
  BOOL OnExecuteClick(MQWidgetBase *sender, MQDocument doc);
  BOOL OnSavePresetClick(MQWidgetBase *sender, MQDocument doc);
  BOOL OnSelectEditorButtonClicked(MQWidgetBase *sender, MQDocument doc);
  BOOL OnSelectLogFileButtonClicked(MQWidgetBase *sender, MQDocument doc);
};
