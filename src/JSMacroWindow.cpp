
#include <codecvt>
#include "JSMacroWindow.h"

JSMacroWindow::JSMacroWindow(MQWindowBase& parent, WindowCallback &callback) : MQWindow(parent), m_callback(callback) {
	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "ja_JP.UTF-8");

	SetTitle(L"JSMacro");
	SetOutSpace(0.4);

	MQFrame *mainFrame = CreateVerticalFrame(this);
	mainFrame->SetVertLayout(MQWidgetBase::LAYOUT_FILL);
	mainFrame->SetHintSizeRateX(40.0);
	mainFrame->SetHintSizeRateY(30.0);

	// location
	MQFrame *locationFrame = CreateHorizontalFrame(mainFrame);

	m_FilePathEdit = CreateEdit(locationFrame);
	m_FilePathEdit->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_FilePathEdit->SetText(L"C:/tmp/test.js");

	MQButton *openButton = CreateButton(locationFrame, L"...");
	openButton->AddClickEvent(this, &JSMacroWindow::OnOpenScriptClick);

	MQButton *execButton = CreateButton(locationFrame, L"Run");
	execButton->AddClickEvent(this, &JSMacroWindow::OnExecuteClick);

	// console log
	m_MessageList = CreateListBox(mainFrame);
	m_MessageList->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetVertLayout(MQWidgetBase::LAYOUT_FILL);
	m_MessageList->SetLineHeightRate(0.8);
	m_MessageList->AddDrawItemEvent(this, &JSMacroWindow::onDrawListItem);

	MQButton *clearButton = CreateButton(mainFrame, L"Clear");
	clearButton->AddClickEvent(this, &JSMacroWindow::OnClearClick);
	this->AddHideEvent(this, &JSMacroWindow::OnHide);
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
		m_FilePathEdit->SetText(dialog->GetFileName());
	}
	return FALSE;
}

BOOL JSMacroWindow::Execute(MQDocument doc) {
	auto str = m_FilePathEdit->GetText();
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string code = converter.to_bytes(str);
	if (code.find("js:") == 0) {
		m_callback.ExecuteString(code.substr(3), doc);
	} else {
		m_callback.ExecuteFile(code, doc);
	}
	return TRUE;
}

BOOL JSMacroWindow::ExecuteProc(MQDocument doc, void *option) {
	JSMacroWindow *self = static_cast<JSMacroWindow*>(option);
	return self->Execute(doc);
}

BOOL JSMacroWindow::OnExecuteClick(MQWidgetBase *sender, MQDocument doc) {
	// Execute(doc);
	MQ_StationCallback(JSMacroWindow::ExecuteProc, this);
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
