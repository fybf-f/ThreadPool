#ifndef TASK_H
#define TASK_H
#include "any.h"
#include "semaphore.h"
#include <memory>

class Result;
/* ���������� */
class Task
{
public:
    /* ���캯�� */
    Task();

    /* Ĭ���������� */
    ~Task() = default;

    /* ����setResult���� */
    void exec();

    /* �������񷵻�ֵ����run�����л�� */
    void setResult(Result* res);

    /* �û������Զ��������������ͣ���Task����̳У���дrun������ʵ���Զ���������C++��̬�� */
    virtual Any run() = 0;

private:
    Result* result_; // �˴�����ʹ������ָ�룬�����������ָ����໥���ã�������Դ��Զ�ò����ͷţ�Result������������ڱ�Task��
};



/* ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ����Result�������ִ����ɱ��ύ������̵߳ķ���ֵ */
class Result
{
public:
    /* ���캯����Ĭ�����������ύ��ִ�У�����ֵ��Ч */
    Result(std::shared_ptr<Task> task, bool isValid = true);

    /* ����������ʹ��ֻ��ָ�룬Ĭ���������� */
    ~Result() = default;

    /* ����һ��setVal��������ȡ����ִ����ķ���ֵ */
    void setVal(Any any);

    /* ��������û��������������ȡtask�ķ���ֵ */
    Any get();

private:
    Any any_;                    // �洢����ķ���ֵ
    Semaphore sem_;              // �߳�ͨ�ŵ��ź�������֪�����Ƿ�ִ����
    std::shared_ptr<Task> task_; // ָ���Ӧ��ȡ����ֵ���������
    std::atomic_bool isValid_;   // �������ķ���ֵ�Ƿ���Ч
};

#endif // !TASK_H
