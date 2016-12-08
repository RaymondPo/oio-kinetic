/**
 * This file is part of the test tools for the OpenIO client libraries
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

/**
 * Tests the internal HTTP client works as expected when accessing an OpenIO
 * Rawx service.
 */


#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include <memory>

#include "utils/macros.h"
#include "utils/net.h"
#include "utils/Http.h"
#include "bin/rawx-server-headers.h"
#include "tests/common/BlobTestSuite.h"
#include "oio/rawx/blob.h"

// The default address used for the target RAWX is an address typically
// generated by the oio-reset.sh tool applied on the "SINGLE" preset.
DEFINE_string(
        URL_RAWX,
        "127.0.0.1:6008",
        "URL of the test rawx");

using oio::rawx::blob::UploadBuilder;
using oio::rawx::blob::DownloadBuilder;
using oio::rawx::blob::RemovalBuilder;

class RawxBlobOpsFactory : public BlobOpsFactory {
    friend class RawxBlobTestSuite;

 protected:
    std::string url;
    std::string srvid, chunkid, cid, content_path, content_id;
    std::string mime_type, storage_policy, chunk_method;
    int64_t content_version;

    std::shared_ptr<net::Socket> socket;

 public:
    ~RawxBlobOpsFactory() override {}

    explicit RawxBlobOpsFactory(std::string u): url(u) {}

    void ResetConnection() {
        socket.reset(new net::MillSocket);
        ASSERT_TRUE(socket->connect(url));
    }
    std::unique_ptr<oio::api::blob::Upload> Upload() override {
        UploadBuilder op;

        rawx_cmd rawx_param ;
        rawx_param.rawx.host = srvid ;
        rawx_param.rawx.chunk_id = chunkid ;
        rawx_param.range.range_size = 0 ;
        op.set_param (rawx_param);

        op.ContainerId(cid);
        op.ContentPath(content_path);
        op.ContentId(content_id);
        op.ContentVersion(content_version);
        op.StoragePolicy(storage_policy);
        op.MimeType(mime_type);
        op.ChunkMethod(chunk_method);
        return op.Build(socket);
    }

    std::unique_ptr<oio::api::blob::Download> Download() override {
        DownloadBuilder op;

        rawx_cmd rawx_param ;
        rawx_param.rawx.host = srvid ;
        rawx_param.rawx.chunk_id = chunkid ;
        op.set_param (rawx_param);

        return op.Build(socket);
    }

    std::unique_ptr<oio::api::blob::Removal> Removal() override {
        RemovalBuilder op;

        rawx_cmd rawx_param ;
        rawx_param.rawx.host = srvid ;
        rawx_param.rawx.chunk_id = chunkid ;
        op.set_param (rawx_param);

        return op.Build(socket);
    }
};

class RawxBlobTestSuite : public BlobTestSuite {
 protected:
    std::string url_;

 protected:
    ~RawxBlobTestSuite() override {}

    RawxBlobTestSuite(): ::BlobTestSuite() {
        url_.assign(FLAGS_URL_RAWX);
    }

    void SetUp() override {
        auto f = new RawxBlobOpsFactory(url_);
        f->ResetConnection();
        f->srvid = generate_string_random(8, random_chars);
        f->chunkid = generate_string_random(64, random_hex);
        f->cid = generate_string_random(64, random_hex);
        f->content_path = generate_string_random(8, random_chars);
        f->content_version = ::time(0);
        f->content_id = generate_string_random(8, random_hex);
        f->storage_policy = "SINGLE";
        f->chunk_method = "plain";
        f->mime_type = "application/octet-stream";
        this->factory_ = f;
    }

    void TearDown() override {
        delete this->factory_;
        this->factory_ = nullptr;
    }
};

DECLARE_BLOBTESTSUITE(RawxBlobTestSuite);

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    FLAGS_logtostderr = true;

    // TODO(jfs) Run a rawx in a coroutine

    return RUN_ALL_TESTS();
}
