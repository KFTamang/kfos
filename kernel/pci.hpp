#include <cstdint>
#include <array>
#include "error.hpp"

namespace pci
{
    /** @brief CONFIG_ADDRESSレジスタのアドレス **/
    const uint16_t kConfigAddress = 0x0cf8;

    /** @brief CONFIG_DATAレジスタのアドレス **/
    const uint16_t kConfigData = 0x0cfc;

    struct ClassCode
    {
        uint8_t base, sub, interface;
        bool Match(uint8_t _base, uint8_t _sub);
        bool Match(uint8_t _base, uint8_t _sub, uint8_t _interface);
    };

    struct Device
    {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
        uint8_t header_type;
        ClassCode class_code;
    };
    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
    uint16_t ReadVendorId(Device &device);
    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

    inline std::array<Device, 32> devices;
    inline int num_device;

    Error ScanBus(uint8_t bus);
    Error ScanAllBus();

    WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index);
    constexpr uint8_t CalcBarAddress(unsigned int bar_index)
    {
        return 0x10 + 4 * bar_index;
    }
    uint32_t ReadConfReg(const Device &device, uint8_t addr);
    void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value);
};