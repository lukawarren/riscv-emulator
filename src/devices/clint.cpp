#include "devices/clint.h"
#include "cpu.h"

/*
    The CLINT has three registers:
    - mtime - a timer register that increases at a constant frequency
    - mtimecmp - used to trigger interrupts when compared to mtime
    - msip - sip = software interrupt pending; used for software interrupts
*/
#define MSIP            0x0
#define MSIP_END        MSIP + sizeof(msip)
#define MTIME           0xbff8
#define MTIME_END       MTIME + sizeof(mtime)
#define MTIMECMP        0x4000
#define MTIMECMP_END    MTIMECMP + sizeof(mtimecmp)

std::optional<u64> CLINT::read_byte(const u64 address)
{
    if (address < MSIP_END)
    {
        switch (address % 4)
        {
            case 0: return { (msip >>  0) & 0xff };
            case 1: return { (msip >>  8) & 0xff };
            case 2: return { (msip >> 16) & 0xff };
            case 3: return { (msip >> 24) & 0xff };
        }
    }

    else if (address >= MTIME && address < MTIME_END)
    {
        switch (address % 8)
        {
            case 0: return { (mtime >>  0) & 0xff };
            case 1: return { (mtime >>  8) & 0xff };
            case 2: return { (mtime >> 16) & 0xff };
            case 3: return { (mtime >> 24) & 0xff };
            case 4: return { (mtime >> 32) & 0xff };
            case 5: return { (mtime >> 40) & 0xff };
            case 6: return { (mtime >> 48) & 0xff };
            case 7: return { (mtime >> 56) & 0xff };
        }
    }

    else if (address >= MTIMECMP && address < MTIMECMP_END)
    {
        switch (address % 8)
        {
            case 0: return { (mtimecmp >>  0) & 0xff };
            case 1: return { (mtimecmp >>  8) & 0xff };
            case 2: return { (mtimecmp >> 16) & 0xff };
            case 3: return { (mtimecmp >> 24) & 0xff };
            case 4: return { (mtimecmp >> 32) & 0xff };
            case 5: return { (mtimecmp >> 40) & 0xff };
            case 6: return { (mtimecmp >> 48) & 0xff };
            case 7: return { (mtimecmp >> 56) & 0xff };
        }
    }

    assert(false);
    return std::nullopt;
}

bool CLINT::write_byte(const u64 address, const u8 value)
{
    if (address < MSIP_END)
    {
        switch (address % 4)
        {
            case 0: msip = (msip & 0xffffff00) | ((u32)value <<  0); return true;
            case 1: msip = (msip & 0xffff00ff) | ((u32)value <<  8); return true;
            case 2: msip = (msip & 0xff00ffff) | ((u32)value << 16); return true;
            case 3: msip = (msip & 0x00ffffff) | ((u32)value << 24); return true;
        }
    }

    else if (address >= MTIME && address < MTIME_END)
    {
        switch (address % 8)
        {
            case 0: mtime = (mtime & 0xffffffffffffff00) | ((u64)value <<  0); return true;
            case 1: mtime = (mtime & 0xffffffffffff00ff) | ((u64)value <<  8); return true;
            case 2: mtime = (mtime & 0xffffffffff00ffff) | ((u64)value << 16); return true;
            case 3: mtime = (mtime & 0xffffffff00ffffff) | ((u64)value << 24); return true;
            case 4: mtime = (mtime & 0xffffff00ffffffff) | ((u64)value << 32); return true;
            case 5: mtime = (mtime & 0xffff00ffffffffff) | ((u64)value << 40); return true;
            case 6: mtime = (mtime & 0xff00ffffffffffff) | ((u64)value << 48); return true;
            case 7: mtime = (mtime & 0x00ffffffffffffff) | ((u64)value << 56); return true;
        }
    }

    else if (address >= MTIMECMP && address < MTIMECMP_END)
    {
        switch (address % 8)
        {
            case 0: mtimecmp = (mtimecmp & 0xffffffffffffff00) | ((u64)value <<  0); return true;
            case 1: mtimecmp = (mtimecmp & 0xffffffffffff00ff) | ((u64)value <<  8); return true;
            case 2: mtimecmp = (mtimecmp & 0xffffffffff00ffff) | ((u64)value << 16); return true;
            case 3: mtimecmp = (mtimecmp & 0xffffffff00ffffff) | ((u64)value << 24); return true;
            case 4: mtimecmp = (mtimecmp & 0xffffff00ffffffff) | ((u64)value << 32); return true;
            case 5: mtimecmp = (mtimecmp & 0xffff00ffffffffff) | ((u64)value << 40); return true;
            case 6: mtimecmp = (mtimecmp & 0xff00ffffffffffff) | ((u64)value << 48); return true;
            case 7: mtimecmp = (mtimecmp & 0x00ffffffffffffff) | ((u64)value << 56); return true;
        }
    }

    assert(false);
    return false;
}

void CLINT::increment(CPU& cpu)
{
    /*
        Increment the mtime register.
        The MTIP bit in the MIP status register gets enabled when mtime >= mtimecmp.
        If at any point mtimecmp > mtime (i.e. someone wrote to it), it's cleared.
        Software interrupts instead manipulate the MSIP register.
     */

    mtime +=1;

    if ((msip & 1) != 0)
        cpu.mip.set_msi();

    if (mtimecmp > mtime)
        cpu.mip.clear_mti();

    if (mtime >= mtimecmp)
        cpu.mip.set_mti();
}
