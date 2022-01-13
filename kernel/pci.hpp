#include <cstdint>
#include <array>
#include "Error.hpp"

namespace pci
{
    /** @brief CONFIG_ADDRESSレジスタのアドレス **/
    const uint16_t kConfigAddress = 0x0cf8;

    /** @brief CONFIG_DATAレジスタのアドレス **/
    const uint16_t kConfigData = 0x0cfc;

    struct Device
    {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
        uint8_t header_type;
    };
    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

    inline std::array<Device, 32> devices;
    inline int num_device;

    Error ScanBus(uint8_t bus);
    Error ScanAllBus();
};