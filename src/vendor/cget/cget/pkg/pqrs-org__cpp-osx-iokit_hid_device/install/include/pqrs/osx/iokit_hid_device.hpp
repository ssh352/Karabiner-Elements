#pragma once

// pqrs::iokit_hid_device v2.2

// (C) Copyright Takayama Fumihiko 2018.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <optional>
#include <pqrs/cf_string.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace pqrs {
namespace osx {
class iokit_hid_device final {
public:
  iokit_hid_device(IOHIDDeviceRef device) : device_(device) {
  }

  virtual ~iokit_hid_device(void) {
  }

  cf_ptr<IOHIDDeviceRef> get_device(void) const {
    return device_;
  }

  std::optional<int64_t> find_int64_property(CFStringRef key) const {
    if (device_) {
      auto property = IOHIDDeviceGetProperty(*device_, key);
      if (property) {
        if (CFGetTypeID(property) == CFNumberGetTypeID()) {
          int64_t value = 0;
          if (CFNumberGetValue(static_cast<CFNumberRef>(property), kCFNumberSInt64Type, &value)) {
            return value;
          }
        }
      }
    }

    return std::nullopt;
  }

  std::optional<std::string> find_string_property(CFStringRef key) const {
    if (device_) {
      auto property = IOHIDDeviceGetProperty(*device_, key);
      return make_string(property);
    }

    return std::nullopt;
  }

  std::optional<int64_t> find_max_input_report_size(void) const {
    return find_int64_property(CFSTR(kIOHIDMaxInputReportSizeKey));
  }

  std::optional<iokit_hid_vendor_id> find_vendor_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDVendorIDKey))) {
      return iokit_hid_vendor_id(*value);
    }
    return std::nullopt;
  }

  std::optional<iokit_hid_product_id> find_product_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDProductIDKey))) {
      return iokit_hid_product_id(*value);
    }
    return std::nullopt;
  }

  std::optional<iokit_hid_location_id> find_location_id(void) const {
    if (auto value = find_int64_property(CFSTR(kIOHIDLocationIDKey))) {
      return iokit_hid_location_id(*value);
    }
    return std::nullopt;
  }

  std::optional<std::string> find_manufacturer(void) const {
    return find_string_property(CFSTR(kIOHIDManufacturerKey));
  }

  std::optional<std::string> find_product(void) const {
    return find_string_property(CFSTR(kIOHIDProductKey));
  }

  std::optional<std::string> find_serial_number(void) const {
    return find_string_property(CFSTR(kIOHIDSerialNumberKey));
  }

  std::optional<std::string> find_transport(void) const {
    return find_string_property(CFSTR(kIOHIDTransportKey));
  }

  std::vector<pqrs::cf_ptr<IOHIDElementRef>> make_elements(void) {
    std::vector<pqrs::cf_ptr<IOHIDElementRef>> result;

    if (device_) {
      // Note:
      //
      // Some devices has duplicated entries of the same usage_page and usage.
      // For example, there are entries of Microsoft Designer Mouse:
      //
      //   * Microsoft Designer Mouse usage_page 1 usage 2
      //   * Microsoft Designer Mouse usage_page 1 usage 2
      //   * Microsoft Designer Mouse usage_page 1 usage 1
      //   * Microsoft Designer Mouse usage_page 1 usage 56
      //   * Microsoft Designer Mouse usage_page 1 usage 56
      //   * Microsoft Designer Mouse usage_page 1 usage 568
      //   * Microsoft Designer Mouse usage_page 12 usage 568
      //   * ...
      //
      // We should treat them as independent entries.
      // (== We should not combine them into one entry.)

      if (auto elements = IOHIDDeviceCopyMatchingElements(*device_, nullptr, kIOHIDOptionsTypeNone)) {
        for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
          auto e = static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i)));
          if (e) {
            result.emplace_back(e);
          }
        }

        CFRelease(elements);
      }
    }

    return result;
  }

  cf_ptr<IOHIDQueueRef> make_queue(CFIndex depth) const {
    cf_ptr<IOHIDQueueRef> result;

    if (device_) {
      if (auto queue = IOHIDQueueCreate(kCFAllocatorDefault, *device_, depth, kIOHIDOptionsTypeNone)) {
        result = queue;

        CFRelease(queue);
      }
    }

    return result;
  }

private:
  cf_ptr<IOHIDDeviceRef> device_;
};
} // namespace osx
} // namespace pqrs