#if 0
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>

//test1
//局部静态变量的单例模式
class Single2 {
private:
    Single2()
    {
    }
    Single2(const Single2&) = delete;
    Single2& operator=(const Single2&) = delete;
public:
    static Single2& GetInst()
    {
        static Single2 single;
        return single;
    }
};

//test2
//饿汉式
class Single2Hungry
{
private:
    Single2Hungry()
    {
    }
    Single2Hungry(const Single2Hungry&) = delete;
    Single2Hungry& operator=(const Single2Hungry&) = delete;
public:
    static Single2Hungry* GetInst()
    {
        if (single == nullptr)
        {
            single = new Single2Hungry();
        }
        return single;
    }
private:
    static Single2Hungry* single;
};

//饿汉式初始化
Single2Hungry* Single2Hungry::single = Single2Hungry::GetInst();
void thread_func_s2(int i)
{
    std::cout << "this is thread " << i << std::endl;
    std::cout << "inst is " << Single2Hungry::GetInst() << std::endl;
}
void test_single2hungry()
{
    std::cout << "s1 addr is " << Single2Hungry::GetInst() << std::endl;
    std::cout << "s2 addr is " << Single2Hungry::GetInst() << std::endl;
    for (int i = 0; i < 3; i++)
    {
        std::thread tid(thread_func_s2, i);
        tid.join();
    }
}

//test3
//懒汉式 >> 这种方式存在一个很严重的问题，就是当多个线程都调用单例函数时，我们不确定资源是被哪个线程初始化的。回收指针存在问题，存在多重释放或者不知道哪个指针释放的问题。
class SinglePointer
{
private:
    SinglePointer()
    {
    }
    SinglePointer(const SinglePointer&) = delete;
    SinglePointer& operator=(const SinglePointer&) = delete;
public:
    static SinglePointer* GetInst()
    {
		//第一次判断是为了避免每次调用GetInst都加锁
        if (single != nullptr)
        {
            return single;
        }
        s_mutex.lock();
        //第二次判断是为了防止多个线程同时进入第一次判断后都创建实例
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        single = new SinglePointer();
        s_mutex.unlock();
        return single;
    }
private:
    static SinglePointer* single;
    static std::mutex s_mutex;
};

//test4
//懒汉式(智能指针)
//可以利用智能指针完成自动回收
class SingleAuto
{
private:
    SingleAuto()
    {
    }
    SingleAuto(const SingleAuto&) = delete;
    SingleAuto& operator=(const SingleAuto&) = delete;
public:
    ~SingleAuto()
    {
        std::cout << "single auto delete success " << std::endl;
    }
    static std::shared_ptr<SingleAuto> GetInst()
    {
        if (single != nullptr)
        {
            return single;
        }
        s_mutex.lock();
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        single = std::shared_ptr<SingleAuto>(new SingleAuto);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleAuto> single;
    static std::mutex s_mutex;
};

std::shared_ptr<SingleAuto> SingleAuto::single = nullptr;
std::mutex SingleAuto::s_mutex;
void test_singleauto()
{
    auto sp1 = SingleAuto::GetInst();
    auto sp2 = SingleAuto::GetInst();
    std::cout << "sp1  is  " << sp1 << std::endl;
    std::cout << "sp2  is  " << sp2 << std::endl;
    //此时存在隐患，可以手动删除裸指针，造成崩溃
    // delete sp1.get();
}

//test5
//设置辅助类，防止用户手动删除裸指针
class SingleAutoSafe;
class SafeDeletor
{
public:
    void operator()(SingleAutoSafe* sf)
    {
        std::cout << "this is safe deleter operator()" << std::endl;
        delete sf;
    }
};
class SingleAutoSafe
{
private:
    SingleAutoSafe() {}
    ~SingleAutoSafe()
    {
        std::cout << "this is single auto safe deletor" << std::endl;
    }
    SingleAutoSafe(const SingleAutoSafe&) = delete;
    SingleAutoSafe& operator=(const SingleAutoSafe&) = delete;
    //定义友元类，通过友元类调用该类析构函数
    friend class SafeDeletor;
public:
    static std::shared_ptr<SingleAutoSafe> GetInst()
    {
        //1处
        if (single != nullptr)
        {
            return single;
        }
        s_mutex.lock();
        //2处
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        //额外指定删除器  
        //3 处
        single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDeletor());
        //也可以指定删除函数
        // single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDelFunc);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleAutoSafe> single;
    static std::mutex s_mutex;
};

//test6
//使用std::call_once实现单例模式
class SingletonOnce {
private:
    SingletonOnce() = default;
    SingletonOnce(const SingletonOnce&) = delete;
    SingletonOnce& operator = (const SingletonOnce& st) = delete;
    static std::shared_ptr<SingletonOnce> _instance;
public:
    static std::shared_ptr<SingletonOnce> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<SingletonOnce>(new SingletonOnce);
            });
        return _instance;
    }
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }
    ~SingletonOnce() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};
std::shared_ptr<SingletonOnce> SingletonOnce::_instance = nullptr;

//test7奇异递归模板
template <typename T>
class SingletonCRTP {
protected:
    SingletonCRTP() = default;
    SingletonCRTP(const SingletonCRTP<T>& st) = delete;
	SingletonCRTP& operator = (const SingletonCRTP<T>& st) = delete;
    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> GetInstance() {
		static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
	     });
        return _instance;
	}

    void PrintAddress() {
        std::cout << this << std::endl;
    }

    ~SingletonCRTP() {
        std::cout << "singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> SingletonCRTP<T>::_instance = nullptr;

void TestSingle() {
    std::thread t1([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SingletonOnce::GetInstance()->PrintAddress();
        });
    std::thread t2([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        SingletonOnce::GetInstance()->PrintAddress();
        });
    t1.join();
    t2.join();
}

void test1() {
    Single2& single = Single2::GetInst();
    Single2& single2 = Single2::GetInst();
    if (&single == &single2) {
        std::cout << "single and single2 are the same instance." << std::endl;
    }
    else {
        std::cout << "single and single2 are different instances." << std::endl;
    }
}

void test2() {
    test_single2hungry();
}

void test4() {
    test_singleauto();
}

void test6() {
	TestSingle();
}

int main() {
    //test1();
    //test2();
    //test4();
    //test6();

    return 0;
}

#endif