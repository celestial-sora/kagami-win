#pragma once
#include "Framing.h"
#include "DialogStyle.h"
#include "PresetIdentity.h"
namespace framingControls {
inline kagami::Framing value;
inline HWND window{},fields[4]{},rotation{},fit{},aspect{},mirror{};
inline HFONT font{};
inline HWND presetName{};
inline std::wstring currentLabel=L"Default",dialogLabel=L"Default",androidSerial,lastAutoKey;
inline unsigned androidOrientation{};
struct Preset {DWORD version=1;double crop[4];DWORD rotation,mirror,fill;};
inline bool readFields(kagami::Framing& next) {
    double* numbers[]{&next.x,&next.y,&next.w,&next.h};
    for(int i=0;i<4;++i){wchar_t number[40];GetWindowTextW(fields[i],number,40);wchar_t* end{};*numbers[i]=wcstod(number,&end);if(end==number||*end)return false;}
    next.rotation=unsigned(SendMessageW(rotation,CB_GETCURSEL,0,0));next.fill=SendMessageW(fit,CB_GETCURSEL,0,0)==1;next.mirror=SendMessageW(mirror,BM_GETCHECK,0,0)==BST_CHECKED;
    return next.valid();
}
inline std::wstring presetKey(){wchar_t name[65]{};GetWindowTextW(presetName,name,65);std::wstring label=name;if(!kagami::presetLabelValid(label))return {};return L"Software\\Kagami\\Presets\\"+label;}
inline void listPresets(){
    SendMessageW(presetName,CB_RESETCONTENT,0,0);HKEY key{};
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\Presets",0,KEY_ENUMERATE_SUB_KEYS,&key)!=ERROR_SUCCESS)return;
    for(DWORD i=0;i<100;++i){wchar_t name[65]{};DWORD length=64;if(RegEnumKeyExW(key,i,name,&length,nullptr,nullptr,nullptr,nullptr)!=ERROR_SUCCESS)break;SendMessageW(presetName,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));}RegCloseKey(key);
}
inline void save() {
    HKEY key{};
    if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\Desktop",0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return;
    RegSetValueExW(key,L"PresetLabel",0,REG_SZ,reinterpret_cast<const BYTE*>(currentLabel.c_str()),DWORD((currentLabel.size()+1)*sizeof(wchar_t)));
    const double crop[]{value.x,value.y,value.w,value.h};
    RegSetValueExW(key,L"Crop",0,REG_BINARY,reinterpret_cast<const BYTE*>(crop),sizeof(crop));
    RegSetValueExW(key,L"OutputFormat",0,REG_BINARY,reinterpret_cast<const BYTE*>(&ui::format),sizeof(ui::format));
    const wchar_t* names[]{L"Rotation",L"Mirror",L"Fill",L"Guide",L"SafeArea",L"Source"};
    DWORD values[]{value.rotation,value.mirror,value.fill,ui::guide,ui::safe,DWORD(ui::selectedSource)};
    for(int i=0;i<6;++i)RegSetValueExW(key,names[i],0,REG_DWORD,reinterpret_cast<BYTE*>(&values[i]),sizeof(DWORD));
    RegCloseKey(key);
}
inline void load() {
    HKEY key{};if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\Desktop",0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return;
    wchar_t label[65]{};DWORD labelSize=sizeof(label);
    if(RegGetValueW(key,nullptr,L"PresetLabel",RRF_RT_REG_SZ,nullptr,label,&labelSize)==ERROR_SUCCESS&&kagami::presetLabelValid(label))currentLabel=label;
    double crop[4]{};DWORD size=sizeof(crop);
    kagami::OutputFormat output;DWORD outputSize=sizeof(output);
    if(RegGetValueW(key,nullptr,L"OutputFormat",RRF_RT_REG_BINARY,nullptr,&output,&outputSize)==ERROR_SUCCESS&&outputSize==sizeof(output)&&output.valid())ui::format=output;
    if(RegGetValueW(key,nullptr,L"Crop",RRF_RT_REG_BINARY,nullptr,crop,&size)==ERROR_SUCCESS&&size==sizeof(crop)) {value.x=crop[0];value.y=crop[1];value.w=crop[2];value.h=crop[3];}
    const wchar_t* names[]{L"Rotation",L"Mirror",L"Fill",L"Guide",L"SafeArea",L"Source"};DWORD values[6]{};
    for(int i=0;i<6;++i){size=sizeof(DWORD);RegGetValueW(key,nullptr,names[i],RRF_RT_REG_DWORD,nullptr,&values[i],&size);}
    value.rotation=values[0];value.mirror=values[1]!=0;value.fill=values[2]!=0;ui::guide=values[3]!=0;ui::safe=values[4]!=0;if(values[5]<2)ui::selectedSource=int(values[5]);
    RegCloseKey(key);if(!value.valid())value={};
}
inline void populate() {
    double values[]{value.x,value.y,value.w,value.h};
    for(int i=0;i<4;++i){wchar_t number[40];swprintf_s(number,L"%.8f",values[i]);SetWindowTextW(fields[i],number);}
    SendMessageW(rotation,CB_SETCURSEL,value.rotation,0);SendMessageW(fit,CB_SETCURSEL,value.fill?1:0,0);SendMessageW(mirror,BM_SETCHECK,value.mirror?BST_CHECKED:BST_UNCHECKED,0);
}
inline bool storePreset(const std::wstring& path,const Preset& stored) {
    if(path.empty())return false;HKEY key{};
    if(RegCreateKeyExW(HKEY_CURRENT_USER,path.c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return false;
    auto result=RegSetValueExW(key,L"Framing",0,REG_BINARY,reinterpret_cast<const BYTE*>(&stored),sizeof(stored));RegCloseKey(key);return result==ERROR_SUCCESS;
}
inline bool readPreset(const std::wstring& path,kagami::Framing& next) {
    if(path.empty())return false;Preset stored{};DWORD size=sizeof(stored);
    if(RegGetValueW(HKEY_CURRENT_USER,path.c_str(),L"Framing",RRF_RT_REG_BINARY,nullptr,&stored,&size)!=ERROR_SUCCESS||size!=sizeof(stored)||stored.version!=1)return false;
    next={stored.crop[0],stored.crop[1],stored.crop[2],stored.crop[3],stored.rotation,stored.mirror!=0,stored.fill!=0};return next.valid();
}
inline void clearAndroidContext(){androidSerial.clear();lastAutoKey.clear();androidOrientation=0;}
inline void selectAndroid(const std::wstring& serial){clearAndroidContext();if(kagami::stableUsbIdentity(serial))androidSerial=serial;}
inline bool matchAndroid(unsigned orientation) {
    androidOrientation=orientation;auto key=kagami::androidPresetKey(androidSerial,currentLabel,orientation);
    if(key.empty()||key==lastAutoKey)return false;lastAutoKey=key;
    kagami::Framing next;if(!readPreset(key,next))return false;
    value=next;save();if(window)populate();return true;
}
inline LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    LRESULT styled{};if(dialogStyle::message(msg,w,l,styled))return styled;
    switch(msg) {
    case WM_COMMAND:
        if(LOWORD(w)==IDOK)w=MAKEWPARAM(200,HIWORD(w));else if(LOWORD(w)==IDCANCEL)w=MAKEWPARAM(202,HIWORD(w));
        if(LOWORD(w)>=204&&LOWORD(w)<=206) {
            auto keyName=presetKey();if(keyName.empty()){MessageBoxW(hwnd,L"Enter a preset label, such as Phone · Portrait, with no slashes.",L"Preset",MB_ICONINFORMATION);return 0;}
            if(LOWORD(w)==204){
                kagami::Framing next;if(!readFields(next)){MessageBoxW(hwnd,L"Enter valid framing values before saving a preset.",L"Preset",MB_ICONWARNING);return 0;}
                Preset stored{1,{next.x,next.y,next.w,next.h},next.rotation,DWORD(next.mirror),DWORD(next.fill)};
                if(!storePreset(keyName,stored)){MessageBoxW(hwnd,L"The preset could not be saved. Your current framing was kept.",L"Preset",MB_ICONWARNING);return 0;}
                currentLabel=keyName.substr(keyName.find_last_of(L'\\')+1);dialogLabel=currentLabel;save();
                auto automatic=kagami::androidPresetKey(androidSerial,currentLabel,androidOrientation);
                if(!automatic.empty()&&!storePreset(automatic,stored))MessageBoxW(hwnd,L"The manual preset was saved, but its USB device/orientation binding could not be saved.",L"Preset",MB_ICONWARNING);
                lastAutoKey=automatic;listPresets();SetWindowTextW(presetName,currentLabel.c_str());
            }else if(LOWORD(w)==205){
                kagami::Framing next;
                if(readPreset(keyName,next)){auto original=value;value=next;populate();value=original;dialogLabel=keyName.substr(keyName.find_last_of(L'\\')+1);}
                else MessageBoxW(hwnd,L"The preset is missing or invalid. Your current framing was kept.",L"Preset",MB_ICONWARNING);
            }else {
                auto label=keyName.substr(keyName.find_last_of(L'\\')+1);
                RegDeleteKeyW(HKEY_CURRENT_USER,keyName.c_str());
                // Delete bindings for this label across all explicitly saved USB devices.
                HKEY devices{};if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Kagami\\AndroidPresets",0,KEY_ENUMERATE_SUB_KEYS,&devices)==ERROR_SUCCESS){
                    for(DWORD i=0;i<1000;++i){wchar_t serial[181]{};DWORD length=180;if(RegEnumKeyExW(devices,i,serial,&length,nullptr,nullptr,nullptr,nullptr)!=ERROR_SUCCESS)break;
                        for(unsigned orientation:{1u,2u}){auto path=kagami::androidPresetKey(serial,label,orientation);if(!path.empty())RegDeleteKeyW(HKEY_CURRENT_USER,path.c_str());}
                    }RegCloseKey(devices);
                }
                lastAutoKey.clear();listPresets();
            }
            return 0;
        }
        if(LOWORD(w)==200) {
            kagami::Framing next=value;
            if(!readFields(next)){MessageBoxW(hwnd,L"Crop coordinates must remain inside 0–1, with positive width and height.",L"Framing",MB_ICONWARNING);return 0;}
            wchar_t label[65]{};GetWindowTextW(presetName,label,65);
            if(*label&&!kagami::presetLabelValid(label)){MessageBoxW(hwnd,L"Use a preset label with no slashes or control characters.",L"Preset",MB_ICONWARNING);return 0;}
            value=next;currentLabel=*label?std::wstring(label):dialogLabel;lastAutoKey=kagami::androidPresetKey(androidSerial,currentLabel,androidOrientation);save();DestroyWindow(hwnd);return 0;
        }
        if(LOWORD(w)==201) {auto original=value;value={};populate();value=original;return 0;}
        if(LOWORD(w)==202) {DestroyWindow(hwnd);return 0;}
        if(LOWORD(w)==203&&HIWORD(w)==CBN_SELCHANGE) {
            int selected=int(SendMessageW(aspect,CB_GETCURSEL,0,0));
            double ratios[]{0,16.0/9,4.0/3,1,9.0/16};
            if(selected>0&&selected<5){kagami::Framing preset=value;kagami::centeredAspect(preset,ratios[selected]);double numbers[]{preset.x,preset.y,preset.w,preset.h};for(int i=0;i<4;++i){wchar_t number[40];swprintf_s(number,L"%.8f",numbers[i]);SetWindowTextW(fields[i],number);}}return 0;
        }
        break;
    case WM_CLOSE: DestroyWindow(hwnd);return 0;
    case WM_DESTROY: window=nullptr; if(font){DeleteObject(font);font=nullptr;}return 0;
    }
    return DefWindowProcW(hwnd,msg,w,l);
}
inline void show(HWND parent) {
    if(window){SetForegroundWindow(window);return;}dialogLabel=currentLabel;
    WNDCLASSW cls{};cls.lpfnWndProc=proc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"Kagami.Framing";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=dialogStyle::background();RegisterClassW(&cls);
    window=CreateWindowW(cls.lpszClassName,L"Kagami · Framing controls",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,520,450,parent,nullptr,cls.hInstance,nullptr);
    if(!window)return;dialogStyle::title(window);
    float dpi=GetDpiForWindow(window)/96.f;
    font=CreateFontW(int(-15*dpi),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    auto control=[&](const wchar_t* kind,const wchar_t* label,DWORD style,int x,int y,int width,int height,int id=0){HWND child=CreateWindowW(kind,label,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(width*dpi),int(height*dpi),window,reinterpret_cast<HMENU>(INT_PTR(id)),cls.hInstance,nullptr);SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);if(wcscmp(kind,L"BUTTON")==0)dialogStyle::button(child);return child;};
    control(L"STATIC",L"Crop the source, then rotate / mirror and fit the camera.",0,20,16,460,44);
    const wchar_t* labels[]{L"Left (0–1)",L"Top (0–1)",L"Width (0–1)",L"Height (0–1)"};
    for(int i=0;i<4;++i){int x=20+(i%2)*235,y=64+(i/2)*42;control(L"STATIC",labels[i],0,x,y,115,26);fields[i]=control(L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,x+119,y,95,28);}
    control(L"STATIC",L"Aspect crop",0,20,158,115,26);aspect=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,155,320,180,203);
    const wchar_t* aspects[]{L"Free crop",L"16:9",L"4:3",L"1:1",L"Portrait 9:16"};for(auto label:aspects)SendMessageW(aspect,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));SendMessageW(aspect,CB_SETCURSEL,0,0);
    control(L"STATIC",L"Rotation",0,20,203,115,26);rotation=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,200,320,160);
    const wchar_t* rotations[]{L"0°",L"90° clockwise",L"180°",L"270° clockwise"};for(auto label:rotations)SendMessageW(rotation,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label));
    control(L"STATIC",L"Scale",0,20,248,115,26);fit=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST,139,245,320,120);SendMessageW(fit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Fit (letterbox)"));SendMessageW(fit,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Fill (center crop)"));
    mirror=control(L"BUTTON",L"Mirror preview and output",WS_TABSTOP|BS_AUTOCHECKBOX,20,292,440,28);
    control(L"STATIC",L"App preset · USB device + orientation saved while receiving",0,20,334,440,26);
    presetName=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWN|CBS_AUTOHSCROLL,20,364,440,180);SendMessageW(presetName,CB_LIMITTEXT,64,0);listPresets();SetWindowTextW(presetName,currentLabel.c_str());
    control(L"BUTTON",L"Save preset",WS_TABSTOP,20,404,136,34,204);control(L"BUTTON",L"Load preset",WS_TABSTOP,170,404,136,34,205);control(L"BUTTON",L"Delete preset",WS_TABSTOP,320,404,140,34,206);
    control(L"BUTTON",L"Reset",WS_TABSTOP,20,460,110,36,201);control(L"BUTTON",L"Cancel",WS_TABSTOP,222,460,110,36,202);control(L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,344,460,115,36,200);
    SetWindowPos(window,nullptr,0,0,int(520*dpi),int(555*dpi),SWP_NOMOVE|SWP_NOZORDER);populate();ShowWindow(window,SW_SHOW);
}
}
