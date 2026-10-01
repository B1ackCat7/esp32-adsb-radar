#include "services/http_body_framing.h"

#include <cassert>
#include <sstream>
#include <string>

using services::http::BodyFramer;
using services::http::BodyFraming;

struct Source {
  std::string wire;
  size_t at = 0;
  int read() {
    return at < wire.size() ? static_cast<unsigned char>(wire[at++]) : -1;
  }
};

template <typename Reader>
std::string readAll(Reader& reader) {
  std::string result;
  char buffer[7];
  size_t n;
  while ((n = reader.readBytes(buffer, sizeof(buffer))) != 0)
    result.append(buffer, n);
  return result;
}

std::string chunk(const std::string& data) {
  std::ostringstream header;
  header << std::hex << data.size();
  return header.str() + ";example=yes\r\n" + data + "\r\n";
}

int main() {
  const std::string json = "{\"now\":123,\"aircraft\":[{\"hex\":\"abc123\",\"flight\":\"EXAMPLE\"}]}";
  // Every JSON split, including inside numbers/strings and multi-digit sizes.
  for (size_t split = 1; split < json.size(); ++split) {
    Source source{chunk(json.substr(0, split)) + chunk(json.substr(split)) +
                  "0\r\nExample-Trailer: yes\r\n\r\nNEXT"};
    BodyFramer<Source> body(source, BodyFraming::kChunked, -1);
    assert(readAll(body) == json);
    assert(body.complete() && !body.framingError() && !body.truncated());
    assert(body.bytesRead() == json.size());
    assert(source.wire.substr(source.at) == "NEXT");
  }
  // Parsing can finish before the terminator: draining detects missing or
  // malformed framing instead of accepting a syntactically complete JSON.
  for (const std::string tail : {"", "0", "0\r", "Z\r\n", "!"}) {
    Source source{chunk(json) + tail};
    BodyFramer<Source> body(source, BodyFraming::kChunked, -1);
    std::string parsed;
    for (size_t i = 0; i < json.size(); ++i) parsed += char(body.read());
    assert(parsed == json);
    body.drain();
    assert(!body.complete());
    assert(body.framingError() || body.truncated());
  }
  for (const std::string wire : {"G\r\n", "100000000\r\n", "1\r\nx!", "1\r\nx\r!"}) {
    Source source{wire};
    BodyFramer<Source> body(source, BodyFraming::kChunked, -1);
    readAll(body);
    assert(body.framingError() && !body.complete());
  }
  const std::string valid = chunk(json) + "0\r\n\r\n";
  const size_t terminator = chunk(json).size();
  for (size_t length = 0; length < terminator; ++length) {
    Source source{valid.substr(0, length)};
    BodyFramer<Source> body(source, BodyFraming::kChunked, -1);
    readAll(body);
    assert(body.truncated() && !body.complete());
  }
  for (int length : {0, int(json.size())}) {
    Source source{json + "NEXT"};
    BodyFramer<Source> body(source, BodyFraming::kIdentity, length);
    assert(readAll(body) == json.substr(0, length));
    assert(body.complete() && !body.truncated());
    assert(source.at == size_t(length));
  }
  {
    Source source{json};
    BodyFramer<Source> body(source, BodyFraming::kIdentity, int(json.size()) + 1);
    assert(readAll(body) == json);
    assert(body.truncated() && !body.complete());
  }
  {
    Source source{json};
    BodyFramer<Source> body(source, BodyFraming::kIdentity, -1);
    assert(readAll(body) == json);
    assert(body.complete() && !body.truncated());
  }
  // Upstream deliberately accepts a disconnect after the final zero chunk.
  {
    Source source{"0\r\n"};
    BodyFramer<Source> body(source, BodyFraming::kChunked, -1);
    assert(readAll(body).empty() && body.complete());
  }
}
