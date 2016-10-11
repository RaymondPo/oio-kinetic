/**
 * Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include "blob.h"
#include <utils/macros.h>

using oio::api::blob::Download;
using oio::api::blob::Errno;
using oio::api::blob::Status;
using oio::api::blob::Cause;

Status Download::SetRange(uint32_t offset UNUSED, uint32_t size UNUSED) {
    return Status(Cause::Unsupported);
}

static inline const char *Status2Str(Cause s) {
    switch (s) {
        case Cause::OK:
            return "OK";
        case Cause::Already:
            return "Already";
        case Cause::Forbidden:
            return "Forbidden";
        case Cause::NotFound:
            return "NotFound";
        case Cause::NetworkError:
            return "NetworkError";
        case Cause::ProtocolError:
            return "ProtocolError";
        case Cause::InternalError:
            return "InternalError";
        case Cause::Unsupported:
            return "Unsupported";
        default:
            return "***invalid status***";
    }
}

const char* Status::Name() const { return Status2Str(rc_); }

Cause _map(int err) {
    switch (err) {
        case ENOENT:
            return Cause::NotFound;
        case EEXIST:
            return Cause::Already;
        case ECONNRESET:
        case ECONNREFUSED:
        case ECONNABORTED:
        case EPIPE:
            return Cause::NetworkError;
        case EACCES:
        case EPERM:
        case ENOTDIR:
            return Cause::Forbidden;
        case ENOTSUP:
            return Cause::Unsupported;
        default:
            return Cause::InternalError;
    }
}

Errno::Errno(int err): Status() {
    if (err != 0)
        rc_ = _map(err);
    msg_.assign(::strerror(err));
}

Errno::Errno(): Errno(errno) {
}