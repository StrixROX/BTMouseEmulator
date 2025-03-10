#include <iostream>
#include <MouseEmulator.h>
#include <simpleble/SimpleBLE.h>

int main(int argc, char** argv) {
    if (!SimpleBLE::Adapter::bluetooth_enabled()) {
        std::cout << "Bluetooth is not enabled" << std::endl;
        return 1;
    }

    std::vector<SimpleBLE::Adapter> availableAdapters = SimpleBLE::Adapter::get_adapters();
    if (availableAdapters.empty()) {
        std::cout << "No Bluetooth adapters found" << std::endl;
        return 1;
    }

    // Use the first adapter
    SimpleBLE::Adapter adapter = availableAdapters[0];

    // Get adapter details
    std::cout << "Using adapter: " << adapter.identifier() << " (" << adapter.address() << ")" << std::endl;




    // Get paired peripherals
    std::vector<SimpleBLE::Peripheral> pairedPeripherals = adapter.get_paired_peripherals();

    std::cout << std::endl;
    std::cout << "--- Paired Devices ---" << std::endl;
    if (pairedPeripherals.size() > 0) {
        for (auto peripheral : pairedPeripherals) {
            std::cout << "- " << peripheral.identifier() << " (" << peripheral.address() << ")" << std::endl;
        }
    }
    else {
        std::cout << "No paired devices found" << std::endl;
    }




    std::cout << std::endl;
    std::cout << "Scanning for Bluetooth devices (5s)..." << std::endl;

    std::vector<SimpleBLE::Peripheral> peripherals;

    adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {
        if (peripheral.is_connectable()) {
            peripherals.push_back(peripheral);
        }
    });

    // Scan for peripherals for 5000 milliseconds
    adapter.scan_for(5000);

    // Get the list of peripherals found

    std::cout << std::endl;
    std::cout << "--- Devices ---" << std::endl;
    if (peripherals.size() > 0) {

        // Print the identifier of each peripheral
        for (auto peripheral : peripherals) {
            std::cout << "- " << peripheral.identifier() << " (" << peripheral.address() << ")" << std::endl;
        }
    }
    else {
        std::cout << "No devices found" << std::endl;
    }

    return 0;
}