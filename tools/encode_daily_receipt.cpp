// Encode a daily-ticket JSON object with the same renderer used by XIAO/Indicator.
// Usage: encode_daily_receipt.exe input.json output.bin
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include "../firmware/xiao/include/daily_json.h"
#include "../firmware/xiao/include/daily_receipt.h"

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) return 2;
  const std::string text((std::istreambuf_iterator<char>(input)), {});
  if (text.size() > 2048) return 3;
  StaticJsonDocument<2048> document;
  if (deserializeJson(document, text)) return 3;
  robo::DailyTicket ticket;
  if (!robo::readDaily(document.as<JsonVariantConst>(), ticket)) return 3;
  std::ofstream output(argv[2], std::ios::binary);
  if (!output) return 2;
  size_t bytes = 0;
  const bool ok = robo::writeDailyReceipt(ticket, [&](uint8_t byte) {
    output.put(static_cast<char>(byte));
    ++bytes;
    return output.good();
  }, true);
  output.flush();
  if (!ok || !output.good()) return 4;
  std::cout << "Encoded " << bytes << " bytes, 384 dots, 128-row bands, cut enabled\n";
  return 0;
}
