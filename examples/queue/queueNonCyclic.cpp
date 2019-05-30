//taken from folly/folly/ProducerConsumerQueue.h
//simplified version to think on

//a try assuming an infinite array records_ (and hence no cycling around the array)
//to try to prove DRF. We'll see about cycles later 

class ProducerConsumerQueue{
	T* records_;

	//AtomicIndex = std::atomic<unsigned int>
	AtomicIndex readIndex_;
	AtomicIndex writeIndex_;

	//pad0 and pad1 for padding -> ignore for now
	
	ProducerConsumerQueue(){
		records = malloc(sizeof(T) * oo);
		readIndex_ = 0;
		writeIndex_ = 0;
	}	


	bool write(Arg recordArg){
		AtomicIndex currentWrite = writeIndex_.load(relaxed);
		AtomicIndex nextRecord = currentWrite + 1;
		records[currentWrite] = new T(recordArg);
		writeIndex_.store(nextRecors, release);
		return true;
		
		//array is infinite, we can always add stg in
	}

	bool read(T& record){
		AtomicIndex currentRead = readIndex_.load(relaxed);
		if(currentRead == writeIndex_.load(acquire))
			return false; //queue is empty
		AtomicIndex nextRecord = currentRead + 1;
		record = records_[currentRead];
		records_[currentRead].~T();
		readIndex_.store(nextRecord, release);
		return true;
	}

	bool isEmpty() const {
		return readIndex_.load(acquire) == return writeIndex_.load(acquire);
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
