#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

//test1 无序
std::atomic<bool> x, y;
std::atomic<int> z;
#if 0
//load和store都采用的是memory_order_relaxed。线程t1按次序执行1和2，但是线程t2看到的可能是y为true,x为false。进而导致TestOrderRelaxed触发断言z为0.
//store顺序对其他线程不保证load看到y==true后，不保证x已经可见
//自旋等待是因为load还没看到值；relaxed保证不了store的顺序，所以看到y==true时x可能还不可见。
void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);  // 1
    y.store(true, std::memory_order_relaxed);  // 2
}
void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed)) { // 3
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_relaxed)) { //4
        ++z;
    }
}
void TestOrderRelaxed() {
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5
}

void test1() {
    TestOrderRelaxed();
}

#endif

#if 0
//test2 全局顺序一致的强一致性，所有线程看到的操作顺序相同
void write_x_then_y() {
    x.store(true, std::memory_order_seq_cst);  // 1
    y.store(true, std::memory_order_seq_cst);  // 2
}
void read_y_then_x() {
    while (!y.load(std::memory_order_seq_cst)) { // 3
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_seq_cst)) { //4
        ++z;
    }
}
void TestOrderSeqCst() {
	//cpu调度线程的执行顺序是不确定的，t1和t2哪个先执行是随机的
    std::thread t1(write_x_then_y);
	std::thread t2(read_y_then_x);//无论t1和t2哪个先执行，t2都能看到y为true,x为true，最终z的值不为0
	//因为线程2如果先执行，那么它会一直循环等待(自旋等待)y为true；如果线程1先执行，那么线程2就能看到y为true和x为true，最终z的值不为0
    t1.join();
    t2.join();
    assert(z.load() != 0); // 5
}

void test2() {
    TestOrderSeqCst();
}
#endif

//test3
void TestReleaseAcquire() {
    std::atomic<bool> rx, ry;
    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed); // 1
		//release操作保证了在它之前的所有store操作对其他线程可见；而acquire操作保证了在它之后的所有load操作能看到release操作之前的store操作的结果(实际上就是内存屏障)
        ry.store(true, std::memory_order_release); // 2
        });
    std::thread t2([&]() {
        while (!ry.load(std::memory_order_acquire)); //3
        assert(rx.load(std::memory_order_relaxed)); //4
        });
    t1.join();
    t2.join();
}

void test3() {
    TestReleaseAcquire();
}


//test4 release-sequence
//多个线程对同一个变量release操作，另一个线程对这个变量acquire，那么只有一个线程的release操作和这个acquire线程构成同步关系
//如果线程t1和t2都对yd进行release操作，情况1：线程t3对yd进行acquire操作，那么只有线程t2的release操作和线程t3的acquire操作构成同步关系，线程t1的release操作和线程t3的acquire操作不构成同步关系，所以线程t3只能看到线程t2的release操作之前的store操作的结果，而看不到线程t1的release操作之前的store操作的结果 >> 因此会触发断言失败
void ReleasAcquireDanger2() {
    std::atomic<int> xd{ 0 }, yd{ 0 };
    std::atomic<int> zd;
    std::thread t1([&]() {
        xd.store(1, std::memory_order_release);  // (1)
        yd.store(1, std::memory_order_release); //  (2)
        });
    std::thread t2([&]() {
        yd.store(2, std::memory_order_release);  // (3)
        });
    std::thread t3([&]() {
        while (!yd.load(std::memory_order_acquire)); //（4）
        assert(xd.load(std::memory_order_acquire) == 1); // (5)
        });
    t1.join();
    t2.join();
    t3.join();
}

void test4() {
    ReleasAcquireDanger2();
}   


//test6
void ReleaseSequence() {
    std::vector<int> data;
    std::atomic<int> flag{ 0 };
	//release sequence
    std::thread t1([&]() {
        data.push_back(42);  //(1)
        flag.store(1, std::memory_order_release); //(2)
        });
    std::thread t2([&]() {
        int expected = 1;
		//自旋是自己构建的while循环
        //while不满足就会自旋,满足就结束了
		//自旋的时候其它线程会执行并且可能会修改flag的值
        while (!flag.compare_exchange_strong(expected, 2, std::memory_order_relaxed)) // (3)
            expected = 1;
        });
    //acquire
    std::thread t3([&]() {
        while (flag.load(std::memory_order_acquire) < 2); // (4)
        assert(data.at(0) == 42); // (5)
        });
    t1.join();
    t2.join();
    t3.join();
}

void test6() {
	ReleaseSequence();
}
//test7
void ConsumeDependency() {
    std::atomic<std::string*> ptr;
    int data;
    std::thread t1([&]() {
        std::string* p = new std::string("Hello World"); // (1)
        data = 42; // (2)
        ptr.store(p, std::memory_order_release); // (3)
        });
    std::thread t2([&]() {
        std::string* p2;
        //memory_order_consume 的核心作用就是建立“数据依赖”关系，而不是建立完全的内存屏障
        while (!(p2 = ptr.load(std::memory_order_consume))); // (4)
        assert(*p2 == "Hello World"); // (5)
        assert(data == 42); // (6)
        });
    t1.join();
    t2.join();
}

void test7() {
    ConsumeDependency();
}

//双重锁单例模式
//利用内存屏障解决释放问题
class SingleMemoryModel
{
private:
    SingleMemoryModel()
    {
    }
    SingleMemoryModel(const SingleMemoryModel&) = delete;
    SingleMemoryModel& operator=(const SingleMemoryModel&) = delete;
public:
    ~SingleMemoryModel()
    {
        std::cout << "single auto delete success " << std::endl;
    }
    static std::shared_ptr<SingleMemoryModel> GetInst()
    {
        // 1 处
        if (_b_init.load(std::memory_order_acquire))
        {
            return single;
        }
        // 2 处
        s_mutex.lock();
        // 3 处
        if (_b_init.load(std::memory_order_relaxed))
        {
            s_mutex.unlock();
            return single;
        }
        // 4处
        //acquire 保证后面的读不能重排到前面
        single = std::shared_ptr<SingleMemoryModel>(new SingleMemoryModel);
        _b_init.store(true, std::memory_order_release);
        s_mutex.unlock();
        return single;
    }
private:
    static std::shared_ptr<SingleMemoryModel> single;
    static std::mutex s_mutex;
    static std::atomic<bool> _b_init;
};
std::shared_ptr<SingleMemoryModel> SingleMemoryModel::single = nullptr;
std::mutex SingleMemoryModel::s_mutex;
std::atomic<bool> SingleMemoryModel::_b_init = false;

void TestSingleMemory() {
    std::thread t1([]() {
        std::cout << "thread t1 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl;
        });
    std::thread t2([]() {
        std::cout << "thread t2 singleton address is 0x: " << SingleMemoryModel::GetInst() << std::endl;
        });
    t2.join();
    t1.join();
}

void test8() {
    TestSingleMemory();
}

int main() {
    //test1();
    //test2();
    //test3();
	//test4();
    //test5();
    //test6();
    //test7();
    test8();

    return 0;
}