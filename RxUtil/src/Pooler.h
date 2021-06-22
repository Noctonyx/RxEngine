 //
// Created by shane on 27/01/2021.
//

#ifndef RX_POOLER_H
#define RX_POOLER_H

#include <memory>
#include <deque>
#include <functional>

namespace RxUtil
{
    template<typename T>
    struct Pooler
    {
        std::function<std::shared_ptr<T>(void)> allocator;
        uint16_t easy;

        explicit Pooler(uint16_t easySize)
            : easy(easySize) {}

        template<typename F>
        void setAllocator(F f)
        {
            allocator = std::move(f);
        }

        void freeUnused() {
            if (!pool.empty()) {
                auto i = pool.begin();
                while (i != pool.end() && i->use_count() == 1) {
                    pool.pop_front();
                }
            }
        }

        std::shared_ptr<T> get()
        {
            std::shared_ptr<T> p = nullptr;

            if (!pool.empty()) {
                auto i = pool.begin();
                while (i != pool.end() && i->use_count() == 1) {
                    p = *i;
                    pool.pop_front();
                    if (pool.size() < easy) {
                        break;
                    }
                    i = pool.begin();
                }
            }
            if (!p) {
                p = allocator();
            }
            pool.push_back(p);
            return p;
        }

        std::shared_ptr<T> getNoAlloc()
        {
            std::shared_ptr<T> p = nullptr;

            if (!pool.empty()) {
                auto i = pool.begin();
                while (i != pool.end() && i->use_count() == 1) {
                    p = *i;
                    pool.pop_front();
                    if (pool.size() < easy) {
                        break;
                    }
                    i = pool.begin();
                }
            }
            if (!p) {
                return nullptr;
            }
            pool.push_back(p);
            return p;
        }

        void add(std::shared_ptr<T> p) {
            pool.push_back(std::move(p));
        }

    private:
        std::deque<std::shared_ptr<T>> pool;
    };
}
#endif //RX_POOLER_H
