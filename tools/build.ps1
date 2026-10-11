[CmdletBinding()]
param([switch]$Installer, [switch]$InstallBuildDependencies, [switch]$AirPlay)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $taskRoot
function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
if ($InstallBuildDependencies) {
    Invoke-Checked winget @('install','--id','Microsoft.VisualStudio.2022.BuildTools','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements','--override','--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')
    if ($AirPlay) { Invoke-Checked winget @('install','--id','MSYS2.MSYS2','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements') }
    if ($Installer) { Invoke-Checked winget @('install','--id','JRSoftware.InnoSetup','--exact','--source','winget','--silent','--accept-package-agreements','--accept-source-agreements') }
}
if ($AirPlay) {
    $msysBash = 'C:/msys64/usr/bin/bash.exe'
    if (!(Test-Path -LiteralPath $msysBash)) { throw 'Install MSYS2.MSYS2 with winget for the AirPlay build.' }
    $env:MSYSTEM = 'UCRT64'
    $env:CHERE_INVOKING = '1'
    Invoke-Checked $msysBash @('-lc', 'pacman -Syu --noconfirm')
    Invoke-Checked $msysBash @('-lc', 'pacman -S --needed --noconfirm git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-libplist mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-gstreamer mingw-w64-ucrt-x86_64-gst-plugins-base mingw-w64-ucrt-x86_64-gst-plugins-good mingw-w64-ucrt-x86_64-gst-plugins-bad mingw-w64-ucrt-x86_64-gst-libav')
    Invoke-Checked $msysBash @('-lc', 'mkdir -p build && bash tools/build-airplay.sh')
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install C++ Build Tools with -InstallBuildDependencies.' }
$vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsPath) { throw 'C++ toolchain not ready. Finish installing Build Tools, then retry.' }
Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
New-Item -ItemType Directory -Force '.local/deps','build','dist' | Out-Null
$packages = @(
    @{Name='cppwinrt'; Id='microsoft.windows.cppwinrt'; Version='3.0.260520.1'; Hash='D22E2E26133D63217AE26E91B1685FB024B03A508A78AF645F8347A3126C8435'},
    @{Name='wil'; Id='microsoft.windows.implementationlibrary'; Version='1.0.260126.7'; Hash='7D67BC71DD0EDC342DB44EA0334AC9B6A1776514F81694B0B5B3C7AC9D608E42'}
)
Write-Host '[1/5] Verify pinned native headers'
foreach ($package in $packages) {
    $archive = Join-Path $taskRoot ".local/deps/$($package.Name).zip"
    if (!(Test-Path -LiteralPath $archive)) {
        Invoke-WebRequest "https://api.nuget.org/v3-flatcontainer/$($package.Id)/$($package.Version)/$($package.Id).$($package.Version).nupkg" -OutFile $archive
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $package.Hash) { throw "Checksum mismatch for $($package.Name)" }
    Expand-Archive -LiteralPath $archive -DestinationPath ".local/deps/$($package.Name)" -Force
}
Invoke-Checked (Join-Path $taskRoot '.local/deps/cppwinrt/bin/cppwinrt.exe') @('-input','sdk','-output',"$taskRoot/.local/deps/cppwinrt/include")
$common = @('/nologo','/utf-8','/O2','/std:c++20','/EHsc','/MT','/W4','/DUNICODE','/D_UNICODE','/D_WIN32_WINNT=0x0A00',"/I$taskRoot/native", "/I$taskRoot/.local/deps/cppwinrt/include", "/I$taskRoot/.local/deps/wil/include", "/Fo$taskRoot\build\")
Write-Host '[2/5] Build Kagami Media Foundation camera'
$sources = (Get-ChildItem -LiteralPath native/source -Filter '*.cpp').FullName
Invoke-Checked cl ($common + @('/LD') + $sources + @('/link','/DEF:native/source/KagamiCamera.def','/OUT:dist/KagamiCamera.dll','advapi32.lib','shlwapi.lib','propsys.lib'))
Write-Host '[3/5] Build native desktop executable'
Push-Location native
try { Invoke-Checked rc @('/nologo','/fo','../build/app.res','app.rc') } finally { Pop-Location }
Invoke-Checked cl ($common + @('native/CameraHost.cpp','build/app.res','/Fe:dist/Kagami.exe','/link','/SUBSYSTEM:WINDOWS','mfplat.lib','mfuuid.lib','mfsensorgroup.lib','ole32.lib','oleaut32.lib','uuid.lib','advapi32.lib','user32.lib','gdi32.lib','shell32.lib'))
Write-Host '[4/5] Test framing and actual local frame bridge (no camera registration)'
Invoke-Checked cl ($common + @('tests/framing.cpp','/Fe:build/framing-tests.exe'))
Invoke-Checked (Join-Path $taskRoot 'build/framing-tests.exe') @()
Invoke-Checked cl ($common + @('tests/smartview_protocol.cpp','/Fe:build/smartview-protocol-tests.exe','/link','a…5443 tokens truncated…32,516,226,32);EnableWindow(buttons[12],!running);
    place(7,406,r.bottom/scale-36,width-424,28);
    EnableWindow(buttons[1],enabled && !running); EnableWindow(buttons[0],!running);EnableWindow(buttons[8],enabled && !running); EnableWindow(buttons[2],running);EnableWindow(buttons[10],enabled&&!running);
}
inline void paint(HWND hwnd, HDC dc) {
    RECT r; GetClientRect(hwnd,&r);
    Bitmap buffer(r.right,r.bottom,PixelFormat32bppPARGB); Graphics g(&buffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias); g.Clear(Color(255,25,24,34)); g.ScaleTransform(scale,scale);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    float width=r.right/scale, height=r.bottom/scale, right=width-406;
    SolidBrush top(Color(255,33,31,44)); g.FillRectangle(&top,0.f,0.f,width,96.f);
    HICON icon=LoadIconW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(1));
    // Draw the approved alpha icon with the Windows icon compositor after buffering.
    text(g,L"Kagami · 鏡",88,19,300,36,28,true);
    text(g,L"Phone screen → Virtual Camera",89,56,330,26,15,false,muted);
    rounded(g,RectF(18,114,358,190),Color(255,33,32,43),true);
    text(g,L"Connection Source",30,127,320,28,18,true);
    rounded(g,RectF(30,163,158,95),Color(255,82,54,94));
    rounded(g,RectF(200,163,158,95),Color(255,47,44,59));
    monitor(g,43,176,25); monitor(g,213,176,25);
    text(g,L"AirPlay",43,202,135,24,18,true); text(g,L"Smart View",213,202,135,24,18,true);
    text(g,L"Apple · Experimental",43,231,140,20,13,false,muted);
    rounded(g,RectF(18,316,358,91),Color(255,51,48,62),true);
    text(g,smartview?(source.empty()?L"●  Waiting for Smart View":L"●  Smart View receiving"):airplay?(source.empty()?L"●  Waiting for AirPlay":L"●  AirPlay receiving"):android?L"●  Android receiver":running?L"●  Test pattern running":L"●  Not connected",30,327,328,25,17,true,Color(255,235,157,247));
    text(g,message.c_str(),30,358,328,43,13,false,muted);
    rounded(g,RectF(18,419,358,143),Color(255,33,32,43),true);
    text(g,L"Camera Settings",30,433,328,28,18,true);
    text(g,L"Output",30,480,95,25); rounded(g,RectF(132,473,226,34),Color(255,43,41,54),true);
    text(g,L"Kagami",144,480,200,25);
    text(g,L"Format",30,524,95,25);
    rounded(g,RectF(18,574,358,158),Color(255,33,32,43),true);
    text(g,L"Preview Options",30,578,300,22,16,true);
    rounded(g,RectF(18,744,358,58),Color(255,33,32,43),true);
    text(g,L"Development preview · celestail-sora",30,814,328,28,12,false,muted);
    Pen divider(Color(255,68,63,79)); g.DrawLine(&divider,390.f,114.f,390.f,height-18);
    float panelHeight=height-320;
    rounded(g,RectF(406,114,right-18,panelHeight),Color(255,33,32,43),true);
    text(g,output?L"▣  Virtual camera output":L"▣  Live Preview",422,128,300,28,18,true);
    rounded(g,RectF(width-213,126,182,28),Color(255,43,38,56),true);
    wchar_t badge[80];swprintf_s(badge,L"%u × %u · %u FPS",format.width,format.height,format.fps);
    text(g,badge,width-209,131,174,24,13,false,muted,true);
    previewRect=RectF(422,163,right-50,panelHeight-65);
    LinearGradientBrush backdrop(previewRect,Color(255,48,33,64),Color(255,31,25,45),45.f);
    g.FillRectangle(&backdrop,previewRect);
    auto& shown=output?preview:source;
    imageRect=previewRect;
    unsigned shownWidth=output?format.width:kagami::width,shownHeight=output?format.height:kagami::height;
    if(running && shown.size()==size_t(shownWidth)*shownHeight*4) {
        Bitmap frame(shownWidth,shownHeight,shownWidth*4,PixelFormat32bppRGB,shown.data());
        float factor=(std::min)(previewRect.Width/shownWidth,previewRect.Height/shownHeight);
        RectF fit(previewRect.X+(previewRect.Width-shownWidth*factor)/2,previewRect.Y+(previewRect.Height-shownHeight*factor)/2,shownWidth*factor,shownHeight*factor);
        imageRect=fit;
        g.DrawImage(&frame,fit);
        if(!output) { Pen selection(Color(255,235,157,247),2);g.DrawRectangle(&selection,fit.X+cropRect.X*fit.Width,fit.Y+cropRect.Y*fit.Height,cropRect.Width*fit.Width,cropRect.Height*fit.Height); }
    } else {
        float center=previewRect.X+previewRect.Width/2, cy=previewRect.Y+previewRect.Height/2;
        monitor(g,center-24,cy-70,48);
        text(g,L"Your phone screen appears here",center-210,cy-8,420,32,20,true,ink,true);
        text(g,L"Start AirPlay or open USB / ADB Wi-Fi setup.",center-210,cy+28,420,28,15,false,muted,true);
    }
    if(guide) { Pen line(Color(160,240,220,250)); for(int i=1;i<3;++i) {
        float x=imageRect.X+imageRect.Width*i/3,y=imageRect.Y+imageRect.Height*i/3;
        g.DrawLine(&line,x,imageRect.Y,x,imageRect.GetBottom()); g.DrawLine(&line,imageRect.X,y,imageRect.GetRight(),y);
    }}
    if(safe) { Pen line(Color(200,240,220,250)); line.SetDashStyle(DashStyleDash);
        g.DrawRectangle(&line,imageRect.X+imageRect.Width*.05f,imageRect.Y+imageRect.Height*.05f,imageRect.Width*.9f,imageRect.Height*.9f); }
    rounded(g,RectF(406,height-184,right-18,142),Color(255,33,32,43),true);
    text(g,L"How to Use",422,height-170,300,25,18,true);
    float column=(right-58)/3;
    const wchar_t* headings[]={L"1  Start receiving",L"2  Adjust framing",L"3  Choose Kagami"};
    const wchar_t* descriptions[]={L"Enable camera, then Start AirPlay or connect an authorized Android device.",L"Drag the source to crop. Open Framing controls to rotate, mirror and fit.",L"Select Kagami in your camera app. AirPlay audio uses Desktop Audio."};
    for(int i=0;i<3;++i) {text(g,headings[i],422+i*column,height-126,column-18,27,16,true);text(g,descriptions[i],422+i*column,height-95,column-18,50,14,false,muted);}
    Graphics screen(dc); screen.DrawImage(&buffer,0,0);
    DrawIconEx(dc,int(26*scale),int(22*scale),icon,int(48*scale),int(48*scale),0,nullptr,DI_NORMAL);
}
inline void drawButton(DRAWITEMSTRUCT* item) {
    Graphics g(item->hDC); g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    float width=float(item->rcItem.right),height=float(item->rcItem.bottom);
    SolidBrush bg(Color(255,33,31,44)); g.FillRectangle(&bg,0.f,0.f,width,height);
    bool disabled=(item->itemState&ODS_DISABLED)!=0, pressed=(item->itemState&ODS_SELECTED)!=0;
    if(item->CtlID==105||item->CtlID==106||item->CtlID==110) {
        bool on=item->CtlID==105?guide:item->CtlID==106?safe:mirrored;
        const wchar_t* label=item->CtlID==105?L"Show Framing Guide":item->CtlID==106?L"Show Safe Area":L"Mirror Output";
        g.ScaleTransform(scale,scale);float logicalWidth=width/scale;
        text(g,label,8,8,logicalWidth-70,26,15);
        rounded(g,RectF(logicalWidth-58,5,54,28),on?Color(255,162,81,250):Color(255,81,77,96));
        SolidBrush thumb(ink);g.FillEllipse(&thumb,logicalWidth-(on?28.f:55.f),8.f,22.f,22.f);
        if(item->itemState&ODS_FOCUS){g.ResetTransform();RECT focus=item->rcItem;InflateRect(&focus,-2,-2);DrawFocusRect(item->hDC,&focus);}return;
    }
    Color color=disabled?Color(255,42,39,51):pressed?Color(255,113,55,160):item->CtlID==111?Color(255,168,75,237):Color(255,57,52,69);
    rounded(g,RectF(1,1,width-2,height-2),color,true);
    wchar_t label[128];GetWindowTextW(item->hwndItem,label,128);
    Font font(L"Segoe UI",14*scale,FontStyleRegular,UnitPixel);SolidBrush brush(disabled?Color(255,130,121,145):ink);
    StringFormat format;format.SetAlignment(StringAlignmentCenter);format.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(label,-1,&font,RectF(0,0,width,height),&format,&brush);
    if(item->itemState&ODS_FOCUS) {RECT focus=item->rcItem;InflateRect(&focus,-4,-4);DrawFocusRect(item->hDC,&focus);}
}
inline void refresh(HWND hwnd) { layout(hwnd); InvalidateRect(hwnd,nullptr,FALSE); }
}
