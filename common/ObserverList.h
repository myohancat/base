#pragma once

#include <mutex>
#include <condition_variable>
#include <array>
#include <cstddef>
#include <bitset>
#include <utility>

template <typename T, std::size_t MaxObservers>
class RawObserverList
{
public:
    enum class RemoveResult
    {
        Removed,
        NotFound,
        NullObserver
    };

    RawObserverList() = default;
    ~RawObserverList() = default;

    RawObserverList(const RawObserverList&) = delete;
    RawObserverList& operator=(const RawObserverList&) = delete;
    RawObserverList(RawObserverList&&) = delete;
    RawObserverList& operator=(RawObserverList&&) = delete;

    bool add(T* observer)
    {
        if (observer == nullptr)
            return false;

        std::lock_guard<std::mutex> lock(mLock);

        std::size_t freeIndex = MaxObservers;

        for (std::size_t i = 0; i < MaxObservers; ++i)
        {
            if (!mOccupiedBits.test(i))
            {
                if (freeIndex == MaxObservers)
                    freeIndex = i;

                continue;
            }

            const Entry& entry = mEntries[i];

            if (entry.observer == observer)
                return false;
        }

        if (freeIndex == MaxObservers)
            return false;

        ++mGeneration;

        Entry& target = mEntries[freeIndex];
        target.observer = observer;
        target.activeCalls = 0;
        target.generation = mGeneration;
        target.removed = false;
        target.waitingRemoval = false;

        mOccupiedBits.set(freeIndex);

        return true;
    }

    bool add(T& observer)
    {
        return add(&observer);
    }

    RemoveResult remove(T* observer)
    {
        if (observer == nullptr)
            return RemoveResult::NullObserver;

        std::lock_guard<std::mutex> lock(mLock);

        const std::size_t index = findActiveEntryIndexLocked(observer);
        if (index == MaxObservers)
            return RemoveResult::NotFound;

        Entry& entry = mEntries[index];
        entry.removed = true;

        if (entry.activeCalls == 0 && !entry.waitingRemoval)
            resetEntryLocked(index);

        return RemoveResult::Removed;
    }

    RemoveResult remove(T& observer)
    {
        return remove(&observer);
    }

    // IMPORTANT:
    // Do not use this function in callback.
    // It's cause deadlock. Please use remove in callback
    RemoveResult removeAndWait(T* observer)
    {
        if (observer == nullptr)
            return RemoveResult::NullObserver;

        std::unique_lock<std::mutex> lock(mLock);

        const std::size_t index = findActiveEntryIndexLocked(observer);
        if (index == MaxObservers)
            return RemoveResult::NotFound;

        Entry& entry = mEntries[index];
        entry.removed = true;

        if (entry.activeCalls == 0)
        {
            resetEntryLocked(index);
            return RemoveResult::Removed;
        }

        entry.waitingRemoval = true;

        mChanged.wait(lock, [&entry]() {
            return entry.activeCalls == 0;
        });

        resetEntryLocked(index);
        return RemoveResult::Removed;
    }

    RemoveResult removeAndWait(T& observer)
    {
        return removeAndWait(&observer);
    }

    template <typename Func>
    void notify(Func&& action) noexcept
    {
        static_assert(
                noexcept(std::declval<Func&>()(std::declval<T*>())),
                "RawObserverList::notify() callback must be noexcept"
        );

        std::size_t startGeneration = 0;
        std::bitset<MaxObservers> occupiedSnapshot;

        {
            std::lock_guard<std::mutex> lock(mLock);
            startGeneration = mGeneration;
            occupiedSnapshot = mOccupiedBits;
        }

        for (std::size_t i = 0; i < MaxObservers; ++i)
        {
            if (!occupiedSnapshot.test(i))
                continue;

            T* target = acquireForNotify(i, startGeneration);
            if (target == nullptr)
                continue;

            action(target);

            releaseAfterNotify(i);
        }

        releaseRemoved();
    }

    bool contains(T* observer) const
    {
        if (observer == nullptr)
            return false;

        std::lock_guard<std::mutex> lock(mLock);

        for (std::size_t i = 0; i < MaxObservers; ++i)
        {
            if (!mOccupiedBits.test(i))
                continue;

            const Entry& entry = mEntries[i];

            if (entry.observer == observer && !entry.removed)
                return true;
        }

        return false;
    }

    bool contains(T& observer) const
    {
        return contains(&observer);
    }

private:
    struct Entry
    {
        T* observer = nullptr;
        std::size_t activeCalls = 0;
        std::size_t generation = 0;
        bool removed = false;
        bool waitingRemoval = false;
    };

    std::size_t findActiveEntryIndexLocked(T* observer) const
    {
        for (std::size_t i = 0; i < MaxObservers; ++i)
        {
            if (!mOccupiedBits.test(i))
                continue;

            const Entry& entry = mEntries[i];

            if (entry.observer == observer && !entry.removed)
                return i;
        }

        return MaxObservers;
    }

    T* acquireForNotify(std::size_t index, std::size_t startGeneration)
    {
        std::lock_guard<std::mutex> lock(mLock);

        if (!mOccupiedBits.test(index))
            return nullptr;

        Entry& entry = mEntries[index];

        if (entry.removed)
            return nullptr;

        if (entry.generation > startGeneration)
            return nullptr;

        ++entry.activeCalls;
        return entry.observer;
    }

    void releaseAfterNotify(std::size_t index)
    {
        bool notifyWaiter = false;

        {
            std::lock_guard<std::mutex> lock(mLock);

            if (!mOccupiedBits.test(index))
                return;

            Entry& entry = mEntries[index];

            if (entry.activeCalls > 0)
                --entry.activeCalls;

            if (entry.activeCalls == 0)
            {
                notifyWaiter = entry.waitingRemoval;

                if (entry.removed && !entry.waitingRemoval)
                    resetEntryLocked(index);
            }
        }

        if (notifyWaiter)
            mChanged.notify_all();
    }

    void releaseRemoved()
    {
        std::lock_guard<std::mutex> lock(mLock);

        for (std::size_t i = 0; i < MaxObservers; ++i)
        {
            if (!mOccupiedBits.test(i))
                continue;

            Entry& entry = mEntries[i];

            if (entry.removed && !entry.waitingRemoval && entry.activeCalls == 0)
                resetEntryLocked(i);
        }
    }

    void resetEntryLocked(std::size_t index)
    {
        Entry& entry = mEntries[index];

        entry.observer = nullptr;
        entry.activeCalls = 0;
        entry.generation = 0;
        entry.removed = false;
        entry.waitingRemoval = false;

        mOccupiedBits.reset(index);
    }

private:
    mutable std::mutex mLock;
    std::condition_variable mChanged;

    std::array<Entry, MaxObservers> mEntries{};
    std::bitset<MaxObservers> mOccupiedBits{};
    std::size_t mGeneration = 0;
};
