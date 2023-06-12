#pragma once

#include <hal/io.h>

#include "base.h"

namespace Dev::Pci {

struct Id {
    u16 vendor;
    u16 device;
};

struct Addr {
    u16 seg;
    u8 bus;
    u8 slot;
    u8 func;
};

struct Device : public Node {
};

/* --- Group, Bus, Slot, Func ----------------------------------------------- */

struct Group : public Device {
};

struct Bus : public Device {
};

struct Slot : public Device {
};

struct Func : public Device {
};

} // namespace Dev::Pci
