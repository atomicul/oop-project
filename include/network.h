#ifndef NETWORK_H
#define NETWORK_H 1

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

class Phone;
class PhoneCDC;

class Network final {
  public:
    using ListenerToken = std::uint64_t;

  private:
    std::string name{};
    std::vector<std::unique_ptr<Phone>> phones{};
    mutable std::mutex mtx{};

    struct CDCListener {
        ListenerToken token{};
        std::string filter_name{};
        std::function<void(const PhoneCDC &)> cb{};
    };
    std::vector<CDCListener> listeners{};
    ListenerToken next_token{1};

    [[nodiscard]] Phone *findPhoneUnlocked(std::string_view phone_name);

  public:
    explicit Network(std::string name);

    void addPhone(std::unique_ptr<Phone> phone);
    [[nodiscard]] std::size_t size() const;

    // Looks up zero or more phones by name under a single lock and invokes the
    // trailing callback with that many `Phone&` arguments. Returns false (and
    // skips the callback) if any name is not found.
    template <typename... Args> bool withPhone(Args &&...args) {
        static_assert(sizeof...(Args) >= 1, "withPhone requires a callback argument");
        auto tup = std::forward_as_tuple(std::forward<Args>(args)...);
        return [this, &tup]<std::size_t... Is>(std::index_sequence<Is...>) {
            constexpr std::size_t N = sizeof...(Is);
            auto &cb = std::get<N>(tup);
            const std::lock_guard lock(mtx);
            std::array<Phone *, N> ptrs{};
            const bool found_all = ((ptrs[Is] = findPhoneUnlocked(std::get<Is>(tup))) && ...);
            if (!found_all) {
                return false;
            }
            cb(*ptrs[Is]...);
            return true;
        }(std::make_index_sequence<sizeof...(Args) - 1>{});
    }

    ListenerToken onPhoneTransmission(std::function<void(const PhoneCDC &)> cb);
    ListenerToken onPhoneTransmission(std::string filter_name,
                                      std::function<void(const PhoneCDC &)> cb);
    bool removeListener(ListenerToken token);

    friend std::ostream &operator<<(std::ostream &os, const Network &network);
};

#endif
