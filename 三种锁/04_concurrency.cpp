#if 0 
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <map>

//test1
//unique_lock 基本用法
std::mutex mtx;
int shared_data = 0;
void use_unique() {
    //lock可自动解锁，也可手动解锁
    std::unique_lock<std::mutex> lock(mtx);
    std::cout << "lock success" << std::endl;
    shared_data++;
    lock.unlock();
}

//可判断是否占有锁
void owns_lock() {
    //lock可自动解锁，也可手动解锁
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    if (lock.owns_lock()) {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
    lock.unlock();
    if (lock.owns_lock()) {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
}

//可以延迟加锁
void defer_lock() {
    //延迟加锁
    std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
    //可以加锁
    lock.lock();
    //可以自动析构解锁，也可以手动解锁
    lock.unlock();
}

//test2
//同时使用owns和defer
void use_own_defer() {
    std::unique_lock<std::mutex> lock(mtx);
    // 判断是否拥有锁
    if (lock.owns_lock()){
        std::cout << "Main thread has the lock." << std::endl;
    }
    else{
        std::cout << "Main thread does not have the lock." << std::endl;
    }
    std::thread t([]() {
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 加锁
        lock.lock();
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 解锁
        lock.unlock();
        });
    t.join();
}

void use_own_adopt() {
    mtx.lock();
    std::unique_lock<std::mutex> lock(mtx, std::adopt_lock);
    if (lock.owns_lock()) {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "does not have the lock" << std::endl;
    }
    lock.unlock();
}

//test3
//之前的交换代码可以可以用如下方式等价实现
int a = 10;
int b = 99;
std::mutex mtx1;
std::mutex mtx2;
void safe_swap() {
    std::lock(mtx1, mtx2);
    std::unique_lock<std::mutex> lock1(mtx1, std::adopt_lock);
    std::unique_lock<std::mutex> lock2(mtx2, std::adopt_lock);
    std::swap(a, b);
    //错误用法
    //mtx1.unlock();
    //mtx2.unlock();
}
void safe_swap2() {
    std::unique_lock<std::mutex> lock1(mtx1, std::defer_lock);
    std::unique_lock<std::mutex> lock2(mtx2, std::defer_lock);
    //需用lock1,lock2加锁
    std::lock(lock1, lock2);
    //错误用法
    //std::lock(mtx1, mtx2);
    std::swap(a, b);
}

//test4
//转移互斥量所有权
//互斥量本身不支持move操作，但是unique_lock支持
std::unique_lock <std::mutex>  get_lock() {
    std::unique_lock<std::mutex>  lock(mtx);
    shared_data++;
	//先找的是拷贝构造函数，unique_lock的拷贝构造函数被删除了，所以会找move构造函数，转移锁的所有权
	return lock;//返回时会调用unique_lock的move构造函数，转移锁的所有权
}
void use_return() {
    std::unique_lock<std::mutex> lock(get_lock());
    shared_data++;
}

//test5
class DNService {
public:
    DNService() {}
    //读操作采用共享锁
    std::string QueryDNS(std::string dnsname) {
        std::shared_lock<std::shared_mutex> shared_locks(_shared_mtx);
        auto iter = _dns_info.find(dnsname);
        if (iter != _dns_info.end()) {
            return iter->second;
        }
        return "";
    }
    //写操作采用独占锁
    void AddDNSInfo(std::string dnsname, std::string dnsentry) {
        std::lock_guard<std::shared_mutex>  guard_locks(_shared_mtx);
        _dns_info.insert(std::make_pair(dnsname, dnsentry));
    }
private:
    std::map<std::string, std::string> _dns_info;
    mutable std::shared_mutex  _shared_mtx;
};

//test6
//递归锁
class RecursiveDemo {
public:
    RecursiveDemo() {}
    bool QueryStudent(std::string name) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            return false;
        }
        return true;
    }
    void AddScore(std::string name, int score) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        if (!QueryStudent(name)) {
            _students_info.insert(std::make_pair(name, score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
    }
    //不推荐采用递归锁，使用递归锁说明设计思路并不理想，需优化设计
    //推荐拆分逻辑，将共有逻辑拆分为统一接口
    void AddScoreAtomic(std::string name, int score) {
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            _students_info.insert(std::make_pair(name, score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
        return;
    }
private:
    std::map<std::string, int> _students_info;
    std::recursive_mutex  _recursive_mtx;
};

void test1() {
	owns_lock();
}

void test2() {
    use_own_defer();
}   

void test3() {
    //safe_swap();
    safe_swap2();
	std::cout << "a:" << a << " b:" << b << std::endl;  
}   

void test4() {
    use_return();
    std::cout << "shared_data: " << shared_data << std::endl;
}

int main() {
    //test1();
    //test2();
    //test3();
    //test4();

    return 0;
}   
#endif