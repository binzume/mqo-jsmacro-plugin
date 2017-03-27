
#include "JSMacroWindow.h"


JSMacroWindow::JSMacroWindow(MQWindowBase& parent) : MQWindow(parent) {
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "ja_JP.UTF-8");

	SetTitle(L"JSMacro");
	SetOutSpace(0.4);

	MQFrame *mainFrame = CreateHorizontalFrame(this);
	mainFrame->SetVertLayout(MQWidgetBase::LAYOUT_FILL);

	m_MessageList = CreateListBox(mainFrame);
	m_MessageList->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetHintSizeRateX(20.0);

}

void JSMacroWindow::AddMessage(const char *message) {
	std::wstring wc(strlen(message), L'#');
	mbstowcs(&wc[0], message, strlen(message));
	int index = m_MessageList->AddItem(wc);
	m_MessageList->MakeItemVisible(index);
}
