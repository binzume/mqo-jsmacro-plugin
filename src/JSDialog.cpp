
#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <sstream>
#include <codecvt>
#include <windows.h>

#include "MQWidget.h"
#include "Utils.h"
#include "qjsutils.h"


//---------------------------------------------------------------------------------------------------------------------
// Dialog
//---------------------------------------------------------------------------------------------------------------------

JSValue NewFile(JSContext* context, const std::string& path, bool writable);

class Dialog : public MQDialog {
public:
  uint64_t button;
  std::vector<std::pair<ValueHolder, MQEdit*>> itemholders;
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  JSContext* context;
  MQWindow mainWindow;

  Dialog(JSContext* ctx) : mainWindow(MQWindow::GetMainWindow()), MQDialog(MQWindow::GetMainWindow()), button(-1), context(ctx) {
  }

  void AddItems(ValueHolder& items, MQWidgetBase* parent) {
    for (uint32_t i = 0; i < items.Length(); i++) {
      ValueHolder item = items[i];
      if (item.IsString()) {
        std::wstring label = converter.from_bytes(item.to<std::string>());
        auto* w = this->CreateLabel(this, label);
        parent->AddChild(w);
      }
      else if (item.IsObject()) {
        std::string type = item["type"].to<std::string>();
        ValueHolder value = item["value"];
        // Local<String> id = item.As<Object>()->Get(UTF8("id")).As<String>();
        if (std::string(type) == "text") {
          std::wstring v = converter.from_bytes(value.to<std::string>());
          auto* w = this->CreateEdit(this, v);
          parent->AddChild(w);
          itemholders.push_back(std::make_pair(item, w));
        }
        else if (std::string(type) == "number") {
          std::wstring v = converter.from_bytes(value.to<std::string>());
          auto* w = this->CreateEdit(this, v);
          w->SetNumeric(MQEdit::NUMERIC_DOUBLE);
          parent->AddChild(w);
          itemholders.push_back(std::make_pair(item, w));
        }
        else if (std::string(type) == "button") {
          std::wstring v = converter.from_bytes(value.to<std::string>());
          auto* w = this->CreateButton(this, v);
          parent->AddChild(w);
        }
        else if (std::string(type) == "hframe") {
          auto* w = this->CreateHorizontalFrame(this);
          parent->AddChild(w);
          ValueHolder items = item["items"];
          AddItems(items, w);
        }
        else if (std::string(type) == "vframe") {
          auto* w = this->CreateVerticalFrame(this);
          parent->AddChild(w);
          ValueHolder items = item["items"];
          AddItems(items, w);
        }
      }
    }
  }

  BOOL OnClick(MQWidgetBase* sender, MQDocument doc) {
    button = sender->GetTag();
    Close(MQDialog::DIALOG_OK);
    return TRUE;
  }
};

static JSValue ModalDialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if (argc < 2) {
    return JS_EXCEPTION;
  }

  auto dialog = new Dialog(ctx);
  ValueHolder params(ctx, argv[0], true);
  ValueHolder buttons(ctx, argv[1], true);

  std::string title = params["title"].to<std::string>();
  ValueHolder items = params["items"];

  dialog->SetTitle(dialog->converter.from_bytes(title));
  dialog->AddItems(items, dialog);


  int buttonCount = buttons.Length();
  if (buttonCount > 0) {
    auto* frame = dialog->CreateHorizontalFrame(dialog);
    dialog->AddChild(frame);
    for (int i = 0; i < buttonCount; i++) {
      std::string name = buttons[i].to<std::string>();
      MQButton* b = dialog->CreateButton(frame, dialog->converter.from_bytes(name));
      b->AddClickEvent(dialog, &Dialog::OnClick);
      b->SetTag(i);
      frame->AddChild(b);
    }
  }

  JSValue ret = JS_UNDEFINED;
  if (dialog->Execute()) {
    ValueHolder result(ctx);
    ValueHolder values(ctx);
    for (const auto& item : dialog->itemholders) {
    std::string value = dialog->converter.to_bytes(item.second->GetText()).c_str();
      ValueHolder h = item.first;
      h.Set("value", value.c_str());
      if (h["id"].IsString()) {
        values.Set(h["id"].to < std::string >() , value);
      }
    }
    result.Set("values", values);
    result.Set("button", (int)dialog->button);
    ret = result.GetValue();
  }
  delete dialog;
  return ret;
}


static JSValue FileDialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if (argc < 1) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }
  ValueHolder path(ctx, argv[0], true);

  if (!path.IsString()) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }

  bool save = argc > 2 ? convert_jsvalue<int>(ctx, argv[1]) : false;

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::wstring filter = converter.from_bytes(path.to < std::string >() );
  JSValue ret = JS_UNDEFINED;

  if (save) {
    auto window = MQWindow::GetMainWindow();
    auto dialog = new MQSaveFileDialog(window);
    dialog->AddFilter(filter);
    if (dialog->Execute()) {
      std::wstring path = dialog->GetFileName();
      ret = NewFile(ctx, converter.to_bytes(path), save);
    }
  }
  else {
    auto window = MQWindow::GetMainWindow();
    auto dialog = new MQOpenFileDialog(window);
    dialog->AddFilter(filter);
    if (dialog->Execute()) {
      std::wstring path = dialog->GetFileName();
      ret = NewFile(ctx, converter.to_bytes(path), save);
    }
  }
  return ret;
}

static JSValue FolderDialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if (argc < 1) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }
  ValueHolder path(ctx, argv[0], true);

  if (!path.IsString()) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::wstring filter = converter.from_bytes(path.to<std::string>());

  auto window = MQWindow::GetMainWindow();
  auto dialog = new MQFolderDialog(window);
  // dialog->SetFileMustExist(true);
  JSValue ret = JS_UNDEFINED;
  if (dialog->Execute()) {
    std::wstring path = dialog->GetFolder();
    ret = JS_NewString(ctx, converter.to_bytes(path).c_str());
  }
  delete dialog;
  return ret;
}

static JSValue AlertDialog(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  if (argc < 1) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }
  ValueHolder message(ctx, argv[0], true);

  if (!message.IsString()) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::wstring wmessage = converter.from_bytes(message.to<std::string>());
  std::wstring wtitle = L"";
  MQWindow window = MQWindow::GetMainWindow();
  MQDialog::MessageInformationBox(window, wmessage, wtitle);
  return JS_UNDEFINED;
}

const JSCFunctionListEntry dialog_funcs[] = {
    function_entry("modalDialog", 2, ModalDialog),
    function_entry("fileDialog", 2, FileDialog),
    function_entry("folderDialog", 2, FolderDialog),
    function_entry("alertDialog", 2, AlertDialog),
};

static int DialogModuleInit(JSContext* ctx, JSModuleDef* m)
{
  return JS_SetModuleExportList(ctx, m, dialog_funcs, COUNTOF(dialog_funcs));
}

JSModuleDef* InitDialogModule(JSContext* ctx) {
  JSModuleDef* m;
  m = JS_NewCModule(ctx, "mqdialog", DialogModuleInit);
  if (!m) {
    return NULL;
  }
  JS_AddModuleExportList(ctx, m, dialog_funcs, COUNTOF(dialog_funcs));
  return m;
}
