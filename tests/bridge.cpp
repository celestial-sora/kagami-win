#include "FrameBridge.h"
#include <iostream>
#include <algorithm>
#include <chrono>
static bool waitFor(kagami::FrameReader& reader, bool empty, BYTE value=0,kagami::OutputFormat format={}) {
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while (std::chrono::steady_clock::now()<end) {
        auto frame=reader.snapshotFrame();
        if (empty && frame.pixels.empty()) return true;
        if (!empty && frame.format.width==format.width && frame.format.height==format.height &&
            frame.pixels.size()==size_t(format.width)*format.height*4 &&
            std::all_of(frame.pixels.begin(),frame.pixels.end(),[value](BYTE x){ return x==value; })) return true;
        Sleep(10);
    }
    return false;
}
int main() {
    auto require=[](bool ok,const char* message){ if (!ok) { std::cerr<<message<<std::endl; exit(1); } };
    require(kagami::valid(kagami::header),"valid protocol");
    auto invalid=kagami::header; invalid.bytes++; require(!kagami::valid(invalid),"oversized frame rejected");
    for(auto format:{kagami::OutputFormat{1920,1080,60},kagami::OutputFormat{720,1280,24}}) {
        kagami::Header sized{kagami::header.magic,format.width,format.height,format.width*format.height*4};
        require(kagami::valid(sized),"landscape/portrait protocol accepted");
    }
    invalid={kagami::header.magic,0xffffffff,0xffffffff,4};require(!kagami::valid(invalid),"overflow dimensions rejected");
    invalid={kagami::header.magic,1920,1920,1920*1920*4};require(!kagami::valid(invalid),"pixel budget rejected");
    invalid={kagami::header.magic,1279,720,1279*720*4};require(!kagami::valid(invalid),"odd NV12 dimensions rejected");
    require(!kagami::validPipe(L"\\\\remote\\pipe\\Kagami"),"remote endpoint rejected");
    kagami::FrameWriter writer,second;
    require(writer.start(),"producer start");
    require(!second.start(),"exclusive producer");
    kagami::FrameReader reader; reader.start(kagami::pipeName());
    require(waitFor(reader,false,0),"initial black slate");
    require(!writer.push(std::vector<BYTE>(1)),"partial frames rejected");
    require(writer.push(std::vector<BYTE>(kagami::frameBytes,73)),"publish complete frame");
    require(waitFor(reader,false,73),"actual named pipe delivers full frame");
    for(auto format:{kagami::OutputFormat{1920,1080,60},kagami::OutputFormat{720,1280,24}}) {
        require(writer.push(std::vector<BYTE>(size_t(format.width)*format.height*4,85),format),"publish selected dimensions");
        require(waitFor(reader,false,85,format),"format and pixels remain atomic across resize");
    }
    require(!writer.push(std::vector<BYTE>(kagami::frameBytes),{1280,720,1000}),"unsupported pacing rejected");
    writer.slate(); require(waitFor(reader,false,0),"slate clears last frame");
    writer.stop(); require(waitFor(reader,true),"disconnect clears snapshot");
    require(writer.start(),"producer restart");
    require(writer.push(std::vector<BYTE>(kagami::frameBytes,91)),"publish after restart");
    require(waitFor(reader,false,91),"consumer reconnects to owned local producer");
    reader.stop(); writer.stop();
    auto before=std::chrono::steady_clock::now();
    require(writer.start(),"start without consumer"); writer.stop();
    require(std::chrono::steady_clock::now()-before<std::chrono::seconds(2),"Stop cancels pending connection");
    require(writer.start(),"start stalled consumer test");
    HANDLE stalled=CreateFileW(kagami::pipeName().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);
    require(stalled!=INVALID_HANDLE_VALUE,"connect consumer without reading"); Sleep(150);
    before=std::chrono::steady_clock::now(); writer.stop(); CloseHandle(stalled);
    require(std::chrono::steady_clock::now()-before<std::chrono::seconds(2),"Stop cancels stalled write");
    std::cout<<"PASS: protocol, exclusive ownership, named pipe frames, slate, disconnect, reconnect, bounded Stop\n";
}
