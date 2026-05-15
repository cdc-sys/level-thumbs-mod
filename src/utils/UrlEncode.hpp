#pragma once
#include <cctype>

void hexAppend(auto& buf, unsigned char c) {
    auto hexDigits = "0123456789ABCDEF";
    buf.append(hexDigits[(c >> 4) & 0xf]);
    buf.append(hexDigits[c & 0xf]);
}

// Encodes a url param
void urlEncodeAppend(auto& buf, std::string_view input) {
    for (char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            buf.append(c);
        } else {
            buf.append('%');
            hexAppend(buf, static_cast<unsigned char>(c));
        }
    }
}