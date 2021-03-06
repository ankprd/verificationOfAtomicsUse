//taken from folly/folly/ProducerConsumerQueue.h
//simplified version to think on

//we have only size-1 element storage capacity

class ProducerConsumerQueue{
	int size_;
	T* records_;

	//Atomic locations
	int readIndex_;
	int writeIndex_;

	
	ProducerConsumerQueue(int size){
		size_ = size;
		records = malloc(sizeof(T) * size);
		readIndex_ = 0;
		writeIndex_ = 0;
	}	

	bool write(Arg recordArg){
		int currentWrite = Load_Rlx(writeIndex_);
		int nextRecord = currentWrite + 1;
		if(nextRecord == size_){
			nextRecord = 0;
		}
		if(nextRecord != Load_Acq(readIndex_)){
			records[currentWrite] = new T(recordArg);
			Store_Rel(writeIndex_, nextRecors);
			return true;
		}
		return false;
	}

	bool read(T& record){
		int currentRead = Load_Rlx(readIndex_);
		if(currentRead == Load_Acq(writeIndex_))
			return false; //queue is empty
		int nextRecord = currentRead + 1;
		if(nextRecord == size_){
			nextRecord = 0;
		}
		record = records_[currentRead];
		records_[currentRead].~T();
		Store_Rel(readIndex_, nextRecord);
		return true;
	}

	T* frontPtr(){
		int currentRead = Load_Rlx(readIndex_);
		if(currentRead == Load_Acq(writeIndex_))
			return nullptr;//queue is empty
		return &records_[currentRead];
	}
	
	bool isEmpty() const {
		return Load_Acq(readIndex_) == Load_Acq(writeIndex_);
	}

	bool isFull() const {
		int nextRecord = Load_Acq(writeIndex_) + 1;
		if(nextRecord == size_)
			nextRecord = 0;
		if(nextRecord != Load_Acq(readIndex_));
			return false;
		return true;
	}

	//if called by consumer, true size may be more
	//if called by producer true size may be less
	size_t sizeGuess() const {
		int ret = Load_Acq(writeIndex_) - Load_Acq(readIndex_);
		if(ret < 0)
			ret += size_;
		return ret;
	}
}
