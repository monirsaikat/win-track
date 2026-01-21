#include <napi.h>
#include <windows.h>
#include <psapi.h>
#include <UIAutomation.h>

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

std::string WideToUtf8(const std::wstring &input) {
  if (input.empty()) {
    return std::string();
  }
  int size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(),
                                 static_cast<int>(input.size()), nullptr, 0,
                                 nullptr, nullptr);
  if (size <= 0) {
    return std::string();
  }
  std::string output(size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.c_str(),
                      static_cast<int>(input.size()), &output[0], size,
                      nullptr, nullptr);
  return output;
}

template <typename T>
void SafeRelease(T **ptr) {
  if (ptr && *ptr) {
    (*ptr)->Release();
    *ptr = nullptr;
  }
}

std::wstring ReadWindowTitle(HWND hwnd) {
  int length = GetWindowTextLengthW(hwnd);
  if (length <= 0) {
    return std::wstring();
  }
  std::wstring title(length, L'\0');
  GetWindowTextW(hwnd, &title[0], length + 1);
  return title;
}

std::wstring ReadProcessPath(HANDLE process) {
  if (!process) {
    return std::wstring();
  }
  std::wstring path;
  DWORD size = MAX_PATH;
  path.resize(size);
  if (QueryFullProcessImageNameW(process, 0, &path[0], &size) == 0) {
    return std::wstring();
  }
  path.resize(size);
  return path;
}

std::wstring ExtractFileName(const std::wstring &path) {
  if (path.empty()) {
    return std::wstring();
  }
  size_t pos = path.find_last_of(L"\\/");
  if (pos == std::wstring::npos || pos + 1 >= path.size()) {
    return path;
  }
  return path.substr(pos + 1);
}

std::wstring ReadFileDescription(const std::wstring &path) {
  if (path.empty()) {
    return std::wstring();
  }
  DWORD handle = 0;
  DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
  if (size == 0) {
    return std::wstring();
  }
  std::vector<BYTE> data(size);
  if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) {
    return std::wstring();
  }

  struct LangAndCodePage {
    WORD language;
    WORD codePage;
  };
  LangAndCodePage *translation = nullptr;
  UINT translationSize = 0;
  if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation",
                      reinterpret_cast<void **>(&translation),
                      &translationSize)) {
    return std::wstring();
  }

  if (!translation || translationSize < sizeof(LangAndCodePage)) {
    return std::wstring();
  }

  wchar_t subBlock[64];
  swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\FileDescription",
             translation[0].language, translation[0].codePage);

  wchar_t *description = nullptr;
  UINT descriptionSize = 0;
  if (VerQueryValueW(data.data(), subBlock,
                     reinterpret_cast<void **>(&description),
                     &descriptionSize) && description && descriptionSize > 0) {
    return std::wstring(description, descriptionSize - 1);
  }

  return std::wstring();
}

std::wstring ToLower(const std::wstring &input) {
  std::wstring output = input;
  std::transform(output.begin(), output.end(), output.begin(),
                 [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
  return output;
}

std::wstring Trim(const std::wstring &input) {
  size_t start = 0;
  while (start < input.size() && iswspace(input[start])) {
    ++start;
  }
  size_t end = input.size();
  while (end > start && iswspace(input[end - 1])) {
    --end;
  }
  return input.substr(start, end - start);
}

bool StartsWith(const std::wstring &value, const std::wstring &prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  return std::equal(prefix.begin(), prefix.end(), value.begin());
}

bool IsLikelyUrl(const std::wstring &value) {
  std::wstring trimmed = Trim(value);
  if (trimmed.empty()) {
    return false;
  }
  std::wstring lower = ToLower(trimmed);
  if (lower.find(L"://") != std::wstring::npos) {
    return true;
  }
  if (StartsWith(lower, L"www.")) {
    return true;
  }
  if (StartsWith(lower, L"localhost")) {
    return true;
  }
  if (StartsWith(lower, L"about:") || StartsWith(lower, L"edge:")
      || StartsWith(lower, L"chrome:") || StartsWith(lower, L"brave:")
      || StartsWith(lower, L"file:") || StartsWith(lower, L"view-source:")
      || StartsWith(lower, L"moz-extension:")) {
    return true;
  }
  if (lower.find(L' ') == std::wstring::npos && lower.find(L'.') != std::wstring::npos) {
    return true;
  }
  return false;
}

bool IsSupportedBrowser(const std::wstring &exeName) {
  std::wstring lower = ToLower(exeName);
  return lower == L"chrome.exe" || lower == L"msedge.exe"
         || lower == L"firefox.exe" || lower == L"brave.exe";
}

bool IsTopArea(const RECT &rect, const RECT &windowRect) {
  double windowHeight =
      static_cast<double>(windowRect.bottom - windowRect.top);
  if (windowHeight <= 0.0) {
    return false;
  }
  double topThreshold = windowRect.top + windowHeight * 0.35;
  return rect.top >= windowRect.top && rect.top <= topThreshold;
}

std::wstring ReadBrowserUrl(HWND hwnd, const RECT &windowRect) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool didInit = (hr == S_OK || hr == S_FALSE);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
    return std::wstring();
  }

  IUIAutomation *automation = nullptr;
  hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&automation));
  if (FAILED(hr) || !automation) {
    if (didInit) {
      CoUninitialize();
    }
    return std::wstring();
  }

  IUIAutomationElement *root = nullptr;
  hr = automation->ElementFromHandle(hwnd, &root);
  if (FAILED(hr) || !root) {
    SafeRelease(&automation);
    if (didInit) {
      CoUninitialize();
    }
    return std::wstring();
  }

  VARIANT varProp;
  VariantInit(&varProp);
  varProp.vt = VT_I4;
  varProp.lVal = UIA_EditControlTypeId;
  IUIAutomationCondition *editCondition = nullptr;
  hr = automation->CreatePropertyCondition(UIA_ControlTypePropertyId, varProp,
                                           &editCondition);
  VariantClear(&varProp);

  VariantInit(&varProp);
  varProp.vt = VT_I4;
  varProp.lVal = UIA_ComboBoxControlTypeId;
  IUIAutomationCondition *comboCondition = nullptr;
  if (SUCCEEDED(hr)) {
    hr = automation->CreatePropertyCondition(UIA_ControlTypePropertyId, varProp,
                                             &comboCondition);
  }
  VariantClear(&varProp);

  IUIAutomationCondition *condition = nullptr;
  if (editCondition && comboCondition) {
    automation->CreateOrCondition(editCondition, comboCondition, &condition);
  } else if (editCondition) {
    condition = editCondition;
    editCondition = nullptr;
  } else if (comboCondition) {
    condition = comboCondition;
    comboCondition = nullptr;
  }

  IUIAutomationElementArray *elements = nullptr;
  if (SUCCEEDED(hr) && condition) {
    root->FindAll(TreeScope_Subtree, condition, &elements);
  }

  std::wstring fallback;
  if (elements) {
    int length = 0;
    elements->get_Length(&length);
    for (int i = 0; i < length; ++i) {
      IUIAutomationElement *element = nullptr;
      if (FAILED(elements->GetElement(i, &element)) || !element) {
        continue;
      }

      IUIAutomationValuePattern *valuePattern = nullptr;
      hr = element->GetCurrentPatternAs(UIA_ValuePatternId,
                                        IID_PPV_ARGS(&valuePattern));
      if (SUCCEEDED(hr) && valuePattern) {
        BSTR bstr = nullptr;
        if (SUCCEEDED(valuePattern->get_CurrentValue(&bstr)) && bstr) {
          std::wstring value(bstr, SysStringLen(bstr));
          SysFreeString(bstr);
          if (IsLikelyUrl(value)) {
            RECT rect{};
            if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))
                && IsTopArea(rect, windowRect)) {
              SafeRelease(&valuePattern);
              SafeRelease(&element);
              fallback = value;
              break;
            }
            if (fallback.empty()) {
              fallback = value;
            }
          }
        }
        SafeRelease(&valuePattern);
      } else {
        BSTR name = nullptr;
        if (SUCCEEDED(element->get_CurrentName(&name)) && name) {
          std::wstring value(name, SysStringLen(name));
          SysFreeString(name);
          if (IsLikelyUrl(value)) {
            RECT rect{};
            if (SUCCEEDED(element->get_CurrentBoundingRectangle(&rect))
                && IsTopArea(rect, windowRect)) {
              SafeRelease(&element);
              fallback = value;
              break;
            }
            if (fallback.empty()) {
              fallback = value;
            }
          }
        }
      }
      SafeRelease(&element);
    }
  }

  SafeRelease(&elements);
  SafeRelease(&condition);
  SafeRelease(&editCondition);
  SafeRelease(&comboCondition);
  SafeRelease(&root);
  SafeRelease(&automation);
  if (didInit) {
    CoUninitialize();
  }
  return fallback;
}

Napi::Value GetActiveWindowWrapped(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  HWND hwnd = GetForegroundWindow();
  if (!hwnd) {
    return env.Null();
  }

  RECT rect;
  if (!GetWindowRect(hwnd, &rect)) {
    return env.Null();
  }

  DWORD processId = 0;
  GetWindowThreadProcessId(hwnd, &processId);
  if (!processId) {
    return env.Null();
  }

  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                               FALSE, processId);

  std::wstring title = ReadWindowTitle(hwnd);
  std::wstring path = ReadProcessPath(process);
  std::wstring name = ExtractFileName(path);
  std::wstring appName = ReadFileDescription(path);
  std::wstring url;
  if (IsSupportedBrowser(name)) {
    url = ReadBrowserUrl(hwnd, rect);
  }

  PROCESS_MEMORY_COUNTERS counters;
  SIZE_T workingSet = 0;
  if (process && GetProcessMemoryInfo(process, &counters, sizeof(counters))) {
    workingSet = counters.WorkingSetSize;
  }

  if (process) {
    CloseHandle(process);
  }

  Napi::Object owner = Napi::Object::New(env);
  if (!name.empty()) {
    owner.Set("name", Napi::String::New(env, WideToUtf8(name)));
  }
  if (!path.empty()) {
    owner.Set("path", Napi::String::New(env, WideToUtf8(path)));
  }
  owner.Set("processId", Napi::Number::New(env, processId));

  Napi::Object bounds = Napi::Object::New(env);
  bounds.Set("x", Napi::Number::New(env, rect.left));
  bounds.Set("y", Napi::Number::New(env, rect.top));
  bounds.Set("width", Napi::Number::New(env, rect.right - rect.left));
  bounds.Set("height", Napi::Number::New(env, rect.bottom - rect.top));

  Napi::Object result = Napi::Object::New(env);
  std::wstring appNameValue = appName.empty() ? name : appName;
  if (!appNameValue.empty()) {
    result.Set("appName", Napi::String::New(env, WideToUtf8(appNameValue)));
  }
  if (!title.empty()) {
    result.Set("title", Napi::String::New(env, WideToUtf8(title)));
  }
  result.Set("id", Napi::String::New(env, std::to_string(
                       reinterpret_cast<uintptr_t>(hwnd))));
  result.Set("bounds", bounds);
  result.Set("owner", owner);
  if (workingSet > 0) {
    result.Set("memoryUsage",
               Napi::Number::New(env, static_cast<double>(workingSet)));
  }
  if (!url.empty()) {
    result.Set("url", Napi::String::New(env, WideToUtf8(url)));
  } else {
    result.Set("url", env.Undefined());
  }
  result.Set("website", env.Undefined());

  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getActiveWindow", Napi::Function::New(env, GetActiveWindowWrapped));
  return exports;
}

NODE_API_MODULE(win_trace, Init)
