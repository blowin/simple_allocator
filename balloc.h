#ifndef BALLOC_H
#define BALLOC_H

#include <cstdlib>
#include <cassert>

#include <vector>

namespace {
    typedef unsigned char chunk_t;
    typedef std::size_t sz_t;
    template<class T>
    using memory_container = std::vector<T>;
    #define NEXT_ADR(val) (*(chunk_t**)val)
    const sz_t PAGE_SIZE = 65536;

    inline sz_t align(sz_t x, sz_t a){ return ((x - 1) | (a - 1)) + 1; }

    template<sz_t page_sz = PAGE_SIZE>
    struct page_pool
    {
        inline chunk_t* get_page(){
            auto page = new chunk_t[page_sz];
            pages.push_back(page);
            return page;
        }

        ~page_pool(){
            auto sz_pool = pages.size();
            for (decltype(sz_pool) i = 0; i < sz_pool; ++i) {
                delete[] pages[i];
            }
        }
    private:
        memory_container<chunk_t*> pages;
    };

    template<class T, sz_t page_sz = PAGE_SIZE, sz_t pointer_sz = sizeof(chunk_t*)>
    struct block_pool : public page_pool<page_sz>
    {
        block_pool() : head(nullptr){
            block_sz_ = align(sizeof(T), pointer_sz);
            count_free_blocks_ = page_sz / block_sz_;
        }

        inline chunk_t* alloc(){
            //syncr
            if(head == nullptr) create_new_block();
            chunk_t* res = head;
            head = NEXT_ADR(res);
            return res;
        }

        inline void dealloc(chunk_t* mem){
            //syncr
            NEXT_ADR(mem) = head;
            head = mem;
        }
    private:
        inline void create_new_block(){
            chunk_t* page = page_pool<page_sz>::get_page();
            head = page;

            auto last = count_free_blocks_ - 1;
            for(decltype(last) i = 0; i < last; ++i){
                chunk_t* next = page + block_sz_;
                NEXT_ADR(page) = next;
                page = next;
            }
            NEXT_ADR(page) = nullptr;
        }
    private:
        sz_t block_sz_;
        sz_t count_free_blocks_;
        chunk_t* head;
    };
}

template<class Type>
struct block_allocator
{
public:
    inline static void* operator new(sz_t obj_sz){
        if(is_correct_obj_sz(obj_sz)){
            return pool.alloc();
        }else{
            return ::operator new(obj_sz);
        }
    }

   inline static void operator delete(void* mem, sz_t size){
        if(mem == nullptr) return;

        if(is_correct_obj_sz(size)){
            pool.dealloc(static_cast<chunk_t*>(mem));
        }else{
            ::operator delete(mem);
        }
    }

    template<class... Args>
    inline static Type* allocate(Args... constructor_params){
        auto res = static_cast<Type*>(block_allocator::operator new(sizeof(Type)));
        assert(res != nullptr);
        new(res)Type(constructor_params...);
        return res;
    }

    inline static void dealloc(void* mem){
        dealloc(sizeof(Type), mem);
    }

    inline static void dealloc(sz_t size, void* mem){
        if(is_correct_obj_sz(size)){
            auto obj = static_cast<Type*>(mem);
            obj->~Type();
        }
        block_allocator<Type>::operator delete(mem, size);
    }
private:
    bool static inline is_correct_obj_sz(sz_t sz){
        return sz == sizeof(Type);
    }
private:
    static block_pool<Type> pool;
};

template<class T> block_pool<T> block_allocator<T>::pool;

#endif // BALLOC_H
