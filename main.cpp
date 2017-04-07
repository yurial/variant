#include <memory>
#include <iostream>

struct A
    {
    int x;
    A() { std::cerr << "A()" << std::endl; }
    A(int v): x(v) { std::cerr << "A(int)" << std::endl; }
    ~A() { std::cerr << "~A()" << std::endl; }
    };

struct B
    {
    double x;
    B() { std::cerr << "B()" << std::endl; }
    B(double v): x(v) { std::cerr << "B(double)" << std::endl; }
    ~B() { std::cerr << "~B()" << std::endl; }
    };

#define APPLY_FOR_VARIANTS(M) \
    M(type_A, A) \
    M(type_B, B)
class Variant
    {
    public:
        enum Type
        {
        type_empty,
        #define M(NAME,TYPE) NAME,
        APPLY_FOR_VARIANTS(M)
        #undef M
        };

    template <typename Func, typename ... Args>
    void call(Func func, Args&& ... args)
        {
        call_ptr( [&func,&args...](auto& ptr){ func( *ptr, std::forward<Args>( args )... ); } );
        }

    template <typename ... Args>
    void init(Type type, Args&& ... args)
        {
        m_type = type;
        call_ptr(
            [&args...](auto& ptr)
                {
                using T = typename std::remove_reference<decltype(ptr)>::type::element_type;
                ptr = std::move( Ptr<T>( new T{ std::forward<Args>( args )... }, deleter<T> ) );
                }
            );
        }

    void reset()
        {
        m_ptr.reset();
        m_type = type_empty;
        }

    protected:
    template <typename T>
    static void deleter(T* ptr) { delete ptr; }
    template <typename T>
    using Deleter = decltype(deleter<T>)*;
    template <typename T>
    using Ptr = std::unique_ptr<T,Deleter<T>>;

    struct Null
        {
        Null() { std::cerr << "Null()" << std::endl; }
        ~Null() { std::cerr << "~Null()" << std::endl; }
        };

    Type        m_type = type_empty;
    Ptr<Null>   m_ptr = Ptr<Null>( nullptr, deleter<Null> );

    template <typename Func, typename ... Args>
    void call_ptr(Func func, Args&& ... args)
        {
        switch ( m_type )
            {
            case type_empty:
                throw std::logic_error( "unitialized type" );
            #define M(NAME,TYPE) \
            case NAME: \
                func(reinterpret_cast<Ptr<TYPE>&>(m_ptr), std::forward<Args>( args )...  ); \
                return;
            APPLY_FOR_VARIANTS(M)
            #undef M
            }
        }

    };
#undef APPLY_FOR_VARIANTS

template <typename T>
void out(const T& val)
    {
    std::cout << val.x << std::endl;
    }

int main()
    {
    Variant::Type type = Variant::type_B;
    Variant v;
    v.init( type );
    v.call( [](auto& val){ val.x = 1.2; } );
    v.call( [](auto& val){ out( val ); } );
    v.reset();
    }
