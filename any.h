#ifndef ANY_H
#define ANY_H
#include <memory>

/* Any类型：表示可以接收任意数据的类型 */
class Any
{
public:
    /* 使用默认构造函数能够减少错误 */
    Any() = default;

    /* 使用默认析构函数能够减少错误 */
    ~Any() = default;

    /* 因为使用了unique_ptr禁用左值引用的拷贝构造与赋值，在此禁用 */
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    /* 因为使用了unique_ptr保留了右值引用的赋值，在此默认使用 */
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    /*
        * 这个构造函数可以让Any类型接收任意其它的数据
        * 构造Any对象时就让base_基类指针指向派生类对象发生多态，并传入数据
    */
    template <typename T> // T:int    Derive<int>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {
    }

    /* 这个方法能把Any对象里面存储的data数据提取出来 */
    template <typename T>
    T cast_()
    {
        /* 通过base_找到他所指向的Derive对象，从中提取data_成员变量 */
        /* 基类指针需要转换成派生类指针，RTTI类型识别（强制类型转换） */
        /* get函数：返回智能指针中保存的裸指针。考虑到有些函数的参数需要内置的裸指针，所以引入该函数。 */
        Derive<T>* pd = dynamic_cast<Derive<T> *>(base_.get());
        if (pd == nullptr)
        {
            throw "type is unmatch!";
        }
        return pd->data_;
    }

private:
    /* 基类类型 */
    class Base
    {
    public:
        virtual ~Base() = default; // 虚析构，使用默认实现
    };

    /* 派生类类型，保存任意类型的数据，通过多态访问被保存的数据 */
    template <typename T>
    class Derive : public Base
    {
    public:
        /* 将任意类型的数据保存在派生类中，并且发生多态时，需要传入这个任意类型的数据 */
        Derive(T data) : data_(data)
        {
        }
        T data_; // 保存了任意的其他类型
    };

private:
    /*
     * 定义一个基类指针用于多态访问任意类型数据
     * unique_ptr把左值引用的拷贝构造函数与赋值删除了，保留了右值引用的拷贝构造与赋值
     */
    std::unique_ptr<Base> base_;
};


#endif  // !ANY_H
