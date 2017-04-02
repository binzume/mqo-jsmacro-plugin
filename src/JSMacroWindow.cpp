
#include <string>
#include <sstream>
#include <codecvt>
#include "JSMacroWindow.h"
#include "MQBasePlugin.h"
#include "MQSetting.h"

JSMacroWindow::JSMacroWindow(MQWindowBase& parent, WindowCallback &callback) : MQWindow(parent), m_callback(callback) {
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "ja_JP.UTF-8");

	SetTitle(L"JSMacro");
	SetOutSpace(0.4);

	MQTab *tab = CreateTab(this);

	MQFrame *mainFrame = CreateVerticalFrame(tab);
	tab->SetTabTitle(0, L"Script");

	mainFrame->SetVertLayout(MQWidgetBase::LAYOUT_FILL);
	mainFrame->SetHintSizeRateX(40.0);
	mainFrame->SetHintSizeRateY(30.0);

	// location
	MQFrame *locationFrame = CreateHorizontalFrame(mainFrame);

	m_FilePathEdit = CreateEdit(locationFrame);
	m_FilePathEdit->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);


	MQButton *openButton = CreateButton(locationFrame, L"...");
	openButton->AddClickEvent(this, &JSMacroWindow::OnOpenScriptClick);
	openButton->SetTag(-1);

	MQButton *execButton = CreateButton(locationFrame, L"Run");
	execButton->AddClickEvent(this, &JSMacroWindow::OnExecuteClick);
	execButton->SetTag(-1);

	// console log
	m_MessageList = CreateListBox(mainFrame);
	m_MessageList->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetVertLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetLineHeightRate(0.9);
	m_MessageList->AddDrawItemEvent(this, &JSMacroWindow::onDrawListItem);

	MQButton *clearButton = CreateButton(mainFrame, L"Clear");
	clearButton->AddClickEvent(this, &JSMacroWindow::OnClearClick);
	this->AddHideEvent(this, &JSMacroWindow::OnHide);


	MQFrame *presetFrame = CreateVerticalFrame(tab);
	tab->SetTabTitle(1, L"Preset");
	for (int i = 0; i < PRESET_SCRIPT_COUNT; i++) {
		MQFrame *locationFrame = CreateHorizontalFrame(presetFrame);

		m_PresetEdit[i] = CreateEdit(locationFrame);
		m_PresetEdit[i]->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);

		MQButton *openButton = CreateButton(locationFrame, L"...");
		openButton->AddClickEvent(this, &JSMacroWindow::OnOpenScriptClick);
		openButton->SetTag(i);

		MQButton *execButton = CreateButton(locationFrame, L"Run");
		execButton->AddClickEvent(this, &JSMacroWindow::OnExecuteClick);
		execButton->SetTag(i);
		std::stringstream ss;
		ss << "EXEC_P" << i;
		RegisterSubCommandButton((MQStationPlugin*)(GetPluginClass()), execButton, ss.str().c_str());
	}
	MQButton *saveButton = CreateButton(presetFrame, L"Save");
	saveButton->AddClickEvent(this, &JSMacroWindow::OnSavePresetClick);

	// Load settings
	std::wstring scriptPath;
	MQSetting *setting = GetPluginClass()->OpenSetting();
	setting->Load("lastScriptPath", scriptPath);
	m_FilePathEdit->SetText(scriptPath);
	for (int i = 0; i < PRESET_SCRIPT_COUNT; i++) {
		scriptPath.clear();
		auto path = m_PresetEdit[i]->GetText();
		std::stringstream ss;
		ss << "presetScriptPath_" << i;
		setting->Load(ss.str().c_str(), scriptPath);
		m_PresetEdit[i]->SetText(scriptPath);
	}
	GetPluginClass()->CloseSetting(setting);

}

void JSMacroWindow::AddMessage(const std::string &message, int tag) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	int index = m_MessageList->AddItem(converter.from_bytes(message));
	m_MessageList->SetItemTag(index, tag);
	m_MessageList->MakeItemVisible(index);
}

BOOL JSMacroWindow::OnClearClick(MQWidgetBase *sender, MQDocument doc) {
	m_MessageList->ClearItems();
	return FALSE;
}

BOOL JSMacroWindow::OnOpenScriptClick(MQWidgetBase *sender, MQDocument doc) {
	auto dialog = new MQOpenFileDialog(*this);
	dialog->SetFileMustExist(true);
	dialog->AddFilter(L"JavaScript(*.js)|*.js");
	if (dialog->Execute()) {
		if (sender->GetTag() >= 0) {
			m_PresetEdit[sender->GetTag()]->SetText(dialog->GetFileName());
		} else {
			m_FilePathEdit->SetText(dialog->GetFileName());
		}
	}
	return FALSE;
}

BOOL JSMacroWindow::Execute(MQDocument doc, int preset) {
	auto str = m_FilePathEdit->GetText();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string code = converter.to_bytes(str);
	if (preset >= 0 && preset < PRESET_SCRIPT_COUNT) {
		auto str = m_PresetEdit[preset]->GetText();
		m_FilePathEdit->SetText(str);
	}
	if (code.find("js:") == 0) {
		m_callback.ExecuteString(code.substr(3), doc);
	} else {
		m_callback.ExecuteFile(code, doc);
		MQSetting *setting = GetPluginClass()->OpenSetting();
		setting->Save("lastScriptPath", str);
		GetPluginClass()->CloseSetting(setting);
	}
	return TRUE;
}

BOOL JSMacroWindow::ExecuteProc(MQDocument doc, void *option) {
	JSMacroWindow *self = static_cast<JSMacroWindow*>(option);
	return self->Execute(doc);
}

BOOL JSMacroWindow::OnExecuteClick(MQWidgetBase *sender, MQDocument doc) {
	if (sender->GetTag() >= 0) {
		auto str = m_PresetEdit[sender->GetTag()]->GetText();
		m_FilePathEdit->SetText(str);
	}
	MQ_StationCallback(JSMacroWindow::ExecuteProc, this);
	return FALSE;
}

BOOL JSMacroWindow::OnSavePresetClick(MQWidgetBase * sender, MQDocument doc) {
	MQSetting *setting = GetPluginClass()->OpenSetting();
	for (int i = 0; i < PRESET_SCRIPT_COUNT; i++) {
		auto path = m_PresetEdit[i]->GetText();
		std::stringstream ss;
		ss << "presetScriptPath_"  << i;
		setting->Save(ss.str().c_str(), path);
	}
	GetPluginClass()->CloseSetting(setting);
	return FALSE;
}

BOOL JSMacroWindow::OnHide(MQWidgetBase *sender, MQDocument doc) {
	m_callback.OnCloseWindow(doc);
	return FALSE;
}

BOOL JSMacroWindow::onDrawListItem(MQWidgetBase* sender, MQDocument doc, MQListBoxDrawItemParam& param) {
	auto tag = m_MessageList->GetItemTag(param.ItemIndex);
	if (tag != 0) {
		if (tag == 2) {
			// Error
			param.Canvas->SetColor(255, 128, 128, 64);
		} else {
			// Normal
			param.Canvas->SetColor(128, 255, 128, 64);
		}
		param.Canvas->FillRect(param.X, param.Y, param.Width, param.Height);
	}
	// param.Canvas->DrawText(m_MessageList->GetItem(param.ItemIndex).c_str(), param.X + 10, param.Y, param.Width, param.Height, false);
	return FALSE;
}
