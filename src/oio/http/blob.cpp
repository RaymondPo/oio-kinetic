/**
 * Copyright (c) 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/
 */

#include <http-parser/http_parser.h>

#include <iomanip>
#include <cstring>
#include <vector>

#include "utils/Http.h"
#include "oio/http/blob.h"

using oio::http::imperative::RemovalBuilder;
using oio::http::imperative::UploadBuilder;
using oio::http::imperative::DownloadBuilder;
using oio::api::blob::Removal;
using oio::api::blob::Upload;
using oio::api::blob::Download;
using oio::api::blob::Status;
using oio::api::blob::Cause;

class HttpRemoval : public Removal {
    friend class RemovalBuilder;

 public:
    HttpRemoval();

    virtual ~HttpRemoval();

    Status Prepare() override;

    Status Commit() override;

    Status Abort() override;

 private:
    http::Call rpc;
};

class HttpUpload : public Upload {
    friend class UploadBuilder;

 public:
    HttpUpload();

    ~HttpUpload();

    void SetXattr(const std::string &k, const std::string &v) override;

    Status Prepare() override;

    Status Commit() override;

    Status Abort() override;

    void Write(const uint8_t *buf, uint32_t len) override;

 private:
    http::Request request;
    http::Reply reply;
};

class HttpDownload : public Download {
    friend class DownloadBuilder;

 public:
    HttpDownload();

    ~HttpDownload();

    bool IsEof() override;

    Status Prepare() override;

    int32_t Read(std::vector<uint8_t> *buf) override;

 private:
    http::Request request;
    http::Reply reply;
};


HttpRemoval::HttpRemoval() {}

HttpRemoval::~HttpRemoval() {}

/* TODO(jfs) Manage prepare/commit/abort semantics.
 * For example, we could prepare the request here, just sending the headers with
 * the Transfer-Enncoding set to chunked. So we could wait for the commit to
 * finish the request */
Status HttpRemoval::Prepare() {
    return Status();
}

Status HttpRemoval::Commit() {
    std::string out;
    auto rc = rpc.Run("", &out);
    if (rc == http::Code::OK || rc == http::Code::Done)
        return Status();
    // TODO(jfs) better error management
    return Status(Cause::InternalError);
}

Status HttpRemoval::Abort() {
    LOG(WARNING) << "Cannot abort a HTTP delete";
    return Status(Cause::Unsupported);
}

RemovalBuilder::RemovalBuilder() {}

RemovalBuilder::~RemovalBuilder() {}

void RemovalBuilder::Name(const std::string &s) { name.assign(s); }

void RemovalBuilder::Host(const std::string &s) { host.assign(s); }

void RemovalBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void RemovalBuilder::Trailer(const std::string &k) { trailers.insert(k); }

std::shared_ptr<oio::api::blob::Removal> RemovalBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto rm = new HttpRemoval;
    rm->rpc.Socket(socket).Method("DELETE").Selector(name).Field("Host", host);
    std::shared_ptr<Removal> shared(rm);
    return shared;
}


HttpUpload::HttpUpload() : request(), reply() {}

HttpUpload::~HttpUpload() {}

void HttpUpload::SetXattr(const std::string &k, const std::string &v) {
    request.Field(k, v);
}

Status HttpUpload::Commit() {
    auto rc = request.FinishRequest();

    rc = reply.ReadHeaders();
    while (rc == http::Code::OK) {
        std::vector<uint8_t> out;
        rc = reply.AppendBody(&out);
    }

    if (reply.Get().parser.status_code / 100 == 2)
        return Status();
    return Status(Cause::InternalError);
}

Status HttpUpload::Abort() { return Status(Cause::Unsupported); }

Status HttpUpload::Prepare() {
    auto rc = request.WriteHeaders();
    (void) rc;
    return Status(Cause::InternalError);
}

void HttpUpload::Write(const uint8_t *buf, uint32_t len) {
    if (buf == nullptr || len == 0)
        return;
    request.Write(buf, len);
}

UploadBuilder::UploadBuilder() {}

UploadBuilder::~UploadBuilder() {}

void UploadBuilder::Name(const std::string &s) { name.assign(s); }

void UploadBuilder::Host(const std::string &s) { host.assign(s); }

void UploadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void UploadBuilder::Trailer(const std::string &k) { trailers.emplace(k); }

std::shared_ptr<oio::api::blob::Upload> UploadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto ul = new HttpUpload;
    ul->request.Socket(socket);
    ul->request.Method("PUT");
    ul->request.Selector(name);
    ul->request.Field("Host", host);
    for (const auto &t : trailers)
        ul->request.Trailer(t);
    for (const auto &e : fields)
        ul->request.Field(e.first, e.second);
    ul->reply.Socket(socket);
    return std::shared_ptr<Upload>(ul);
}


HttpDownload::HttpDownload() : request(), reply() {}

HttpDownload::~HttpDownload() {}

bool HttpDownload::IsEof() {
    return reply.Get().step == http::Reply::Step::Done;
}

Status HttpDownload::Prepare() {
    auto rc = request.WriteHeaders();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }

    rc = request.FinishRequest();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }

    rc = reply.ReadHeaders();
    if (rc != http::Code::OK && rc != http::Code::Done) {
        if (rc == http::Code::NetworkError)
            return Status(Cause::NetworkError);
        return Status(Cause::InternalError);
    }

    return Status(Cause::OK);
}

int32_t HttpDownload::Read(std::vector<uint8_t> *buf) {
    buf->clear();
    reply.AppendBody(buf);
    return buf->size();
}

DownloadBuilder::DownloadBuilder() {}

DownloadBuilder::~DownloadBuilder() {}

void DownloadBuilder::Field(const std::string &k, const std::string &v) {
    fields[k] = v;
}

void DownloadBuilder::Host(const std::string &s) { host.assign(s); }

void DownloadBuilder::Name(const std::string &s) { name.assign(s); }

std::shared_ptr<oio::api::blob::Download> DownloadBuilder::Build(
        std::shared_ptr<net::Socket> socket) {
    auto dl = new HttpDownload;
    dl->request.Socket(socket);
    dl->request.Method("GET");
    dl->request.Selector(name);
    dl->request.Field("Host", host);
    for (const auto &e : fields)
        dl->request.Field(e.first, e.second);
    dl->reply.Socket(socket);
    return std::shared_ptr<Download>(dl);
}
