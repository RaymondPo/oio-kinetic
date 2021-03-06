/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */


#include <cassert>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <cstring>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "oio/api/blob.h"
#include "oio/container/container.h"

using user_container::container;

#define SELECTOR(str) std::string("/v3.0/") + ContainerParam.NameSpace()    +\
                      std::string("/container/") + str                      +\
                      std::string("?acct=") + ContainerParam.Account()      +\
                      std::string("&ref=") +  ContainerParam.Container()    +\
                      (ContainerParam.Type().size() ? std::string("&type=") +\
                              ContainerParam.Type() :  "")

oio_err container::http_call_parse_body(http_param *http) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK) {
        err.get_message(-1, "HTTP_exec exec error");
    } else {
        if (!ContainerParam.put_properties(http->body_out))
            err.put_message(http->body_out);
    }
    return err;
}

oio_err container::http_call(http_param *http) {
    http::Code rc = http->HTTP_call();
    oio_err err;
    if (!rc == http::Code::OK)
        err.get_message(-1, "HTTP_exec exec error");
    else
        err.put_message(http->body_out);
    return err;
}

oio_err container::GetProperties() {
    http_param http(_socket, "POST", SELECTOR("get_properties"));
    return http_call_parse_body(&http);
}

oio_err container::Create() {
    http_param http(_socket, "POST", (SELECTOR("create")));
    ContainerParam.get_properties(&http.body_in);
    http.header_in["x-oio-action-mode"] = "autocreate";
    return http_call(&http);
}

oio_err container::SetProperties() {
    http_param http(_socket, "POST", (SELECTOR("set_properties")));
    ContainerParam.get_properties(&http.body_in);
    return http_call(&http);
}

oio_err container::DelProperties() {
    http_param http(_socket, "POST", (SELECTOR("del_properties")));
    ContainerParam.get_properties_key(&http.body_in, &del_properties);
    del_properties.clear();
    return http_call(&http);
}

oio_err container::Show()    {
    http_param http(_socket, "GET", (SELECTOR("show")));
    http.header_out_filter = "x-oio-container-meta-sys-";
    http.header_out = &ContainerParam.System();
    return http_call(&http);
}

oio_err container::List()    {
    http_param http(_socket, "GET", (SELECTOR("list")));
    http.header_out_filter = "x-oio-container-meta-sys-";
    http.header_out = &ContainerParam.System();
    return http_call(&http);
}

oio_err container::Destroy() {
    http_param http(_socket, "POST", (SELECTOR("destroy")));
    return http_call(&http);
}

oio_err container::Touch()   {
    http_param http(_socket, "POST", (SELECTOR("touch")));
    return http_call(&http);
}

oio_err container::Dedup()   {
    http_param http(_socket, "POST", (SELECTOR("dedup")));
    return http_call(&http);
}

