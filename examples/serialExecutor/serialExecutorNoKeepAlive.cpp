//from folly/executors/SerialExecutor.cpp
//No keepAliveCounter, we'll see about it later

class SerialExecutor{
	Executor parent_;
	size_t scheduled_; //atomic
	UnboundedQueue queue_; //multiple producers single consumer

	SerialExecutor(parent){
		parent_ = parent;
		queue = newQueue();
		scheduled_ = 0;
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
}
