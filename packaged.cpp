#include<bits/stdc++.h>
using namespace std;
class ThreadPool{
private:
    vector<thread> works;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop;
public:
    ThreadPool(size_t n):stop(false){
        for(int i=0;i<n;i++){
            works.emplace_back([this](){
                while(true){
                    unique_lock<mutex>lock(mtx);
                    cv.wait(lock,[this](){
                        return stop || tasks.size();
                    });
                    if(stop && tasks.empty()) return;
                    auto task=move(tasks.front());
                    tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }
    ~ThreadPool(){
        {
            unique_lock<mutex>lock(mtx);
            stop=true;
        }
        cv.notify_all();
        for(auto &t:works) t.join();
    }
    template<typename F,typename... Args>
    auto enqueue(F &&f,Args&&... args)->future<invoke_result_t<F,Args...>>{
        using FType = invoke_result_t<F, Args...>;
        /*
        等效于
        1. 使用 bind 将传入的可调用对象 f 和参数 args 绑定起来，生成一个无参数的可调用对象。
       bind(forward<F>(f), forward<Args>(args)...) 的结果是一个可以执行的函数对象，将该函数通过lambda传递给线程池。
       forward 用于完美转发参数，保持最开始参数的值类别（左值或右值）。
       ref() 可以用来传递引用参数，避免不必要的拷贝。

        2. 使用 packaged_task 封装这个可调用对象：
       packaged_task<函数（参数类型）> task(可调用对象);
         这里的函数类型是 FType()，表示无参数返回 FType 类型的函数。

        3. 将 packaged_task 包装到 shared_ptr 中：
       auto task = make_shared<packaged_task<FType()>>(...);
       这样做可以安全地在多线程环境中传递和拷贝任务对象，保证生命周期管理。
        */
        auto task=make_shared<packaged_task<FType()>>(
            bind(forward<F>(f),forward<Args>(args)...)
        );

        auto res=task->get_future();
        {
            unique_lock<mutex>lock(mtx);
            tasks.emplace([task]{
                (*task)();
            });
        }
        cv.notify_one();
        return res;
    }
};

int main(){
    ThreadPool pool(4);
    vector<future<int>>res;
    for(int i=0;i<10;i++){
        //  res先放入 占位符 future， 后续再获取结果
        res.emplace_back(pool.enqueue([i](){
            cout<<"task : "<<i<<" is running"<<endl;
            this_thread::sleep_for(chrono::seconds(1));
            cout<<"task : "<<i<<" is finished"<<endl;
            return i+10;
        }));
        cout<<"i = "<<i<<"result : "<<endl;
    }
    for(auto &x:res) cout<<"results : "<<x.get()<<endl;
}
