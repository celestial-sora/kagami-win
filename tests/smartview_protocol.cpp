#include "SmartViewProtocol.h"
#include <cassert>
int main(){
    using namespace kagami::smart;Packet packet;assert(valid(packet));
    packet.state=State::Video;assert(!valid(packet));packet.bytes=kagami::frameBytes;assert(valid(packet));
    packet.bytes++;assert(!valid(packet));packet={};packet.detail[95]=L'x';assert(!valid(packet));
    packet={};packet.state=State::Failed;assert(!valid(packet));packet.error=E_FAIL;assert(valid(packet));
    assert(validEndpoint(L"\\\\.\\pipe\\LOCAL\\Kagami.SmartView.542a79e7-54ac-48f9-b948-4620292bcc85"));
    assert(!validEndpoint(L"\\\\remote\\pipe\\LOCAL\\Kagami.SmartView.542a79e7-54ac-48f9-b948-4620292bcc85"));
    assert(!validEndpoint(L"\\\\.\\pipe\\LOCAL\\Kagami.SmartView.542a79e7-54ac-48f9-b948-4620292bcc8/"));
    assert(!validEndpoint(L"\\\\.\\pipe\\LOCAL\\Kagami.SmartView.542a79e7a54ac-48f9-b948-4620292bcc85"));
    assert((!kagami::OutputFormat{1920,1920,30}.valid()));assert((!kagami::OutputFormat{1280,720,1000}.valid()));
}
