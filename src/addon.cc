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

std::wstring RemoveInvisibleChars(const std::wstring &input) {
  std::wstring output;
  output.reserve(input.size());
  for (wchar_t c : input) {
    if (c == static_cast<wchar_t>(0x200B)
        || c == static_cast<wchar_t>(0x200C)
        || c == static_cast<wchar_t>(0x200D)
        || c == static_cast<wchar_t>(0xFEFF)) {
      continue;
    }
    output.push_back(c);
  }
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

std::wstring StripSurroundingPunctuation(const std::wstring &input) {
  std::wstring value = Trim(input);
  while (!value.empty()) {
    wchar_t front = value.front();
    wchar_t back = value.back();
    bool trimFront = front == L'[' || front == L'(' || front == L'{' || front == L'<';
    bool trimBack = back == L']' || back == L')' || back == L'}' || back == L'>';
    if (!trimFront && !trimBack) {
      break;
    }
    if (trimFront) {
      value.erase(value.begin());
    }
    if (trimBack && !value.empty()) {
      value.pop_back();
    }
    value = Trim(value);
  }
  return value;
}

std::wstring NormalizeToken(const std::wstring &input) {
  return ToLower(StripSurroundingPunctuation(RemoveInvisibleChars(input)));
}

bool StartsWith(const std::wstring &value, const std::wstring &prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  return std::equal(prefix.begin(), prefix.end(), value.begin());
}

bool EndsWith(const std::wstring &value, const std::wstring &suffix) {
  if (value.size() < suffix.size()) {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

bool ContainsDigit(const std::wstring &value) {
  for (wchar_t c : value) {
    if (iswdigit(c)) {
      return true;
    }
  }
  return false;
}

size_t SkipWhitespace(const std::wstring &value, size_t pos) {
  while (pos < value.size() && iswspace(value[pos])) {
    ++pos;
  }
  return pos;
}

bool MatchWordInsensitive(const std::wstring &value,
                          size_t pos,
                          const std::wstring &word) {
  if (pos + word.size() > value.size()) {
    return false;
  }
  for (size_t i = 0; i < word.size(); ++i) {
    if (towlower(value[pos + i]) != word[i]) {
      return false;
    }
  }
  return true;
}

bool IsDashSeparatorAt(const std::wstring &value, size_t pos) {
  if (pos + 2 >= value.size()) {
    return false;
  }
  if (value[pos] != L' ' || value[pos + 2] != L' ') {
    return false;
  }
  wchar_t mid = value[pos + 1];
  return mid == L'-' || mid == static_cast<wchar_t>(0x2013)
         || mid == static_cast<wchar_t>(0x2014);
}

bool FindLastTitleSeparator(const std::wstring &value, size_t *posOut) {
  if (!posOut || value.size() < 3) {
    return false;
  }
  for (size_t i = value.size() - 3; i < value.size(); --i) {
    if (IsDashSeparatorAt(value, i)) {
      *posOut = i;
      return true;
    }
    if (i == 0) {
      break;
    }
  }
  return false;
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

std::wstring NormalizeExecutableName(const std::wstring &name) {
  std::wstring lower = NormalizeToken(name);
  if (EndsWith(lower, L".exe")) {
    lower = lower.substr(0, lower.size() - 4);
  }
  return lower;
}

bool IsProfileSegment(const std::wstring &lower) {
  if (lower == L"profile") {
    return true;
  }
  if (StartsWith(lower, L"profile ")) {
    return true;
  }
  if (StartsWith(lower, L"person ")) {
    return true;
  }
  if (lower == L"personal") {
    return true;
  }
  if (lower == L"default") {
    return true;
  }
  if (lower == L"in private") {
    return true;
  }
  if (lower == L"guest" || lower == L"incognito" || lower == L"inprivate") {
    return true;
  }
  return false;
}

bool IsMultiTabSummarySegment(const std::wstring &lower) {
  if (!ContainsDigit(lower)) {
    return false;
  }
  if (lower.find(L"more page") != std::wstring::npos) {
    return true;
  }
  if (lower.find(L"more tab") != std::wstring::npos) {
    return true;
  }
  return false;
}

std::wstring RemoveMultiTabSummaryFromTitle(const std::wstring &value) {
  for (size_t i = 0; i + 3 < value.size(); ++i) {
    if (!MatchWordInsensitive(value, i, L"and")) {
      continue;
    }
    if (i > 0 && !iswspace(value[i - 1])) {
      continue;
    }
    size_t j = i + 3;
    if (j >= value.size() || !iswspace(value[j])) {
      continue;
    }
    j = SkipWhitespace(value, j);
    if (j >= value.size() || !iswdigit(value[j])) {
      continue;
    }
    while (j < value.size() && iswdigit(value[j])) {
      ++j;
    }
    j = SkipWhitespace(value, j);
    if (!MatchWordInsensitive(value, j, L"more")) {
      continue;
    }
    j += 4;
    j = SkipWhitespace(value, j);
    bool matched = false;
    if (MatchWordInsensitive(value, j, L"page")) {
      j += 4;
      matched = true;
    } else if (MatchWordInsensitive(value, j, L"tab")) {
      j += 3;
      matched = true;
    }
    if (!matched) {
      continue;
    }
    if (j < value.size() && (value[j] == L's' || value[j] == L'S')) {
      ++j;
    }
    return Trim(value.substr(0, i));
  }
  return value;
}

bool IsRemovableTitleSuffix(const std::wstring &segment,
                            const std::wstring &appName,
                            const std::wstring &exeName,
                            bool isBrowser) {
  std::wstring lower = NormalizeToken(segment);
  if (lower.empty()) {
    return false;
  }

  std::wstring appLower = NormalizeToken(appName);
  if (!appLower.empty() && lower == appLower) {
    return true;
  }

  std::wstring exeLower = NormalizeExecutableName(exeName);
  if (!exeLower.empty() && lower == exeLower) {
    return true;
  }

  if (!isBrowser) {
    return false;
  }

  if (IsProfileSegment(lower) || IsMultiTabSummarySegment(lower)) {
    return true;
  }

  if (lower == L"microsoft edge" || lower == L"google chrome"
      || lower == L"brave" || lower == L"brave browser"
      || lower == L"mozilla firefox" || lower == L"firefox") {
    return true;
  }

  return false;
}

std::wstring NormalizeWindowTitle(const std::wstring &title,
                                  const std::wstring &appName,
                                  const std::wstring &exeName) {
  std::wstring value = Trim(title);
  if (value.empty()) {
    return value;
  }

  bool isBrowser = IsSupportedBrowser(exeName);
  bool changed = true;
  while (changed) {
    changed = false;
    size_t pos = 0;
    if (!FindLastTitleSeparator(value, &pos)) {
      break;
    }
    std::wstring suffix = value.substr(pos + 3);
    if (!IsRemovableTitleSuffix(suffix, appName, exeName, isBrowser)) {
      break;
    }
    std::wstring prefix = Trim(value.substr(0, pos));
    if (prefix.empty()) {
      break;
    }
    value = prefix;
    changed = true;
  }

  if (isBrowser) {
    value = RemoveMultiTabSummaryFromTitle(value);
  }

  return value;
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
  std::wstring titleValue = NormalizeWindowTitle(title, appNameValue, name);
  if (!appNameValue.empty()) {
    result.Set("appName", Napi::String::New(env, WideToUtf8(appNameValue)));
  }
  if (!titleValue.empty()) {
    result.Set("title", Napi::String::New(env, WideToUtf8(titleValue)));
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
