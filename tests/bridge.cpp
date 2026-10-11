#include "FrameBridge.h"
#include <iostream>
#include <algorithm>
#include <chrono>
static bool waitFor(kagami::FrameReader& reader, bool empty, BYTE value=0) {
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while (std::chrono::steady_clock::now()<end) {
        auto frame=reader.snapshot();
        if (empty && frame.empty()) return true;
        if (!empty && frame.size()==kagami::frameBytes && std::all_of(frame.begin(),frame.end(),[value](BYTE x){ return x==value; })) return true;
        Sleep(10);
    }
    return false;
}
int main() {
    auto require=[](bool ok,const char* message){ if (!ok) { std::cerr<<message<<std::endl; exit(1); } };
    require(kagami::valid(kagami::header),"valid protocol");
    auto invalid=kagami::header; invalid.bytes++; require(!kagami::valid(invalid),"oversized frame rejected");
    require(!kagami::validPipe(L"\\\\remote\\pipe\\Kagami"),"remote endpoint rejected");
    kagami::FrameWriter writer,second;
    require(writer.start(),"producer start");
    require(!second.start(),"exclusive producer");
    kagami::FrameReader reader; reader.start(kagami::pipeName());
    require(waitFor(reader,false,0),"initial black slate");
    require(!writer.push(std::vector<BYTE>(1)),"partial frames rejected");
    require(writer.push(std::vector<BYTE>(kagami::frameBytes,73)),"publish complete frame");
    require(waitFor(reader,false,73),"actual named pipe delivers full frame");
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
