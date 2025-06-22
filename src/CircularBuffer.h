#pragma once
#include <vector>
template <typename T>
class CircularBuffer
{
public:
    CircularBuffer(size_t capacity)
        : capacity_(capacity), buffer_(capacity), head_(0), size_(0)
    {
    }

    // Ajoute des données à la fin du buffer (concaténation)
    void push_back(const std::vector<T> &data)
    {
        assert(data.size() <= capacity_);
        for (auto &v : data)
        {
            buffer_[head_] = v;
            head_ = (head_ + 1) % capacity_;
            if (size_ < capacity_)
                size_++;
        }
    }
    void push_back(const T& d){
        buffer_[head_] = d;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_)
            size_++;
    }

    void reset(){
        head_= size_=  0;
    }

    // Accès en lecture au buffer dans l’ordre chronologique (ancien vers récent)
    // Remplit out avec le contenu actuel, out doit avoir au moins size_ éléments
    void get_ordered(std::vector<T> &out) const
    {
        out.resize(size_);
        size_t start = (head_ + capacity_ - size_) % capacity_;
        for (size_t i = 0; i < size_; ++i)
        {
            out[i] = buffer_[(start + i) % capacity_];
        }
    }
    std::vector<T> get_ordered(){
        std::vector<T> out;
        get_ordered(out);
        return out;
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    size_t head_; // position d'écriture
    size_t size_; // nb éléments valides
};