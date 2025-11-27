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