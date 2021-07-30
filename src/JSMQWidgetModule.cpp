
#include <windows.h>

#include <sstream>
#include <vector>

#include "MQWidget.h"
#include "Utils.h"
#include "qjsutils.h"

void dump_exception(JSContext* ctx, JSValue val);

//---------------------------------------------------------------------------------------------------------------------
// Dialog
//---------------------------------------------------------------------------------------------------------------------

JSValue NewFile(JSContext* context, const std::string& path, bool writable);

struct ItemBinder {
  enum class WidgetType { Edit, CheckBox, ComboBox, Button };
  ValueHolder item;
  WidgetType type;
  MQWidgetBase* widget;
  std::string id;

  ItemBinder(ValueHolder _item, WidgetType _type, MQWidgetBase* _widget)
      : item(_item), type(_type), widget(_widget) {
    if (item["id"].IsString()) {
      id = item["id"].To<std::string>();
    }
  }

  void set(JSContext* ctx, JSValueConst value) {
    if (type == WidgetType::Edit) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
      std::wstring v =
          converter.from_bytes(convert_jsvalue<std::string>(ctx, value));
      ((MQEdit*)widget)->SetText(v);
    } else if (type == WidgetType::CheckBox) {
      ((MQCheckBox*)widget)->SetChecked(convert_jsvalue<bool>(ctx, value));
    } else if (type == WidgetType::ComboBox) {
      ((MQComboBox*)widget)->SetCurrentIndex(convert_jsvalue<int>(ctx, value));
    }
    item.Set("value", JS_DupValue(ctx, value));
  }

  JSValue get(JSContext* ctx) {
    JSValue value = JS_UNDEFINED;
    if (type == WidgetType::Edit) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
      value = to_jsvalue(ctx, converter.to_bytes(((MQEdit*)widget)->GetText()));
    } else if (type == WidgetType::CheckBox) {
      value = to_jsvalue(ctx, ((MQCheckBox*)widget)->GetChecked());
    } else if (type == WidgetType::ComboBox) {
      value = to_jsvalue(ctx, ((MQComboBox*)widget)->GetCurrentIndex());
    }
    item.Set("value", JS_DupValue(ctx, value));
    return value;
  }

  template <typename T>
  void onchanged(T* p, BOOL (T::*f)(MQWidgetBase*, MQDocument doc),
                 bool prior = false) {
    if (type == WidgetType::Edit) {
      ((MQEdit*)widget)->AddChangedEvent(p, f, prior);
    } else if (type == WidgetType::CheckBox) {
      ((MQCheckBox*)widget)->AddChangedEvent(p, f, prior);
    } else if (type == WidgetType::ComboBox) {
      ((MQComboBox*)widget)->AddChangedEvent(p, f, prior);
    }
  }

  template <typename T>
  void onclick(T* p, BOOL (T::*f)(MQWidgetBase*, MQDocument doc),
               bool prior = false) {
    if (type == WidgetType::Button) {
      ((MQButton*)widget)->AddClickEvent(p, f, prior);
    }
  }

  void read(
      ValueHolder& values,
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>& converter) {
    if (type == WidgetType::Edit) {
      std::string value =
          converter.to_bytes(((MQEdit*)widget)->GetText()).c_str();
      item.Set("value", value);
      if (values.IsObject() && !id.empty()) {
        values.Set(id, value);
      }
    } else if (type == WidgetType::CheckBox) {
      bool value = ((MQCheckBox*)widget)->GetChecked();
      item.Set("value", value);
      if (values.IsObject() && !id.empty()) {
        values.Set(id, value);
      }
    } else if (type == WidgetType::ComboBox) {
      int value = ((MQComboBox*)widget)->GetCurrentIndex();
      item.Set("value", value);
      if (values.IsObject() && !id.empty()) {
        values.Set(id, value);
      }
    }
  }
};

class WidgetBuilder {
 public:
  std::vector<ItemBinder>& itemholders;
  MQWindowBase* window;
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  WidgetBuilder(MQWindowBase* _w, std::vector<ItemBinder>& _itemholders)
      : window(_w), itemholders(_itemholders) {}

  MQWidgetBase* build(ValueHolder& item, MQWidgetBase* parent) {
    if (item.IsString()) {
      std::wstring label = converter.from_bytes(item.To<std::string>());
      auto* w = window->CreateLabel(parent, label);
      return w;
    } else if (item.IsArray()) {
      auto* w = window->CreateVerticalFrame(parent);
      buildChildren(item, w);
      return w;
    } else if (item.IsObject()) {
      std::string type = item["type"].To<std::string>();
      ValueHolder value = item["value"];
      if (std::string(type) == "text") {
        std::wstring v = converter.from_bytes(value.To<std::string>());
        auto* w = window->CreateEdit(parent, v);
        itemholders.emplace_back(item, ItemBinder::WidgetType::Edit, w);
        return w;
      } else if (std::string(type) == "number") {
        std::wstring v = converter.from_bytes(value.To<std::string>());
        auto* w = window->CreateEdit(parent, v);
        w->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        itemholders.emplace_back(item, ItemBinder::WidgetType::Edit, w);
        return w;
      } else if (std::string(type) == "checkbox") {
        std::wstring v = converter.from_bytes(item["label"].To<std::string>());
        auto* w = window->CreateCheckBox(parent, v);
        w->SetChecked(value.To<bool>());
        itemholders.emplace_back(item, ItemBinder::WidgetType::CheckBox, w);
        return w;
      } else if (std::string(type) == "combobox") {
        auto* w = window->CreateComboBox(parent);
        for (uint32_t j = 0; j < item["items"].Length(); j++) {
          std::wstring s =
              converter.from_bytes(item["items"][j].To<std::string>());
          w->AddItem(s);
        }
        w->SetCurrentIndex(value.To<int>());
        itemholders.emplace_back(item, ItemBinder::WidgetType::ComboBox, w);
        return w;
      } else if (std::string(type) == "button") {
        std::wstring v = converter.from_bytes(value.To<std::string>());
        auto* w = window->CreateButton(parent, v);
        itemholders.emplace_back(item, ItemBinder::WidgetType::Button, w);
        return w;
      } else if (std::string(type) == "hframe") {
        auto* w = window->CreateHorizontalFrame(parent);
        buildChildren(item["items"], w);
        return w;
      } else if (std::string(type) == "vframe") {
        auto* w = window->CreateVerticalFrame(parent);
        buildChildren(item["items"], w);
        return w;
      } else if (std::string(type) == "group") {
        std::wstring v = converter.from_bytes(item["title"].To<std::string>());
        auto* w = window->CreateGroupBox(parent, v);
        buildChildren(item["items"], w);
        return w;
      }
      return nullptr;
    }
    return nullptr;
  }

  void buildChildren(ValueHolder items, MQWidgetBase* parent) {
    for (uint32_t i = 0; i < items.Length(); i++) {
      ValueHolder item = items[i];
      auto w = build(item, parent);
      if (w != nullptr) {
        if (item["hfill"].To<bool>()) {
          w->SetHorzLayout(MQWidgetBase::LAYOUT_FILL);
        }
        if (item["vfill"].To<bool>()) {
          w->SetVertLayout(MQWidgetBase::LAYOUT_FILL);
        }
      }
    }
  }
};

class JSFormWindow : public MQWindow, public JSClassBase<JSFormWindow> {
  std::vector<ItemBinder> binders;
  MQWidgetBase* rootFrame;

 public:
  static const JSCFunctionListEntry proto_funcs[];

  JSFormWindow(MQWindowBase parent) : MQWindow(parent), rootFrame(nullptr) {}
  JSFormWindow(MQWindowBase parent, JSContext* ctx, JSValueConst formsSpec)
      : MQWindow(parent), rootFrame(nullptr) {
    SetContent(ctx, formsSpec);
  }

  void SetContent(JSContext* ctx, JSValueConst formsSpec) {
    Clear();
    ValueHolder params(ctx, formsSpec, true);
    WidgetBuilder builder(this, binders);
    SetTitle(builder.converter.from_bytes(params["title"].To<std::string>()));
    params["modal"].To<bool>() ? SetModal() : ReleaseModal();
    ValueHolder items = params["items"];
    rootFrame = builder.build(items, this);

    for (auto& b : binders) {
      if (b.type == ItemBinder::WidgetType::Button) {
        b.onclick(this, &JSFormWindow::OnClick);
      } else {
        b.onchanged(this, &JSFormWindow::OnChange);
      }
    }
    SetVisible(true);
  }

  int GetWidth() { return MQWindow::GetWidth(); }
  void SetWidth(int width) { MQWindow::SetWidth(width); }
  int GetHeight() { return MQWindow::GetHeight(); }
  void SetHeight(int height) { MQWindow::SetHeight(height); }

  void SetValue(JSContext* ctx, std::string id, JSValueConst value) {
    auto b = std::find_if(binders.begin(), binders.end(),
                          [&](ItemBinder& b) { return b.id == id; });
    if (b != binders.end()) {
      b->set(ctx, value);
    }
  }

  JSValue GetValue(JSContext* ctx, std::string id) {
    auto b = std::find_if(binders.begin(), binders.end(),
                          [&](ItemBinder& b) { return b.id == id; });
    return b != binders.end() ? b->get(ctx) : JS_UNDEFINED;
  }

  void Clear() {
    if (rootFrame) {
      DeleteWidget(rootFrame);
      rootFrame = nullptr;
    }
    binders.clear();
  }

  void ReadValues(JSContext* ctx, JSValueConst value) {
    ValueHolder values(ctx, value);
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    for (auto& binder : binders) {
      binder.read(values, converter);
    }
  }
  void Close() {
    SetVisible(false);
    Clear();
  }

  BOOL OnClick(MQWidgetBase* sender, MQDocument doc) {
    auto b = std::find_if(binders.begin(), binders.end(),
                          [&](ItemBinder& b) { return b.widget == sender; });
    if (b == binders.end()) {
      return FALSE;
    }
    if (b->item["onclick"].IsFunction()) {
      JSContext* ctx = b->item.ctx;
      JSValue item = b->item.GetValue();
      auto fn = b->item["onclick"].GetValue();
      JSValue r = JS_Call(ctx, fn, item, 1, &item);
      if (JS_IsException(r)) {
        dump_exception(ctx, r);
      }
      JS_FreeValue(ctx, r);
      JS_FreeValue(ctx, fn);
      JS_FreeValue(ctx, item);
    }
    return TRUE;
  }

  BOOL OnChange(MQWidgetBase* sender, MQDocument doc) {
    auto b = std::find_if(binders.begin(), binders.end(),
                          [&](ItemBinder& b) { return b.widget == sender; });
    if (b == binders.end()) {
      return FALSE;
    }
    if (b->item["onchange"].IsFunction()) {
      JSContext* ctx = b->item.ctx;
      JSValue item = b->item.GetValue();
      JSValue value = b->get(ctx);
      auto fn = b->item["onchange"].GetValue();
      JSValue r = JS_Call(ctx, fn, item, 1, &item);
      if (JS_IsException(r)) {
        dump_exception(ctx, r);
      }
      JS_FreeValue(ctx, r);
      JS_FreeValue(ctx, fn);
      JS_FreeValue(ctx, item);
      JS_FreeValue(ctx, value);
    }
    return TRUE;
  }
};

const JSCFunctionListEntry JSFormWindow::proto_funcs[] = {
    function_entry<&Close>("close"),
    function_entry<&ReadValues>("readValues"),
    function_entry<&SetValue>("setValue"),
    function_entry<&GetValue>("getValue"),
    function_entry<&SetContent>("setContent"),
    function_entry_getset<&GetWidth, &SetWidth>("width"),
    function_entry_getset<&GetHeight, &SetHeight>("height"),
};

static JSValue CreateForm(JSContext* ctx, JSValueConst this_val, int argc,
                          JSValueConst* argv) {
  if (argc < 1) {
    return JS_EXCEPTION;
  }

  JSValue obj = JS_NewObjectClass(ctx, JSFormWindow::class_id);
  if (JS_IsException(obj)) return obj;
  JS_SetOpaque(obj, new JSFormWindow(MQWindow::GetMainWindow(), ctx, argv[0]));
  return obj;
}

class Dialog : public MQDialog {
  std::vector<ItemBinder> itemholders;

 public:
  uint64_t button;

  Dialog(JSContext* ctx, MQWindowBase parent) : MQDialog(parent), button(-1) {}

  void Build(ValueHolder& params, ValueHolder& buttons) {
    WidgetBuilder builder(this, itemholders);
    ValueHolder items = params["items"];
    std::string title = params["title"].To<std::string>();
    SetTitle(builder.converter.from_bytes(title));
    builder.buildChildren(items, this);

    int buttonCount = buttons.Length();
    if (buttonCount > 0) {
      auto* frame = CreateHorizontalFrame(this);
      for (int i = 0; i < buttonCount; i++) {
        std::string name = buttons[i].To<std::string>();
        MQButton* b = CreateButton(frame, builder.converter.from_bytes(name));
        b->AddClickEvent(this, &Dialog::OnClick);
        b->SetTag(i);
      }
    }
  }

  void ReadValues(ValueHolder& values) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    for (auto& binder : itemholders) {
      binder.read(values, converter);
    }
  }

  BOOL OnClick(MQWidgetBase* sender, MQDocument doc) {
    button = sender->GetTag();
    Close(MQDialog::DIALOG_OK);
    return TRUE;
  }
};

static JSValue ModalDialog(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv) {
  if (argc < 2) {
    return JS_EXCEPTION;
  }

  auto dialog = new Dialog(ctx, MQWindow::GetMainWindow());
  ValueHolder params(ctx, argv[0], true);
  ValueHolder buttons(ctx, argv[1], true);

  dialog->Build(params, buttons);

  JSValue ret = JS_UNDEFINED;
  if (dialog->Execute() == MQDialog::DIALOG_OK) {
    ValueHolder result(ctx);
    ValueHolder values(ctx);
    dialog->ReadValues(values);
    result.Set("values", values);
    result.Set("button", (int)dialog->button);
    ret = result.GetValue();
  }
  delete dialog;
  return ret;
}

static JSValue FileDialog(JSContext* ctx, JSValueConst this_val, int argc,
                          JSValueConst* argv) {
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
  std::wstring filter = converter.from_bytes(path.To<std::string>());
  JSValue ret = JS_UNDEFINED;

  if (save) {
    auto window = MQWindow::GetMainWindow();
    auto dialog = new MQSaveFileDialog(window);
    dialog->AddFilter(filter);
    if (dialog->Execute()) {
      std::wstring path = dialog->GetFileName();
      ret = NewFile(ctx, converter.to_bytes(path), save);
    }
  } else {
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

static JSValue FolderDialog(JSContext* ctx, JSValueConst this_val, int argc,
                            JSValueConst* argv) {
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
  std::wstring filter = converter.from_bytes(path.To<std::string>());

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

static JSValue AlertDialog(JSContext* ctx, JSValueConst this_val, int argc,
                           JSValueConst* argv) {
  ValueHolder message(ctx, argv[0], true);
  ValueHolder title(ctx, argv[1], true);

  if (!message.IsString()) {
    JS_ThrowTypeError(ctx, "invalid arguments");
    return JS_EXCEPTION;
  }

  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::wstring wmessage = converter.from_bytes(message.To<std::string>());
  std::wstring wtitle = converter.from_bytes(title.To<std::string>());
  MQWindow window = MQWindow::GetMainWindow();
  MQDialog::MessageInformationBox(window, wmessage, wtitle);
  return JS_UNDEFINED;
}

const JSCFunctionListEntry dialog_funcs[] = {
    function_entry("createForm", 1, CreateForm),
    function_entry("modalDialog", 2, ModalDialog),
    function_entry("fileDialog", 2, FileDialog),
    function_entry("folderDialog", 2, FolderDialog),
    function_entry("alertDialog", 2, AlertDialog),
};

static int DialogModuleInit(JSContext* ctx, JSModuleDef* m) {
  NewClassProto<JSFormWindow>(ctx, "FormWindow");

  return JS_SetModuleExportList(ctx, m, dialog_funcs,
                                (int)std::size(dialog_funcs));
}

JSModuleDef* InitMQWidgetModule(JSContext* ctx) {
  JSModuleDef* m = JS_NewCModule(ctx, "mqwidget", DialogModuleInit);
  if (!m) {
    return NULL;
  }
  JS_AddModuleExportList(ctx, m, dialog_funcs, (int)std::size(dialog_funcs));
  return m;
}
