#pragma once
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory
{
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity)), capacity_(capacity)
    {
    }

    ~RawMemory()
    {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory &) = delete;
    RawMemory &operator=(const RawMemory &rhs) = delete;
    RawMemory(RawMemory &&other) noexcept
        : buffer_(std::exchange(other.buffer_, nullptr)), 
        capacity_(std::exchange(other.capacity_, 0))
    {
    }

    RawMemory &operator=(RawMemory &&rhs) noexcept
    {
        if (this != &rhs)
        {
            Deallocate(buffer_);
            buffer_ = std::exchange(rhs.buffer_, nullptr);
            capacity_ = std::exchange(rhs.capacity_, 0);
        }
        return *this;
    }

    T *operator+(size_t offset) noexcept
    {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T *operator+(size_t offset) const noexcept
    {
        return const_cast<RawMemory &>(*this) + offset;
    }

    const T &operator[](size_t index) const noexcept
    {
        return const_cast<RawMemory &>(*this)[index];
    }

    T &operator[](size_t index) noexcept
    {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory &other) noexcept
    {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T *GetAddress() const noexcept
    {
        return buffer_;
    }

    T *GetAddress() noexcept
    {
        return buffer_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T *Allocate(size_t n)
    {
        return n != 0 ? static_cast<T *>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T *buf) noexcept
    {
        operator delete(buf);
    }

    T *buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector
{
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }
    Vector() = default;
    Vector(const Vector &other)
        : data_(other.size_), size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.Size(), data_.GetAddress());
    }
    explicit Vector(size_t size)
        : data_(size), size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }
    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            data_.Swap(rhs.data_);
            size_ = std::exchange(rhs.size_, 0);
        }
        return *this;
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector tmp(rhs);
                Swap(tmp);
            } else {
                size_t i = 0;
                for (; i < size_ && i < rhs.size_; ++i) {
                    data_[i] = rhs.data_[i];
                }
                if (i < rhs.size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + i, rhs.size_ - i, data_.GetAddress() + i);
                } else {
                    std::destroy_n(data_.GetAddress() + i, size_ - i);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity <= data_.Capacity())
        {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
        }
        else
        {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
        }
        data_.Swap(new_data);
    }
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }
    void Resize(size_t new_size) {
        if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        } else if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }
    
    void PushBack(const T& value) {
        if (size_ == data_.Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_buffer = new_data.GetAddress();
    
            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_buffer);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_buffer);
            }
    
            
            new (new_buffer + size_) T(value);
    
            
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(value);
        }
    
        ++size_;
    }
    
    void PushBack(T&& value) {
        if (size_ == data_.Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_buffer = new_data.GetAddress();
    
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_buffer);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_buffer);
            }
    
            
            new (new_buffer + size_) T(std::move(value));
    
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::move(value));
        }
    
        ++size_;
    }
    
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_buffer = new_data.GetAddress();

            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_buffer);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_buffer);
            }

            
            T* new_elem = new (new_buffer + size_) T(std::forward<Args>(args)...);

            
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);

            ++size_;
            return *new_elem;
        } else {
            T* new_elem = new (data_ + size_) T(std::forward<Args>(args)...);
            ++size_;
            return *new_elem;
        }
    }
    void PopBack() noexcept {
        assert(size_ > 0);
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

    
    
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
        size_t index = pos - begin(); 
        if (size_ == data_.Capacity()) {
            if(pos == end()){
                EmplaceBack(std::forward<Args>(args)...);
                return begin() + index;
            }
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_buffer = new_data.GetAddress();

            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), index, new_buffer);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), index, new_buffer);
            }

            
            T* new_elem = new (new_buffer + index) T(std::forward<Args>(args)...);

            
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + index, size_ - index, new_buffer + index + 1);
            } else {
                std::uninitialized_copy_n(data_.GetAddress() + index, size_ - index, new_buffer + index + 1);
            }

            
            
            DestroyN(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            return new_elem;
        } else {
            if(pos == end()){
                EmplaceBack(std::forward<Args>(args)...);
                return begin() + index;
            }
            T tmp_copy(std::forward<Args>(args)...);
            if (index < size_) {
                new (data_ + size_) T(std::move(data_[size_ - 1]));
                std::move_backward(data_ + index, data_ + size_ - 1, data_ + size_);
            }
            
            data_[index] = std::move(tmp_copy);
            ++size_;
            return data_ + index;
        }

        
        return begin() + index; 
    }

    iterator Erase(const_iterator pos){
        size_t index = pos - begin(); 

        
        std::destroy_at(data_ + index);

        
        if (index < size_ - 1) {
            std::move(data_ + index + 1, data_ + size_, data_ + index);
        }

        --size_;
        return begin() + index;
    }
    iterator Insert(const_iterator pos, const T& value){
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos, std::move(value));
    }

    size_t Size() const noexcept
    {
        return size_;
    }

    size_t Capacity() const noexcept
    {
        return data_.Capacity();
    }

    const T &operator[](size_t index) const noexcept
    {
        return const_cast<Vector &>(*this)[index];
    }

    T &operator[](size_t index) noexcept
    {
        assert(index < size_);
        return data_[index];
    }
    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    static void DestroyN(T *buf, size_t n) noexcept
    {
        for (size_t i = 0; i != n; ++i)
        {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T *buf, const T &elem)
    {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T *buf) noexcept
    {
        buf->~T();
    }
    RawMemory<T> data_;
    size_t size_ = 0;
};