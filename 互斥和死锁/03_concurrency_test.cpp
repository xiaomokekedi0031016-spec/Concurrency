#if 0
//抛异常解决方案
#include <mutex>
#include <iostream>
#include <stack>

//抛异常解决方案
struct empty_stack : std::exception
{
    const char* what() const throw() {
        //相当于是noexcept，表示这个函数不会抛出异常
        return "empty stack!";
    }
};

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
        if (data.empty()) {
            throw empty_stack();
        }
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
        try {
            if (!safe_stack.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                safe_stack.pop();
            }
        }
        catch (const empty_stack& e) {
            std::cout << e.what() << std::endl;
        }
        });
    std::thread t2([&safe_stack]() {
        try {
            if (!safe_stack.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                safe_stack.pop();
            }
        }
        catch (const empty_stack& e) {
            std::cout << e.what() << std::endl;
        }
        });
    //std::thread t2([&safe_stack]() {
    //    if (!safe_stack.empty()) {
    //        std::this_thread::sleep_for(std::chrono::seconds(1));//某一个时刻t1和t2都判断了safe_stack不为空，进入了if语句块，但是t1先执行了sleep_for，等t1睡眠结束后，t2先执行了pop操作，将栈顶元素弹出，此时safe_stack变成空的了，接着t1执行pop操作，此时safe_stack已经是空的了，所以会出现异常
    //        safe_stack.pop();
    //    }
    //    });
    t1.join();
    t2.join();
}



int main() {
    test3();


    return 0;
}

#endif

#if 0
//引用解决方案
#include <mutex>
#include <iostream>
#include <stack>

//抛异常解决方案
struct empty_stack : std::exception
{
    const char* what() const throw() {
        //相当于是noexcept，表示这个函数不会抛出异常
        return "empty stack!";
    }
};

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
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) throw empty_stack();
        value = data.top();
        data.pop();
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

    auto worker = [&safe_stack]() {
        try {
            int value;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop(value);
            std::cout << "pop success, value = " << value << std::endl;
        }
        catch (const empty_stack& e) {
            std::cout << "stack empty: " << e.what() << std::endl;
        }
        };
    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();
}



int main() {
    test3();


    return 0;
}

#endif


#if 0
//智能指针解决方案
#include <mutex>
#include <iostream>
#include <stack>

//抛异常解决方案
struct empty_stack : std::exception
{
    const char* what() const throw() {
        //相当于是noexcept，表示这个函数不会抛出异常
        return "empty stack!";
    }
};

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
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        //试图弹出前检查是否为空栈
        if (data.empty()) throw empty_stack();
		//if(data.empty()) return nullptr; //如果不想抛异常，可以返回一个空指针，表示弹出失败
        //改动栈容器前设置返回值
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
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

    auto worker = [&safe_stack]() {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
            std::cout << "pop success" << std::endl;
        }
        catch (const empty_stack& e) {
            std::cout << "stack empty: " << e.what() << std::endl;
        }
        };
    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();
}



int main() {
    test3();

    return 0;
}

#endif