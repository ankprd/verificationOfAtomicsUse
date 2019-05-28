Variables : 
__reader : offset of 2

//no loops , lock return true if they got the lock, unlock return true if succesfully unlocked
int readLock(l){
	if(curWriter == tid)
		return ERROR
	int r = fetchAndAdd_Acq(__readers, 4) + 4
	if(r & WRPHASE)
		return true;
	if(r & WRPHASE != 0 && r & WRLOCKED == 0){
		if(CAS_Acq(__readers, &r, r ^ WRPHASE))
			return true;
	}
	return false;
}

bool readUnlock(l){
	int r = Load_Rlx(__readers)
	int rNew = r - 4
	if(rNew / 4 == 0 && rNew & WRLOCKED)
			rNew |= WRPHASE
	return (CAS_Rel(__readers, r, rNew))
	//May fail bc the operation performed on __readers cannot be done using only a Fetch..
}

bool writeLock(l){
//false : it will not remove the wrlocked it set to true if it fails
//maybe keep it that way try to prove it and see which additionnal specs we would need to forbid this
	if(curWriter == tid)
		return ERROR
	int r = FetchAndOr_Acq(__readers, WRLOCKED)
	if(r & WRLOCKED)
		return false;
	if(r & WRPHASE)
		return true;
	if(r & WRPHASE == 0 && r / 4 == 0)
		return (CAS_Acq(__readers, r, r | WRPHASE))
	return false;
}


bool writeUnlock(l){
	Store_Rlx(curWriter, 0)
	r = Load_Rlx(__readers)
	int rNew = r ^ WRLOCKED
	if(r / 8 > 0)
		rNew ^= WRPHASE
	return (CAS_Rel(__readers, r, rNew))
}
//again can fail bc more bookkeeping to do so need a compare and swap, not just a Fetc...
