#include "pci.hpp"
#include "asmfunc.h"
#include <cstdint>

namespace pci
{

    bool ClassCode::Match(uint8_t _base, uint8_t _sub)
    {
        return (base == _base) && (sub == _sub);
    }

    bool ClassCode::Match(uint8_t _base, uint8_t _sub, uint8_t _interface)
    {
        return (base == _base) && (sub == _sub) && (interface == _interface);
    }

    uint32_t GenerateAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t register_offset)
    {
        auto shl = [](uint32_t x, uint8_t shift)
        {
            return x << shift;
        };
        return shl(1, 31) | shl(bus, 16) | shl(device, 11) | shl(function, 7) | (register_offset & 0xfcu);
    }

    void WriteAddress(uint32_t address)
    {
        IoOut32(kConfigAddress, address);
    }

    void WriteData(uint32_t data)
    {
        IoOut32(kConfigData, data);
    }

    uint32_t ReadData()
    {
        return IoIn32(kConfigData);
    }

    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function)
    {
        uint32_t addr = GenerateAddress(bus, device, function, 0x00u);
        WriteAddress(addr);
        uint32_t ret = ReadData();
        return (ret & 0xffffu) >> 0;
    }

    uint16_t ReadVendorId(Device &device)
    {
        return ReadVendorId(device.bus, device.device, device.function);
    }

    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function)
    {
        uint32_t addr = GenerateAddress(bus, device, function, 0x00u);
        WriteAddress(addr);
        uint32_t ret = ReadData();
        return ret >> 16;
    }

    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function)
    {
        uint32_t addr = GenerateAddress(bus, device, function, 0x0cu);
        WriteAddress(addr);
        uint32_t ret = ReadData();
        return (ret & 0x0f00u) >> 16;
    }

    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function)
    {
        uint32_t addr = GenerateAddress(bus, device, function, 0x08u);
        WriteAddress(addr);
        uint32_t ret = ReadData();
        uint8_t base = (ret >> 24) & 0xffu;
        uint8_t sub = (ret >> 16) & 0xffu;
        uint8_t interface = (ret >> 8) & 0xffu;
        ClassCode cc{base, sub, interface};
        return cc;
    }

    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function)
    {
        uint32_t addr = GenerateAddress(bus, device, function, 0x18u);
        WriteAddress(addr);
        uint32_t ret = ReadData();
        return ret;
    }

    bool IsSingleFunctionDevice(uint8_t header_type)
    {
        return (header_type & 0x80u) == 0;
    }

    Error AddDevice(uint8_t bus, uint8_t device, uint8_t function, uint8_t header_type)
    {
        if (num_device == devices.size())
        {
            return Error::kFull;
        }
        auto class_code = ReadClassCode(bus, device, function);
        devices[num_device] = Device{bus, device, function, header_type, class_code};
        num_device++;
        return Error::kSuccess;
    }

    Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function)
    {
        auto header_type = ReadHeaderType(bus, device, function);
        if (auto err = AddDevice(bus, device, function, header_type))
        {
            return err;
        }

        auto class_code = ReadClassCode(bus, device, function);

        if (class_code.Match(0x06u, 0x04u))
        {
            // standard PCI-PCI bridge
            auto bus_numbers = ReadBusNumbers(bus, device, function);
            uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
            return ScanBus(secondary_bus);
        }

        return Error::kSuccess;
    }

    Error ScanDevice(uint8_t bus, uint8_t device)
    {
        if (auto err = ScanFunction(bus, device, 0))
        {
            return err;
        }
        if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0)))
        {
            return Error::kSuccess;
        }
        for (uint8_t function = 1; function < 8; function++)
        {
            if (ReadVendorId(bus, device, function) == 0xffffu)
            {
                continue;
            }
            if (auto err = ScanFunction(bus, device, function))
            {
                return err;
            }
        }
        return Error::kSuccess;
    }

    Error ScanBus(uint8_t bus)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            if (ReadVendorId(bus, device, 0) == 0xffffu)
            {
                continue;
            }

            if (auto err = ScanDevice(bus, device))
            {
                return err;
            }
        }
        return Error::kSuccess;
    }

    Error ScanAllBus()
    {
        num_device = 0;

        auto header_type = ReadHeaderType(0, 0, 0);
        if (IsSingleFunctionDevice(header_type))
        {
            return ScanBus(0);
        }
        for (uint8_t function = 1; function < 8; function++)
        {
            if (ReadVendorId(0, 0, function) == 0xffffu)
            {
                continue;
            }

            if (auto err = ScanBus(function))
            {
                return err;
            }
        }
        return Error::kSuccess;
    }

    uint32_t ReadConfReg(const Device &dev, uint8_t reg_addr)
    {
        WriteAddress(GenerateAddress(dev.bus, dev.device, dev.function, reg_addr));
        return ReadData();
    }

    void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value)
    {
        WriteAddress(GenerateAddress(dev.bus, dev.device, dev.function, reg_addr));
        WriteData(value);
    }

    WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index)
    {
        if (bar_index >= 6)
        {
            return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
        }

        const auto addr = CalcBarAddress(bar_index);
        const auto bar = ReadConfReg(device, addr);

        // 32 bit address
        if ((bar & 4u) == 0)
        {
            return {bar, MAKE_ERROR(Error::kSuccess)};
        }

        // 64 bit address
        if (bar_index >= 5)
        {
            return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
        }

        const auto bar_upper = ReadConfReg(device, addr + 4);
        return {
            bar | (static_cast<uint64_t>(bar_upper) << 32),
            MAKE_ERROR(Error::kSuccess)};
    }

};