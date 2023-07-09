#ifndef THREAD_H
#define THREAD_H
#include <functional>

/* �߳����� */
class Thread
{
public:
    /* �̵߳ĺ����������� */
    using ThreadFunc = std::function<void(int)>;

    /* �̹߳��죬��Ĭ�ϲ����������̶߳���ʱ��Ҫ����һ���̺߳��� */
    Thread(ThreadFunc func);

    /* �߳����� */
    ~Thread();

    /* �����߳� */
    void start();

    /* ��ȡ�߳�id */
    int getId() const;

private:
    ThreadFunc func_;        // �����������͵ĳ�Ա���ڳ�ʼ�������б�ʹ��std::bind()������
    static int generateId_;  // ��ʼΪ0�����ڳ�ʼ��threadId_�߳�id��ÿ�γ�ʼ���궼����
    int threadId_;           // �����߳�id����Cachedģʽ������ɾ���̶߳�������߳�id�����Ǵ����̵߳ı�ţ�������pcb�����¼�Ǹ�
};

#endif // !THREAD_H
