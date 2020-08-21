#ifndef IOTASK_H
#define IOTASK_H

#include <Arduino.h>
#include <Task.h>
#include <MsgPacketizer.h>


// We might actually want use the canrun 
class IOHandler : public Task
{
    public:
        IOHandler();
        virtual void run(uint32_t now);
        virtual bool canRun(uint32_t now);

    private:
};

IOHandler::IOHandler()
: Task()
{
    // Do we need to contruct something ?
}

bool IOHandler::canRun(uint32_t now)
{
    timerWrite(GLOBAL_wdtimer, 0); //reset timer (feed watchdog)
    MsgPacketizer::post();  // If there was something to send, send it
    // Go to parsers if we had data to parse
    return (Serial && Serial.available()) || (SerialBT.connected() && SerialBT.available());
}

void IOHandler::run(uint32_t now)
{
    MsgPacketizer::parse();
}



#endif
