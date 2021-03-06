/**
 * This file is part of the CLI tools around the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BIN_COMMON_HEADER_PARSER_H_
#define BIN_COMMON_HEADER_PARSER_H_

#include <http-parser/http_parser.h>

#include <string>

struct SoftError {
    int http, soft;
    const char *why;

    SoftError(): http{0}, soft{0}, why{nullptr} { }

    SoftError(int http, int soft, const char *why):
            http(http), soft(soft), why(why) {
    }

    void Reset() { http = 0; soft = 0, why = nullptr; }

    void Pack(std::string *dst);

    bool Ok() const { return http == 0 || (http == 200 && soft == 200); }
};

struct header_lexer_s {
    const char *ts, *te;
    int cs, act;
};

static inline bool _http_url_has(const http_parser_url &u, int field) {
    assert(field < UF_MAX);
    return 0 != (u.field_set & (1 << field));
}

static inline std::string _http_url_field(const http_parser_url &u, int f,
        const char *buf, size_t len) {
    if (!_http_url_has(u, f))
        return std::string("");
    assert(u.field_data[f].off <= len);
    assert(u.field_data[f].len <= len);
    assert(u.field_data[f].off + u.field_data[f].len <= len);
    return std::string(buf + u.field_data[f].off, u.field_data[f].len);
}

#endif  // BIN_COMMON_HEADER_PARSER_H_
