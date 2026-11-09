#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr char kDevicePath[] = "/dev/hidraw0";
constexpr unsigned char kReportId = 0xCC;
constexpr std::size_t kReportLength = 33;

void printReport(const unsigned char* report, std::size_t length)
{
    std::cout << "Feature report 0xCC (" << length << " bytes):";
    for (std::size_t i = 0; i < length; ++i) {
        if (i % 16 == 0) {
            std::cout << "\n  ";
        }
        std::printf("%02X%s", report[i], i + 1 == length ? "" : " ");
    }
    std::cout << '\n';
}

} // namespace

int main()
{
    const int fd = open(kDevicePath, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "ERROR: cannot open " << kDevicePath << " read-only: "
                  << std::strerror(errno) << '\n';
        return 1;
    }

    std::array<unsigned char, kReportLength> report{};
    report[0] = kReportId;

    const int result = ioctl(fd, HIDIOCGFEATURE(kReportLength), report.data());
    const int ioctlError = errno;
    close(fd);

    if (result < 0) {
        std::cerr << "ERROR: HIDIOCGFEATURE(" << kReportLength
                  << ") for report ID 0xCC failed: "
                  << std::strerror(ioctlError) << '\n';
        return 1;
    }

    if (result == 0) {
        std::cerr << "ERROR: HIDIOCGFEATURE(33) returned no report bytes\n";
        return 1;
    }

    if (result > static_cast<int>(report.size())) {
        std::cerr << "ERROR: HIDIOCGFEATURE(33) reported " << result
                  << " bytes for a " << report.size() << "-byte buffer\n";
        return 1;
    }

    const std::size_t returnedLength = static_cast<std::size_t>(result);
    printReport(report.data(), returnedLength);

    if (report[0] != kReportId) {
        std::cerr << "ERROR: expected report ID 0xCC, got 0x"
                  << std::hex << std::uppercase
                  << static_cast<unsigned int>(report[0]) << std::dec << '\n';
        return 1;
    }

    if (returnedLength < kReportLength) {
        std::cerr << "WARNING: short response: descriptor-sized buffer is "
                  << kReportLength << " bytes, but the device returned "
                  << returnedLength
                  << ". Payload not interpreted; no retry was attempted.\n";
    } else {
        std::cout << "Received the descriptor-sized response; payload not "
                     "interpreted.\n";
    }

    return 0;
}
