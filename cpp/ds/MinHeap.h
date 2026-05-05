#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Compare compare = Compare());
    explicit MinHeap(const std::vector<T>& values, Compare compare = Compare());

    void push(const T& value);
    void push(T&& value);

    template <typename... Args>
    void emplace(Args&&... args);

    T pop();
    const T& top() const;

    bool empty() const;
    std::size_t size() const;
    void clear();

    const std::vector<T>& raw_data() const;

private:
    std::vector<T> data_;
    Compare compare_;

    void sift_up(std::size_t index);
    void sift_down(std::size_t index);
};

template <typename T, typename Compare>
MinHeap<T, Compare>::MinHeap(Compare compare)
    : data_(), compare_(std::move(compare)) {}

template <typename T, typename Compare>
MinHeap<T, Compare>::MinHeap(const std::vector<T>& values, Compare compare)
    : data_(values), compare_(std::move(compare)) {
    if (data_.empty()) {
        return;
    }

    for (std::size_t i = data_.size() / 2; i > 0; --i) {
        sift_down(i - 1);
    }
}

template <typename T, typename Compare>
void MinHeap<T, Compare>::push(const T& value) {
    data_.push_back(value);
    sift_up(data_.size() - 1);
}

template <typename T, typename Compare>
void MinHeap<T, Compare>::push(T&& value) {
    data_.push_back(std::move(value));
    sift_up(data_.size() - 1);
}

template <typename T, typename Compare>
template <typename... Args>
void MinHeap<T, Compare>::emplace(Args&&... args) {
    data_.emplace_back(std::forward<Args>(args)...);
    sift_up(data_.size() - 1);
}

template <typename T, typename Compare>
T MinHeap<T, Compare>::pop() {
    if (data_.empty()) {
        throw std::out_of_range("Cannot pop from an empty MinHeap");
    }

    T result = std::move(data_.front());
    if (data_.size() == 1) {
        data_.pop_back();
        return result;
    }

    data_.front() = std::move(data_.back());
    data_.pop_back();
    sift_down(0);
    return result;
}

template <typename T, typename Compare>
const T& MinHeap<T, Compare>::top() const {
    if (data_.empty()) {
        throw std::out_of_range("Cannot read top of an empty MinHeap");
    }
    return data_.front();
}

template <typename T, typename Compare>
bool MinHeap<T, Compare>::empty() const {
    return data_.empty();
}

template <typename T, typename Compare>
std::size_t MinHeap<T, Compare>::size() const {
    return data_.size();
}

template <typename T, typename Compare>
void MinHeap<T, Compare>::clear() {
    data_.clear();
}

template <typename T, typename Compare>
const std::vector<T>& MinHeap<T, Compare>::raw_data() const {
    return data_;
}

template <typename T, typename Compare>
void MinHeap<T, Compare>::sift_up(std::size_t index) {
    while (index > 0) {
        const std::size_t parent = (index - 1) / 2;
        if (!compare_(data_[index], data_[parent])) {
            break;
        }
        std::swap(data_[index], data_[parent]);
        index = parent;
    }
}

template <typename T, typename Compare>
void MinHeap<T, Compare>::sift_down(std::size_t index) {
    while (true) {
        const std::size_t left = 2 * index + 1;
        const std::size_t right = left + 1;
        std::size_t smallest = index;

        if (left < data_.size() && compare_(data_[left], data_[smallest])) {
            smallest = left;
        }
        if (right < data_.size() && compare_(data_[right], data_[smallest])) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }

        std::swap(data_[index], data_[smallest]);
        index = smallest;
    }
}
