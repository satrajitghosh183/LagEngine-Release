#pragma once

#include <atomic>
#include <vector>
#include <cstdint>
#include <cassert>

namespace GameEngine {

    /**
     * @brief Lock-free Chase-Lev work-stealing deque
     *
     * The owning thread pushes/pops from the bottom (LIFO).
     * Other threads steal from the top (FIFO).
     * This gives cache-friendly execution for the owner and
     * load balancing via stealing for other workers.
     *
     * Based on: "Dynamic Circular Work-Stealing Deque" by Chase & Lev (2005)
     */
    template <typename T>
    class WorkStealingDeque {
    public:
        explicit WorkStealingDeque(int64_t capacity = 1024)
            : m_Bottom(0), m_Top(0) {
            m_Array.store(new CircularArray(capacity), std::memory_order_relaxed);
        }

        ~WorkStealingDeque() {
            delete m_Array.load(std::memory_order_relaxed);
        }

        WorkStealingDeque(const WorkStealingDeque&) = delete;
        WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;

        // Owner thread only
        void Push(T item) {
            int64_t b = m_Bottom.load(std::memory_order_relaxed);
            int64_t t = m_Top.load(std::memory_order_acquire);
            CircularArray* a = m_Array.load(std::memory_order_relaxed);

            if (b - t > a->Size() - 1) {
                // Grow the array
                a = a->Grow(t, b);
                m_Array.store(a, std::memory_order_relaxed);
            }

            a->Put(b, std::move(item));
            std::atomic_thread_fence(std::memory_order_release);
            m_Bottom.store(b + 1, std::memory_order_relaxed);
        }

        // Owner thread only
        bool Pop(T& result) {
            int64_t b = m_Bottom.load(std::memory_order_relaxed) - 1;
            CircularArray* a = m_Array.load(std::memory_order_relaxed);
            m_Bottom.store(b, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_seq_cst);

            int64_t t = m_Top.load(std::memory_order_relaxed);
            if (t <= b) {
                result = a->Get(b);
                if (t == b) {
                    // Last element — race with stealers
                    if (!m_Top.compare_exchange_strong(t, t + 1,
                            std::memory_order_seq_cst, std::memory_order_relaxed)) {
                        // Lost the race
                        m_Bottom.store(t + 1, std::memory_order_relaxed);
                        return false;
                    }
                    m_Bottom.store(t + 1, std::memory_order_relaxed);
                }
                return true;
            } else {
                // Deque was empty
                m_Bottom.store(t, std::memory_order_relaxed);
                return false;
            }
        }

        // Any thread can steal (called by non-owner threads)
        bool Steal(T& result) {
            int64_t t = m_Top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            int64_t b = m_Bottom.load(std::memory_order_acquire);

            if (t < b) {
                CircularArray* a = m_Array.load(std::memory_order_consume);
                result = a->Get(t);
                if (!m_Top.compare_exchange_strong(t, t + 1,
                        std::memory_order_seq_cst, std::memory_order_relaxed)) {
                    return false; // Lost the race — another stealer got it
                }
                return true;
            }
            return false; // Empty
        }

        int64_t Size() const {
            int64_t b = m_Bottom.load(std::memory_order_relaxed);
            int64_t t = m_Top.load(std::memory_order_relaxed);
            return b >= t ? b - t : 0;
        }

        bool Empty() const { return Size() == 0; }

    private:
        struct CircularArray {
            explicit CircularArray(int64_t n) : m_Mask(n - 1) {
                assert((n & (n - 1)) == 0 && "Capacity must be a power of 2");
                m_Data.resize(static_cast<size_t>(n));
            }

            int64_t Size() const { return m_Mask + 1; }

            T Get(int64_t i) const {
                return m_Data[static_cast<size_t>(i & m_Mask)];
            }

            void Put(int64_t i, T item) {
                m_Data[static_cast<size_t>(i & m_Mask)] = std::move(item);
            }

            CircularArray* Grow(int64_t t, int64_t b) {
                auto* newArr = new CircularArray(Size() * 2);
                for (int64_t i = t; i < b; i++) {
                    newArr->Put(i, Get(i));
                }
                return newArr;
            }

        private:
            std::vector<T> m_Data;
            int64_t m_Mask;
        };

        std::atomic<int64_t> m_Bottom;
        std::atomic<int64_t> m_Top;
        std::atomic<CircularArray*> m_Array;
    };

} // namespace GameEngine
