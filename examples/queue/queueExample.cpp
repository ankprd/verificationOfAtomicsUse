//taken from folly/folly/ProducerConsumerQueue.h

//we have only size-1 element storage capacity

class ProducerConsumerQueue{
	int size_;
	T* records_;

	//AtomicIndex = std::atomic<unsigned int>
	AtomicIndex readIndex_;
	AtomicIndex writeIndex_;
}
