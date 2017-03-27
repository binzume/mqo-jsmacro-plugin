
#include <codecvt>
#include "JSMacroWindow.h"

JSMacroWindow::JSMacroWindow(MQWindowBase& parent, WindowCallback &callback) : MQWindow(parent), m_callback(callback) {
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "ja_JP.UTF-8");

	SetTitle(L"JSMacro");
	SetOutSpace(0.4);

	MQFrame *mainFrame = CreateVerticalFrame(this);
	mainFrame->SetVertLayout(MQWidgetBase::LAYOUT_FILL);

	// location
	MQFrame *locationFrame = CreateHorizontalFrame(mainFrame);

	m_FilePathEdit = CreateEdit(locationFrame);
	m_FilePathEdit->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_FilePathEdit->SetText(L"C:/tmp/test.js");

	MQButton *execButton = CreateButton(locationFrame, L"Run");
	execButton->AddClickEvent(this, &JSMacroWindow::OnExecuteClick);

	// console log
	m_MessageList = CreateListBox(mainFrame);
	m_MessageList->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetHintSizeRateX(20.0);

	MQButton *clearButton = CreateButton(mainFrame, L"Clear");
	clearButton->AddClickEvent(this, &JSMacroWindow::OnClearClick);
	this->AddHideEvent(this, &JSMacroWindow::OnHide);
}

void JSMacroWindow::AddMessage(const char *message) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	int index = m_MessageList->AddItem(converter.from_bytes(message));
	m_MessageList->MakeItemVisible(index);
}

BOOL JSMacroWindow::OnClearClick(MQWidgetBase *sender, MQDocument doc) {
	m_MessageList->ClearItems();
	return FALSE;
}

BOOL JSMacroWindow::OnExecuteClick(MQWidgetBase *sender, MQDocument doc) {
	auto str = m_FilePathEdit->GetText();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	m_callback.ExecuteFile(converter.to_bytes(str), doc);
	return FALSE;
}


BOOL JSMacroWindow::OnHide(MQWidgetBase *sender, MQDocument doc) {
	m_callback.OnCloseWindow(doc);
	return FALSE;
}
