/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <cstddef>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <memory>

template <typename T>
class ObserverList
{
public:
    ObserverList() = default;
    ~ObserverList() = default;

    ObserverList(const ObserverList&) = delete;
    ObserverList& operator=(const ObserverList&) = delete;

    void addObserver(T* observer)
    {
        if (!observer)
            return;

        std::lock_guard<std::mutex> lock(mLock);

        for (auto& entry : mObservers)
        {
            if (entry->mPtr == observer)
            {
                // Re-adding an observer that is pending removal cancels removal.
                entry->mPendingRemoval.store(false);
                return;
            }
        }

        //mObservers.push_back(std::make_unique<Entry>(observer)); // c++14
        mObservers.emplace_back(std::unique_ptr<Entry>(new Entry(observer)));
    }

    void removeObserver(T* observer)
    {
        if (!observer)
            return;

        std::lock_guard<std::mutex> lock(mLock);

        for (auto& entry : mObservers)
        {
            if (entry->mPtr == observer)
            {
                entry->mPendingRemoval.store(true);
                break;
            }
        }

        if (mNotifyDepth == 0)
            compactLocked();
    }

    template <typename Func>
    void notify(Func&& action)
    {
        std::vector<Entry*> targets;

        {
            std::lock_guard<std::mutex> lock(mLock);

            ++mNotifyDepth;

            targets.reserve(mObservers.size());

            for (const auto& entry : mObservers)
            {
                if (!entry->mPendingRemoval.load() && entry->mPtr)
                    targets.push_back(entry.get());
            }
        }

        try
        {
            for (Entry* entry : targets)
            {
                if (!entry->mPendingRemoval.load())
                    action(entry->mPtr);
            }
        }
        catch (...)
        {
            finishNotify();
            throw;
        }

        finishNotify();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mLock);

        for (const auto& entry : mObservers)
        {
            if (!entry->mPendingRemoval.load() && entry->mPtr)
                return false;
        }

        return true;
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mLock);

        std::size_t count = 0;

        for (const auto& entry : mObservers)
        {
            if (!entry->mPendingRemoval.load() && entry->mPtr)
                ++count;
        }

        return count;
    }

private:
    struct Entry
    {
        explicit Entry(T* ptr)
            : mPtr(ptr)
            , mPendingRemoval(false)
        {
        }

        T*               mPtr;
        std::atomic_bool mPendingRemoval;
    };

    void finishNotify()
    {
        std::lock_guard<std::mutex> lock(mLock);

        if (mNotifyDepth > 0)
            --mNotifyDepth;

        if (mNotifyDepth == 0)
            compactLocked();
    }

    // Must be called with mLock held.
    void compactLocked()
    {
        mObservers.erase(
            std::remove_if(
                mObservers.begin(),
                mObservers.end(),
                [](const std::unique_ptr<Entry>& entry) {
                    return entry->mPendingRemoval.load() ||
                           entry->mPtr == nullptr;
                }),
            mObservers.end());
    }

private:
    std::vector<std::unique_ptr<Entry> > mObservers;
    std::size_t                          mNotifyDepth = 0;
    mutable std::mutex                   mLock;
};
