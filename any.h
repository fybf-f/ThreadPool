#ifndef ANY_H
#define ANY_H
#include <memory>

/* Any���ͣ���ʾ���Խ����������ݵ����� */
class Any
{
public:
    /* ʹ��Ĭ�Ϲ��캯���ܹ����ٴ��� */
    Any() = default;

    /* ʹ��Ĭ�����������ܹ����ٴ��� */
    ~Any() = default;

    /* ��Ϊʹ����unique_ptr������ֵ���õĿ��������븳ֵ���ڴ˽��� */
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    /* ��Ϊʹ����unique_ptr��������ֵ���õĸ�ֵ���ڴ�Ĭ��ʹ�� */
    Any(Any&&) = default;
    Any& operator=(Any&&) = default;

    /*
        * ������캯��������Any���ͽ�����������������
        * ����Any����ʱ����base_����ָ��ָ���������������̬������������
    */
    template <typename T> // T:int    Derive<int>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {
    }

    /* ��������ܰ�Any��������洢��data������ȡ���� */
    template <typename T>
    T cast_()
    {
        /* ͨ��base_�ҵ�����ָ���Derive���󣬴�����ȡdata_��Ա���� */
        /* ����ָ����Ҫת����������ָ�룬RTTI����ʶ��ǿ������ת���� */
        /* get��������������ָ���б������ָ�롣���ǵ���Щ�����Ĳ�����Ҫ���õ���ָ�룬��������ú����� */
        Derive<T>* pd = dynamic_cast<Derive<T> *>(base_.get());
        if (pd == nullptr)
        {
            throw "type is unmatch!";
        }
        return pd->data_;
    }

private:
    /* �������� */
    class Base
    {
    public:
        virtual ~Base() = default; // ��������ʹ��Ĭ��ʵ��
    };

    /* ���������ͣ������������͵����ݣ�ͨ����̬���ʱ���������� */
    template <typename T>
    class Derive : public Base
    {
    public:
        /* ���������͵����ݱ������������У����ҷ�����̬ʱ����Ҫ��������������͵����� */
        Derive(T data) : data_(data)
        {
        }
        T data_; // �������������������
    };

private:
    /*
     * ����һ������ָ�����ڶ�̬����������������
     * unique_ptr����ֵ���õĿ������캯���븳ֵɾ���ˣ���������ֵ���õĿ��������븳ֵ
     */
    std::unique_ptr<Base> base_;
};


#endif  // !ANY_H
