#if 1
#include <mutex>
#include <iostream>
#include <stack>
#include <thread>
#include <chrono>
#if 0


//test1
std::mutex mtx1;
int shared_data = 100;
void use_lock() {
    while (true) {
        mtx1.lock();
        shared_data++;
        std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
        std::cout << "sharad data is " << shared_data << std::endl;
        mtx1.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}
void test1() {
    std::thread t1(use_lock);
    std::thread t2([]() {
        while (true) {
            mtx1.lock();
            shared_data--;
            std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
            std::cout << "sharad data is " << shared_data << std::endl;
            mtx1.unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        });
    t1.join();
    t2.join();
}

//test2
void use_lock1() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx1);
            shared_data++;
            std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
            std::cout << "sharad data is " << shared_data << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void test2() {
    std::thread t1(use_lock1);
    std::thread t2([]() {
        while (true) {
            {
				std::lock_guard<std::mutex> lock(mtx1);//使用lock_guard注意锁的范围，锁的范围越小越好
                shared_data--;
                std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
                std::cout << "sharad data is " << shared_data << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        });
    t1.join();
    t2.join();
}

//test3 >> err
template<typename T>
class threadsafe_stack1
{
private:
    std::stack<T> data;
    mutable std::mutex m;//一般在获取get的操作下会设计这种互斥锁
public:
    threadsafe_stack1() {}
    threadsafe_stack1(const threadsafe_stack1& other)
    {
		std::lock_guard<std::mutex> lock(other.m);//给要复制的对象加锁，保证在复制过程中数据的一致性
        //在构造函数的函数体（constructor body）内进行复制操作
        data = other.data;
    }
    threadsafe_stack1& operator=(const threadsafe_stack1&) = delete;
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }
    //问题代码
    T pop()
    {
        std::lock_guard<std::mutex> lock(m);
        auto element = data.top();
        data.pop();
        return element;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

void test3() {
    threadsafe_stack1<int> safe_stack;
    safe_stack.push(1);
    std::thread t1([&safe_stack]() {
        if (!safe_stack.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
        });
    std::thread t2([&safe_stack]() {
        if (!safe_stack.empty()) {
			std::this_thread::sleep_for(std::chrono::seconds(1));//某一个时刻t1和t2都判断了safe_stack不为空，进入了if语句块，但是t1先执行了sleep_for，等t1睡眠结束后，t2先执行了pop操作，将栈顶元素弹出，此时safe_stack变成空的了，接着t1执行pop操作，此时safe_stack已经是空的了，所以会出现异常
            safe_stack.pop();
        }
        });
    t1.join();
    t2.join();
}

//test4 
//A在等m2、B在等m1 >> 死锁
std::mutex t_lock1;
std::mutex t_lock2;
int m_1 = 0;
int m_2 = 1;
void dead_lock1() {
    while (true) {
        std::cout << "dead_lock1 begin " << std::endl;
        t_lock1.lock();
        m_1 = 1024;
        t_lock2.lock();
        m_2 = 2048;
        t_lock2.unlock();
        t_lock1.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock2 end " << std::endl;
    }
}
void dead_lock2() {
    while (true) {
        std::cout << "dead_lock2 begin " << std::endl;
        t_lock2.lock();
        m_2 = 2048;
        t_lock1.lock();
        m_1 = 1024;
        t_lock1.unlock();
        t_lock2.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock2 end " << std::endl;
    }
}

void test4() {
    std::thread t1(dead_lock1);
    std::thread t2(dead_lock2);
    t1.join();
    t2.join();
}

//test5
//加锁和解锁作为原子操作解耦合，各自只管理自己的功能
void atomic_lock1() {
    std::cout << "lock1 begin lock" << std::endl;
    t_lock1.lock();
    m_1 = 1024;
    t_lock1.unlock();
    std::cout << "lock1 end lock" << std::endl;
}
void atomic_lock2() {
    std::cout << "lock2 begin lock" << std::endl;
    t_lock2.lock();
    m_2 = 2048;
    t_lock2.unlock();
    std::cout << "lock2 end lock" << std::endl;
}
void safe_lock1() {
    while (true) {
        atomic_lock1();
        atomic_lock2();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
void safe_lock2() {
    while (true) {
        atomic_lock2();
        atomic_lock1();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
void test5() {
    std::thread t1(safe_lock1);
    std::thread t2(safe_lock2);
    t1.join();
    t2.join();
}




//test6
class som_big_object {
public:
    som_big_object(int data) :_data(data) {}
    //拷贝构造
    som_big_object(const som_big_object& b2) :_data(b2._data) {
        _data = b2._data;
    }
    //移动构造
    som_big_object(som_big_object&& b2) noexcept :_data(std::move(b2._data)) {
    }
    //重载输出运算符
    friend std::ostream& operator << (std::ostream& os, const som_big_object& big_obj) {
        os << big_obj._data;
        return os;
    }
    //重载赋值运算符
    som_big_object& operator = (const som_big_object& b2) {
        if (this == &b2) {
            return *this;
        }
        _data = b2._data;
        return *this;
    }
    //交换数据
    friend void swap(som_big_object& b1, som_big_object& b2) {
        som_big_object temp = std::move(b1);
        b1 = std::move(b2);
        b2 = std::move(temp);
    }
private:
    int _data;
};

class big_object_mgr {
public:
    big_object_mgr(int data = 0) :_obj(data) {}
    void printinfo() {
        std::cout << "current obj data is " << _obj << std::endl;
    }
    friend void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2);
    friend void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2);
    friend void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2);
private:
    std::mutex _mtx;
    som_big_object _obj;
};

//线程1拿着 objm1 的锁，等 objm2 的锁、线程2拿着 objm2 的锁，等 objm1 的锁
void danger_swap(big_object_mgr& objm1, big_object_mgr& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::lock_guard <std::mutex> gurad1(objm1._mtx);
    //此处为了故意制造死锁，我们让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void test6() {
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(danger_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(danger_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

//test7
void safe_swap(big_object_mgr& objm1, big_object_mgr& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    //std::lock_guard（配合 std::adopt_lock）利用了 C++ 的 RAII 机制：无论函数是正常结束还是因为异常退出，guard 对象都会析构，从而自动调用 unlock()，保证锁一定会被释放。
    std::lock(objm1._mtx, objm2._mtx);
    //领养锁管理它自动释放
    std::lock_guard <std::mutex> gurad1(objm1._mtx, std::adopt_lock);
    //此处为了故意制造死锁，我们让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard <std::mutex> gurad2(objm2._mtx, std::adopt_lock);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void test7() {
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

//test8
//c++17 引入了 std::scoped_lock，它可以同时锁定多个互斥量，并且在作用域结束时自动解锁，避免了死锁的风险。
void safe_swap_scope(big_object_mgr& objm1, big_object_mgr& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::scoped_lock guard(objm1._mtx, objm2._mtx);
    //等价于
    //std::scoped_lock<std::mutex, std::mutex> guard(objm1._mtx, objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void test8() {
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap_scope, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap_scope, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

#endif
//test9
//层级锁
class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned long value) :_hierarchy_value(value),
        _previous_hierarchy_value(0) {
    }
    hierarchical_mutex(const hierarchical_mutex&) = delete;
    hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;
    void lock() {
        check_for_hierarchy_violation();
        _internal_mutex.lock();
        update_hierarchy_value();
    }
    void unlock() {
        if (_this_thread_hierarchy_value != _hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        _this_thread_hierarchy_value = _previous_hierarchy_value;
        _internal_mutex.unlock();
    }
    bool try_lock() {
        check_for_hierarchy_violation();
        if (!_internal_mutex.try_lock()) {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
private:
    std::mutex  _internal_mutex;
    //当前层级值
    unsigned long const _hierarchy_value;
    //上一次层级值
    unsigned long _previous_hierarchy_value;
    //本线程记录的层级值
    static thread_local  unsigned long  _this_thread_hierarchy_value;
    void check_for_hierarchy_violation() {
        if (_this_thread_hierarchy_value <= _hierarchy_value) {
            throw  std::logic_error("mutex  hierarchy violated");
        }
    }
    void  update_hierarchy_value() {
        _previous_hierarchy_value = _this_thread_hierarchy_value;
        _this_thread_hierarchy_value = _hierarchy_value;
    }
};

//thread_local 变量用于存储每个线程的层级值
thread_local unsigned long hierarchical_mutex::_this_thread_hierarchy_value(ULONG_MAX);
void test9() {
    hierarchical_mutex  hmtx1(1000);
    hierarchical_mutex  hmtx2(500);
    std::thread t1([&hmtx1, &hmtx2]() {
        hmtx1.lock();
        hmtx2.lock();
        hmtx2.unlock();
        hmtx1.unlock();
        });
    std::thread t2([&]() {
        try {
            hmtx2.lock();
            hmtx1.lock();
            hmtx1.unlock();
            hmtx2.unlock();
        }
        catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
        });
    t1.join();
    t2.join();
}

void test10() {
    hierarchical_mutex hmtx1(1000);
    hierarchical_mutex hmtx2(500);

    std::thread t1([&hmtx1, &hmtx2]() {
        // 简单示例：如果不成功就自旋等待，或者直接放弃
        if (hmtx1.try_lock()) {
            if (hmtx2.try_lock()) {
                hmtx2.unlock();
            }
            hmtx1.unlock();
        }
        });

    std::thread t2([&hmtx1, &hmtx2]() {
        // 注意：这里试图违反层级顺序 (先锁500再锁1000)，
        // 根据你的 check_for_hierarchy_violation，这会直接抛出异常，
        // 因为锁住 hmtx2 后当前线程层级降为 500，再锁 hmtx1 (1000) 时 500 < 1000，报错。

        if (hmtx2.try_lock()) {
            // 此时当前线程层级变成 500
            try {
                // 尝试锁 1000 -> 抛出异常 "mutex hierarchy violated"
                if (hmtx1.try_lock()) {
                    hmtx1.unlock();
                }
            }
            catch (...) {
                // 捕获异常
            }
            hmtx2.unlock();
        }
        });

    t1.join();
    t2.join();
}


int main() {
    //test1();
    //test2();
    //test3();
    //test4();
    //test5();
    //test6();
    //test7();
    //test8();
    test9();
    //test10();


    return 0;
}

#endif