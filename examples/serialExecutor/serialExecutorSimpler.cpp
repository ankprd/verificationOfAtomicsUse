Executor newSerialExecutor(Executor parent){
  s = alloc();
  s.parent_ = parent;
  s.scheduled_ = 0;
  s.keepAliveCounter_ = 1;
  s.queue = new Queue();
  return s;
}

void add(Func func, Executor s){
  s.queue_.enqueue(func);
	s.parent_.add({this->run()});
}

void run(Executor s){
	if(s.scheduled_.fetch_add(1, acquire) > 0)
		return;
	do {
		Func func = s.queue_.dequeue();
		func();
	} while(s.scheduled_.fetch_sub(1, release) > 1);
}

void drop(Executor s){
	int c = s.keepAliveCounter_.fetch_sub(1, release);
	if(c == 1)
		free(s);
}

Executor copy(Executor s){
	s.keepAliveCounter_.fetch_add(1, acquire);
	return s;
}

