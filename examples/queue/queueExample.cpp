//taken from folly/folly/ProducerConsumerQueue.h
//simplified version to think on

//we have only size-1 element storage capacity

class ProducerConsumerQueue{
	int size_;
	T* records_;

	//AtomicIndex = std::atomic<unsigned int>
	AtomicIndex readIndex_;
	AtomicIndex writeIndex_;

	//pad0 and pad1 for padding -> ignore for now
	
	ProducerConsumerQueue(int size){
		size_ = size;
		records = malloc(sizeof(T) * size);
		readIndex_ = 0;
		writeIndex_ = 0;
	}	

	//destructor : no synch -> ignore for now
	//but make sure can only be called by one thread + no concurrent reading on it ?
	

	//variadic templates are used -> ignore for now, all fcts take only one argument
	//ignore forward as well
	bool write(Arg recordArg){
		AtomicIndex currentWrite = writeIndex_.load(relaxed);
		AtomicIndex nextRecord = currentWrite + 1;
		if(nextRecord == size_){
			nextRecord = 0;
		}
		if(nextRecord != readIndex_.load(acquire)){
			records[currentWrite] = new T(recordArg);
			writeIndex_.store(nextRecors, release);
			return true;
		}
		return false;
	}

	bool read(T& record){
		AtomicIndex currentRead = readIndex_.load(relaxed);
		if(currentRead == writeIndex_.load(acquire))
			return false; //queue is empty
		AtomicIndex nextRecord = currentRead + 1;
		if(nextRecord == size_){
			nextRecord = 0;
		}
		record = records_[currentRead];
		records_[currentRead].~T();
		readIndex_.store(nextRecord, release);
		return true;
	}

	T* frontPtr(){
		AtomicIndex currentRead = readIndex_.load(relaxed);
		if(currentRead == writeIndex_.load(acquire))
			return nullptr;//queue is empty
		return &records_[currentRead];
	}

	//pop is similar to read
	
	bool isEmpty() const {
		return readIndex_.load(acquire) == return writeIndex_.load(acquire);
	}

	bool isFull() const {
		AtomicIndex nextRecord = writeIndex_.load(acquire) + 1;
		if(nextRecord == size_)
			nextRecord = 0;
		if(nextRecord != readIndex_.load(acquire));
			return false;
		return true;
	}

	//if called by consumer, true size may be more
	//if called by producer true size may be less
	size_t sizeGuess() const {
		int ret = writeIndex_.load(acquire) - readIndex_.load(acquire);
		if(ret < 0)
			ret += size_;
		return ret;
	}
}
