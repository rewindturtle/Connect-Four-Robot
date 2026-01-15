#pragma once

#include <atomic>


namespace utils {
    class AtomicFlag {
        private:
            std::atomic<bool> _flag;
        public:
            AtomicFlag() : _flag(false) {}
            AtomicFlag(bool initial) : _flag(initial) {}
            AtomicFlag(const AtomicFlag&) = delete;
            AtomicFlag& operator=(const AtomicFlag&) = delete;

            inline bool get() const {
                return _flag.load(std::memory_order_acquire);
            }

            inline void set(bool value) {
                _flag.store(value, std::memory_order_release);
            }

            inline bool exchange(bool newValue) {
                return _flag.exchange(newValue, std::memory_order_acq_rel);
            }

            inline operator bool() const {
                return get();
            }

            inline AtomicFlag& operator=(bool value) {
                set(value);
                return *this;
            }

            inline bool operator==(bool other) const {
                return get() == other;
            }

            inline bool operator!=(bool other) const {
                return get() != other;
            }
    };
}
