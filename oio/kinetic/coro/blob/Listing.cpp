/** Copyright 2016 Contributors (see the AUTHORS file)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, you can
 * obtain one at https://mozilla.org/MPL/2.0/ */

#include <string>
#include <glog/logging.h>
#include <oio/api/blob.h>
#include <oio/kinetic/coro/client/ClientInterface.h>
#include <oio/kinetic/coro/rpc/GetKeyRange.h>
#include <oio/kinetic/coro/blob.h>

using oio::kinetic::blob::ListingBuilder;
using oio::kinetic::client::ClientInterface;
using oio::kinetic::client::ClientFactory;
using oio::kinetic::client::Sync;
using oio::kinetic::rpc::GetKeyRange;

namespace blob = ::oio::api::blob;

class Listing : public blob::Listing {
    friend class ListingBuilder;

  public:
    Listing() noexcept;

    ~Listing() noexcept { }

    blob::Listing::Status Prepare() noexcept override;

    bool Next(std::string &id, std::string &dst) noexcept override;
    
  private:
    std::vector<std::shared_ptr<ClientInterface>> clients;
    std::string name;

    std::vector<std::pair<int, std::string>> items;
    unsigned int next_item;
};

Listing::Listing() noexcept
        : clients(), name(), items(), next_item{0} { }

bool Listing::Next(std::string &id, std::string &key) noexcept {
    if (next_item >= items.size())
        return false;

    const auto &item = items[next_item++];
    id.assign(clients[std::get<0>(item)]->Id());
    key.assign(std::get<1>(item));
    return true;
}

// Concurrently get the lists on each node
// TODO limit the parallelism to N (configurable) items
blob::Listing::Status Listing::Prepare() noexcept {

    items.clear();
    std::vector<std::shared_ptr<GetKeyRange>> ops;
    std::vector<std::shared_ptr<Sync>> syncs;

    for (auto cli: clients) {
        auto gkr = new GetKeyRange;
        gkr->Start(name + "-#");
        gkr->End(name + "-X");
        gkr->IncludeStart(true);
        gkr->IncludeEnd(false);
        ops.emplace_back(gkr);
        syncs.emplace_back(cli->Start(gkr));
    }
    for (auto sync: syncs)
        sync->Wait();

    for (unsigned int i = 0; i < ops.size(); ++i) {
        std::vector<std::string> keys;
        ops[i]->Steal(keys);
        for (auto &k: keys) {
            // We try here to avoid copies
            std::string s;
            s.swap(k);
            items.emplace_back(i, std::move(s));
            assert(s.size() == 0);
            assert(k.size() == 0);
        }
    }

    if (items.empty())
        return blob::Listing::Status::NotFound;

    return blob::Listing::Status::OK;
}

ListingBuilder::~ListingBuilder() { }

ListingBuilder::ListingBuilder(std::shared_ptr<ClientFactory> f) noexcept
        : factory(f), targets(), name() { }

void ListingBuilder::Name(const std::string &n) noexcept {
    name.assign(n);
}

void ListingBuilder::Name(const char *n) noexcept {
    assert(n != nullptr);
    return Name(std::string(n));
}

void ListingBuilder::Target(const std::string &to) noexcept {
    targets.insert(to);
}

void ListingBuilder::Target(const char *to) noexcept {
    assert(to != nullptr);
    return Target(std::string(to));
}

std::unique_ptr<blob::Listing> ListingBuilder::Build() noexcept {
    assert(!targets.empty());
    assert(!name.empty());

    auto listing = new Listing;
    listing->name.assign(name);
    for (auto to: targets)
        listing->clients.emplace_back(factory->Get(to));
    return std::unique_ptr<Listing>(listing);
}
