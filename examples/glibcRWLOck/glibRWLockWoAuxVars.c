Variables : 
__reader : offset of 3
__writers_futex
__wrphase_futex
__curWriter

//a try with no __writers and wrphase futex. Note that we do not have epxlicit handover anymore, so we assume the lock is never destroyed

int readLock(l){
	if(curWriter == tid)
		return ERROR
	int r = fetchAndAdd_Acq(__readers, 1) + 1
	if(r & WRPHASE)
		return 0
	while(r & WRPHASE != 0 && r & WRLOCKED == 0){
		if(CAS_Acq(__readers, &r, r ^ WRPHASE))
			return 0;
	}
	//there is a writer (maybe in waiting), it shall get us to write mode
	while(Load_Rlx(__readers) & WRPHASE == 0){;}
}

int readUnlock(l){
	int r = Load_Rlx(__readers)
	while(1){
		int rNew = r - 1
		if(rNew == 0){
			if(rNew & WRLOCKED)
				rNew |= WRPHASE
			rNew &= ~(RWaiting)
		}
		if(CAS_Rel(__readers, r, rNew))
			break
	}
	//Used to be a fence_acq but not necessary when we do not use the auxiliary futex I think
}

int writeLock(l){
	if(curWriter == tid)
		return ERROR
	int r = FetchAndOr_Acq(__readers, WRLOCKED)
	if(r & WRLOCKED){
		while(1){
			if(r & WRLOCKED == 0){
				if(CAS_Acq(__readers, r, r | WRLOCKED))
					break
				continue
			}
			r = Load_Rlx(__readers)
		}
		r |= WRLOCKED
	}
	if(r & WRPHASE)
		goto done
	while(r & WRPHASE == 0 && r / 8 == 0){
		if(CAS(__readers, r, r | WRPHASE)){
			goto done
		}
	}
	int wpf = Load_Rlx(__wrphase_futex)
	ready = false
	while(Load_Acq(__readers) & WRPHASE == 0) {;}
	Store_Rlx(curWriter, tid)
	return 0;
}

int writeUnlock(l){
	Store_Rlx(curWriter, 0)
	r = Load_Rlx(__readers)
	while(1){
		int rNew = r ^ WRLOCKED
		if(r / 8 > 0)
			rNew ^= WRPHASE
		if(CAS(__readers, r, rNew))
			break;
	}
}
