#ifndef CONTAINER1_H
#define CONTAINER1_H

#include <iostream>
#include <stdexcept>
#include <memory>     
#include <algorithm>  
#include <iterator>   

template<typename T, typename Allocator = std::allocator<T>>
class ConsistentContainer
{
private:
    
    using AllocTraits = std::allocator_traits<Allocator>; // без template было бы std::allocator_traits<std::allocator<T>>

public:
    
    using value_type        = T;
    using allocator_type    = Allocator;
    using size_type         = std::size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type&;
    using const_reference   = const value_type&;
    using pointer           = typename AllocTraits::pointer;
    using const_pointer     = typename AllocTraits::const_pointer;
    using iterator          = pointer;          // простой указатель как итератор
    using const_iterator    = const_pointer;

private:
    pointer     data_;          
    size_type   size_;          
    size_type   capacity_;      
    double      growth_;        
    Allocator   alloc_;         // экземпляр аллокатора

    
    void grow()
    {
        size_type new_capacity;
        if (capacity_ - size_ <= 1)
            new_capacity = static_cast<size_type>(capacity_ * 1.1);
        else
            new_capacity = static_cast<size_type>(capacity_ * growth_);
        if (new_capacity <= capacity_)
            new_capacity = capacity_ + 1;  

        // Выделяем новую память через аллокатор
        pointer new_data = AllocTraits::allocate(alloc_, new_capacity);

        // Переносим существующие элементы в новую память (конструируем копии)
        for (size_type i = 0; i < size_; ++i)
            AllocTraits::construct(alloc_, new_data + i, data_[i]);

        // Уничтожаем старые элементы и освобождаем старую память
        for (size_type i = 0; i < size_; ++i)
            AllocTraits::destroy(alloc_, data_ + i);
        AllocTraits::deallocate(alloc_, data_, capacity_);

        data_ = new_data;
        capacity_ = new_capacity;
    }

    void shrink_to_fit_impl()
    {
        if (size_ == capacity_ || size_ == 0)
            return;

        size_type new_capacity = size_ + 1;
        if (new_capacity < 5) new_capacity = 5;

        pointer new_data = AllocTraits::allocate(alloc_, new_capacity);

        // Переносим элементы
        for (size_type i = 0; i < size_; ++i)
            AllocTraits::construct(alloc_, new_data + i, data_[i]);

        // Уничтожаем старые элементы и освобождаем старую память
        for (size_type i = 0; i < size_; ++i)
            AllocTraits::destroy(alloc_, data_ + i);
        AllocTraits::deallocate(alloc_, data_, capacity_);

        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
   
    explicit ConsistentContainer(size_type base_capacity = 5,
                                 double growth_factor = 1.6,
                                 const Allocator& alloc = Allocator())
        : data_(nullptr), size_(0), capacity_(base_capacity), growth_(growth_factor), alloc_(alloc)
    {
        if (base_capacity > 0)
            data_ = AllocTraits::allocate(alloc_, base_capacity);
    }


    ConsistentContainer(const ConsistentContainer& other)
        : size_(other.size_),
          capacity_(other.size_),  
          growth_(other.growth_),
          alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_))
    {
        if (capacity_ > 0)
        {
            data_ = AllocTraits::allocate(alloc_, capacity_);
            // Используем uninitialized_copy для безопасного копирования
            std::uninitialized_copy(other.data_, other.data_ + size_, data_);
        }
        else
        {
            data_ = nullptr;
        }
    }


    // конструктор перемещения
    ConsistentContainer(ConsistentContainer&& other) noexcept
        : data_(other.data_),
          size_(other.size_),
          capacity_(other.capacity_),
          growth_(other.growth_),
          alloc_(std::move(other.alloc_))
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    
    ~ConsistentContainer()
    {
        clear();
        if (data_)
            AllocTraits::deallocate(alloc_, data_, capacity_);
    }

    ConsistentContainer& operator=(const ConsistentContainer& other)
    {
        if (this != &other)
        {
            if (AllocTraits::propagate_on_container_copy_assignment::value)
                alloc_ = other.alloc_;

            ConsistentContainer tmp(other);
            swap(tmp);
        }
        return *this;
    }

    ConsistentContainer& operator=(ConsistentContainer&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            if (data_)
                AllocTraits::deallocate(alloc_, data_, capacity_);

            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            growth_ = other.growth_;

            if (AllocTraits::propagate_on_container_move_assignment::value)
                alloc_ = std::move(other.alloc_);
            else
                alloc_ = other.alloc_;

            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void swap(ConsistentContainer& other) noexcept
    {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
        swap(growth_, other.growth_);
        if (AllocTraits::propagate_on_container_swap::value)
            swap(alloc_, other.alloc_);
    }

    void clear() noexcept
    {
        for (size_type i = 0; i < size_; ++i)
            AllocTraits::destroy(alloc_, data_ + i);
        size_ = 0;
    }

    void push_back(const T& value)
    {
        if (size_ >= capacity_)
            grow();
        AllocTraits::construct(alloc_, data_ + size_, value);
        ++size_;
    }

    void push_front(const T& value)
    {
        insert(0, value);
    }

    void insert(size_type index, const T& value)
    {
        if (index > size_)
            throw std::out_of_range("ConsistentContainer::insert: index out of range");

        if (size_ >= capacity_)
            grow();

        // Сдвигаем элементы вправо, начиная с конца
        for (size_type i = size_; i > index; --i)
        {
            // Перемещаем существующий элемент на новое место
            AllocTraits::construct(alloc_, data_ + i, std::move(data_[i-1]));
            AllocTraits::destroy(alloc_, data_ + i - 1);
        }

        // Конструируем новый элемент на vacated месте
        AllocTraits::construct(alloc_, data_ + index, value);
        ++size_;
    }
    
    void pop_back()
    {
        if (size_ == 0)
            throw std::out_of_range("pop_back on empty container");
        --size_;
        AllocTraits::destroy(alloc_, data_ + size_);
    }

    void pop_front()
    {
        if (size_ == 0)
            throw std::out_of_range("pop_front on empty container");
        AllocTraits::destroy(alloc_, data_);
        for (size_type i = 1; i < size_; ++i)
        {
            AllocTraits::construct(alloc_, data_ + i - 1, std::move(data_[i]));
            AllocTraits::destroy(alloc_, data_ + i);
        }
        --size_;
    }

    reference at(size_type index)
    {
        if (index >= size_)
            throw std::out_of_range("at: index out of range");
        return data_[index];
    }

    const_reference at(size_type index) const
    {
        if (index >= size_)
            throw std::out_of_range("at: index out of range");
        return data_[index];
    }

    reference operator[](size_type index) { return data_[index]; }
    const_reference operator[](size_type index) const { return data_[index]; }

    iterator begin() { return data_; }
    iterator end()   { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end()   const { return data_ + size_; }
    const_iterator cbegin() const { return data_; }
    const_iterator cend()   const { return data_ + size_; }

    // Размер и ёмкость
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    void shrink_to_fit()
    {
        shrink_to_fit_impl();
    }

    // Получить аллокатор
    allocator_type get_allocator() const { return alloc_; }
};

template<typename T, typename Alloc>
bool operator==(const ConsistentContainer<T, Alloc>& lhs,
                const ConsistentContainer<T, Alloc>& rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    for (std::size_t i = 0; i < lhs.size(); ++i)
        if (lhs[i] != rhs[i])
            return false;
    return true;
}

template<typename T, typename Alloc>
bool operator!=(const ConsistentContainer<T, Alloc>& lhs,
                const ConsistentContainer<T, Alloc>& rhs)
{
    return !(lhs == rhs);
}

#endif 