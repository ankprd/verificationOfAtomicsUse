//from folly/executors/SerialExecutor.cpp


class SerialExecutor{
	Executor parent_;
	size_t scheduled_; //atomic
	UnboundedQueue queue_; //multiple producers single consumer
	int keepAliveCounter_;

	SerialExecutor(parent){
		parent_ = parent;
		queue = newQueue();
		scheduled_ = 0;
		keepAliveCounter_ = 1;
	}

	void add(Func func){
		queue_.enqueue(func);
		parent_->add({this->run()});
	}

	void run(){
		if(scheduled_.fetch_add(1, acquire) > 0)
			return;
		do {
			Func func = queue_.dequeue();
			func();
		} while(scheduled_.fetch_sub(1, release) > 1);
	}

	void keepAliveAcquire(){
		int c = keepAliveCounter_.fetch_add(1, acquire);
	}

	void keepAliveRelease(){
		int c = keepAliveCounter_.fetch_sub(1, release);
		if(c == 1)
			delete this;
	}

  KeepAlive getKeepAliveCounter(Executor* executor){
    executor->keepAliveAcquire();
    return KeepAlive(executor, false);
  }

	KeepAlive create(Executor parent){
		return KeepAlive(SerialExecutor(parent));
	}
}

class KeepAlive{
	Executor* executor_;

	KeepAlive(KeepAlive other){
    *this = getKeepAliveToken(other.get());
  }

  KeepAlive(Executor* executor, bool v) : executor_(executor){}

	~KeepAlive() {
		executor->keepAliveRelease();
	}

	Executor* get(){
		return executor;
	}

	KeepAlive copy(){
		executor.keepAliveAcquire();
		return KeepAlive(executor);
	}
}
