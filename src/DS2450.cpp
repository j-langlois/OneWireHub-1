#include "OneWireHub.h"
#include "DS2450.h"

DS2450::DS2450(uint8_t ID1, uint8_t ID2, uint8_t ID3, uint8_t ID4, uint8_t ID5, uint8_t ID6, uint8_t ID7) :
        OneWireItem(ID1, ID2, ID3, ID4, ID5, ID6, ID7)
{
    uint8_t mem_size = PAGE_COUNT*PAGE_SIZE;
    memset(&memory, 0, mem_size);
    if (mem_size > 0x1C) memory[0x1C] = 0x40;
}

bool DS2450::duty(OneWireHub *hub)
{
    uint16_t memory_address;
    uint16_t memory_address_start;
    uint8_t b;
    uint16_t crc;

    ow_crc16_reset();  // TODO: dump this function and replace it with the generic crc16

    uint8_t done = hub->recv();
    switch (done)
    {
        case 0xAA: // READ MEMORY

            ow_crc16_update(0xAA); // Cmd

            b = hub->recv(); // Adr1
            reinterpret_cast<uint8_t *>(&memory_address)[0] = b;
            ow_crc16_update(b);

            b = hub->recv(); // Adr2
            reinterpret_cast<uint8_t *>(&memory_address)[1] = b;
            ow_crc16_update(b);

            memory_address_start = memory_address;
            if (memory_address > (PAGE_COUNT-1)*PAGE_SIZE) memory_address = 0; // prevent read out of bounds

            for (uint8_t i = 0; i < PAGE_SIZE; ++i)
            {
                b = memory[memory_address + i];
                hub->send(b); // TODO: add possibility to break loop if send fails
                ow_crc16_update(b);
            }

            crc = ow_crc16_get();
            hub->send(reinterpret_cast<uint8_t *>(&crc)[0]);
            hub->send(reinterpret_cast<uint8_t *>(&crc)[1]);

            // TODO: not fully implemented

            if (dbg_sensor)
            {
                Serial.print("DS2450 : READ MEMORY : ");
                Serial.println(memory_address_start, HEX);
            }

            break;

        case 0x55: // write memory (only page 1&2 allowed)
            ow_crc16_update(0x55); // Cmd

            b = hub->recv(); // Adr1
            reinterpret_cast<uint8_t *>(&memory_address)[0] = b;
            ow_crc16_update(b);

            b = hub->recv(); // Adr2
            reinterpret_cast<uint8_t *>(&memory_address)[1] = b;
            ow_crc16_update(b);

            memory_address_start = memory_address;
            if (memory_address > (PAGE_COUNT-1)*PAGE_SIZE) memory_address = 0; // prevent read out of bounds

            for (uint8_t i = 0; i < PAGE_SIZE; ++i)
            {
                memory[memory_address + i] = hub->recv(); // TODO: add possibility to break loop if recv fails, hub->read_error?
                ow_crc16_update(memory[memory_address + i]);
            }

            crc = ow_crc16_get();
            hub->send(reinterpret_cast<uint8_t *>(&crc)[0]);
            hub->send(reinterpret_cast<uint8_t *>(&crc)[1]);

            // TODO: write back data if wanted, till the end of register

            if (dbg_sensor)
            {
                Serial.print("DS2450 : READ MEMORY : ");
                Serial.println(memory_address_start, HEX);
            }

        case 0x3C: // convert, starts adc
            ow_crc16_update(0x3C); // Cmd
            b = hub->recv(); // input select mask, not important
            ow_crc16_update(b);
            b = hub->recv(); // read out control byte
            ow_crc16_update(b);
            crc = ow_crc16_get();
            hub->send(reinterpret_cast<uint8_t *>(&crc)[0]);
            hub->send(reinterpret_cast<uint8_t *>(&crc)[1]);
            hub->sendBit(0); // still converting....
            hub->sendBit(1); // finished conversion

        default:
            if (dbg_HINT)
            {
                Serial.print("DS2450=");
                Serial.println(done, HEX);
            }
            break;
    }

    return true;
}

bool DS2450::setPotentiometer(const uint16_t p1, const uint16_t p2, const uint16_t p3, const uint16_t p4)
{
    setPotentiometer(0, p1);
    setPotentiometer(1, p2);
    setPotentiometer(2, p3);
    setPotentiometer(3, p4);
    return true;
};

bool DS2450::setPotentiometer(const uint8_t number, const uint16_t value)
{
    if (number > 3) return 1;
    uint8_t lbyte = (value>>0) & static_cast<uint8_t>(0xFF);
    uint8_t hbyte = (value>>8) & static_cast<uint8_t>(0xFF);
    memory[2*number+0] = lbyte;
    memory[2*number+1] = hbyte;
    return true;
};