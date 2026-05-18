/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <cassert>

// ============================================================================
// 1. RawObserverList
//
// - raw pointer 기반 non-owning observer list
// - notify()는 lock을 잡은 상태에서 callback을 호출한다.
// - 따라서 removeObserver()가 반환되면 진행 중인 notify callback과 겹치지 않는다.
// - callback 안에서 addObserver/removeObserver/clear 호출은 금지된다.
// - 같은 스레드에서 callback 중 add/remove/clear가 호출되면 debug에서는 assert,
//   release에서는 무시하고 return한다.
//
// 용도:
// - observer가 stack object이거나 shared_ptr로 관리할 수 없는 레거시 객체
// - B::~B() { removeListener(this); } 패턴을 단순하게 유지하고 싶은 경우
//
// 주의:
// - callback 안에서 이 ObserverList를 수정하지 말 것
// - callback 안에서 오래 걸리는 작업이나 다른 heavy lock 획득은 피하는 것이 좋음
// ============================================================================
template <typename T>
class RawObserverList
{
public:
    RawObserverList() = default;
    ~RawObserverList() = default;

    RawObserverList(const RawObserverList&) = delete;
    RawObserverList& operator=(const RawObserverList&) = delete;

    void addObserver(T* observer)
    {
        if (!observer)
            return;

        if (isNotifyingThisThread())
        {
            assert(false &&
                   "RawObserverList::addObserver() must not be called from notify callback on the same thread");
            return;
        }

        std::lock_guard<std::mutex> lock(mLock);

        if (std::find(mObservers.begin(), mObservers.end(), observer) == mObservers.end())
            mObservers.push_back(observer);
    }

    void removeObserver(T* observer)
    {
        if (!observer)
            return;

        if (isNotifyingThisThread())
        {
            assert(false &&
                   "RawObserverList::removeObserver() must not be called from notify callback on the same thread");
            return;
        }

        std::lock_guard<std::mutex> lock(mLock);

        mObservers.erase(
            std::remove(mObservers.begin(), mObservers.end(), observer),
            mObservers.end());
    }

    void clear()
    {
        if (isNotifyingThisThread())
        {
            assert(false &&
                   "RawObserverList::clear() must not be called from notify callback on the same thread");
            return;
        }

        std::lock_guard<std::mutex> lock(mLock);
        mObservers.clear();
    }

    template <typename Func>
    void notify(Func&& action)
    {
        std::lock_guard<std::mutex> lock(mLock);

        NotifyScope scope(this);

        for (T* observer : mObservers)
        {
            if (observer)
                action(observer);
        }
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mObservers.empty();
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mLock);
        return mObservers.size();
    }

    bool contains(T* observer) const
    {
        if (!observer)
            return false;

        std::lock_guard<std::mutex> lock(mLock);

        return std::find(mObservers.begin(), mObservers.end(), observer) != mObservers.end();
    }

private:
    bool isNotifyingThisThread() const
    {
        return tCurrentNotifyingList == this;
    }

    struct NotifyScope
    {
        explicit NotifyScope(const RawObserverList* list)
            : mPrev(tCurrentNotifyingList)
        {
            tCurrentNotifyingList = list;
        }

        ~NotifyScope()
        {
            tCurrentNotifyingList = mPrev;
        }

        NotifyScope(const NotifyScope&) = delete;
        NotifyScope& operator=(const NotifyScope&) = delete;

        const RawObserverList* mPrev;
    };

private:
    std::vector<T*>    mObservers;
    mutable std::mutex mLock;

    static thread_local const RawObserverList* tCurrentNotifyingList;
};

template <typename T>
thread_local const RawObserverList<T>* RawObserverList<T>::tCurrentNotifyingList = nullptr;


// ============================================================================
// 2. ObserverList
//
// - weak_ptr 기반 non-owning observer list
// - ObserverList는 observer를 소유하지 않는다.
// - notify() 시점에 weak_ptr을 shared_ptr로 승격해서 callback 동안 생명주기를 보장한다.
// - callback 호출 중에는 lock을 잡지 않는다.
// - callback 안에서 addObserver/removeObserver 호출 가능
//
// 정책:
// - notify 시점에 targets로 복사된 observer는 removeObserver()가 동시에 호출되어도
//   이번 notify에서는 호출될 수 있다.
// - 단, shared_ptr로 생명주기를 잡고 있으므로 use-after-free는 발생하지 않는다.
//
// 용도:
// - observer가 shared_ptr로 관리되는 구조
// - 싱글톤 A가 observer를 소유하지 않아야 하는 구조
// ============================================================================
template <typename T>
class ObserverList
{
public:
    ObserverList() = default;
    ~ObserverList() = default;

    ObserverList(const ObserverList&) = delete;
    ObserverList& operator=(const ObserverList&) = delete;

    void addObserver(const std::shared_ptr<T>& observer)
    {
        if (!observer)
            return;

        std::lock_guard<std::mutex> lock(mLock);

        compactExpiredLocked();

        for (const auto& weak : mObservers)
        {
            if (auto sp = weak.lock())
            {
                if (sp.get() == observer.get())
                    return;
            }
        }

        mObservers.emplace_back(observer);
    }

    void removeObserver(const std::shared_ptr<T>& observer)
    {
        if (!observer)
            return;

        removeObserver(observer.get());
    }

    void removeObserver(T* observer)
    {
        if (!observer)
            return;

        std::lock_guard<std::mutex> lock(mLock);

        mObservers.erase(
            std::remove_if(
                mObservers.begin(),
                mObservers.end(),
                [observer](const std::weak_ptr<T>& weak) {
                    auto sp = weak.lock();
                    return !sp || sp.get() == observer;
                }),
            mObservers.end());
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mLock);
        mObservers.clear();
    }

    template <typename Func>
    void notify(Func&& action)
    {
        std::vector<std::shared_ptr<T>> targets;

        {
            std::lock_guard<std::mutex> lock(mLock);

            targets.reserve(mObservers.size());

            for (auto it = mObservers.begin(); it != mObservers.end(); )
            {
                if (auto sp = it->lock())
                {
                    targets.push_back(std::move(sp));
                    ++it;
                }
                else
                {
                    it = mObservers.erase(it);
                }
            }
        }

        for (const auto& observer : targets)
            action(observer.get());
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mLock);

        for (const auto& weak : mObservers)
        {
            if (!weak.expired())
                return false;
        }

        return true;
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mLock);

        std::size_t count = 0;

        for (const auto& weak : mObservers)
        {
            if (!weak.expired())
                ++count;
        }

        return count;
    }

    bool contains(T* observer) const
    {
        if (!observer)
            return false;

        std::lock_guard<std::mutex> lock(mLock);

        for (const auto& weak : mObservers)
        {
            if (auto sp = weak.lock())
            {
                if (sp.get() == observer)
                    return true;
            }
        }

        return false;
    }

    bool contains(const std::shared_ptr<T>& observer) const
    {
        if (!observer)
            return false;

        return contains(observer.get());
    }

private:
    void compactExpiredLocked()
    {
        mObservers.erase(
            std::remove_if(
                mObservers.begin(),
                mObservers.end(),
                [](const std::weak_ptr<T>& weak) {
                    return weak.expired();
                }),
            mObservers.end());
    }

private:
    std::vector<std::weak_ptr<T>> mObservers;
    mutable std::mutex           mLock;
};
