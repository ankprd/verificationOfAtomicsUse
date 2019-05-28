Variables : 
__reader : offset of 3
__wrphase_futex
__curWriter

int readLock(l){
	if(curWriter == tid)
		return ERROR
	int r = fetchAndAdd_Acq(__readers, 1) + 1
	if(r & WRPHASE)
		return 0
	while(r & WRPHASE != 0 && r & WRLOCKED == 0){
		if(CAS_Acq(__readers, &r, r ^ WRPHASE)){
			Store_Rlx(__wrphase_futex, 0)
			return 0;
		}
	}
	int wpf = Load_Rlx(__wrphase_futex)
	bool ready = false
	while(1){
		while(wpf == 1)
			wpf = Load_Rlx(__wrphase_futex)
		if(ready)
			break
		if(Load_Acq(__readers) & WRPHASE == 0)
			ready = true
	}
}

int readUnlock(l){
	int r = Load_Rlx(__readers)
	while(1){
		int rNew = r - 1
		if(rNew == 0){
			if(rNew & WRLOCKED)
				rNew |= WRPHASE
		}
		if(CAS_Rel(__readers, r, rNew))
			break
	}
	if(rNew & WRPHASE){
		Fence_Acq
		Store_Rlx(__wrphase_futex, 1)
	}
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
			int wf = Load_Rlx(__writers_futex)
			while(wf == 1) {wf = Load_Rlx(__writers_futex)}
			r = Load_Rlx(__readers)
		}
		r |= WRLOCKED
	}
	Store_Rlx(__writers_futex, 1)
	if(r & WRPHASE)
		goto done
	while(r & WRPHASE == 0 && r / 8 == 0){
		if(CAS(__readers, r, r | WRPHASE)){
			Store_Rlx(__wrphase_futex, 1)
			goto done
		}
	}
	int wpf = Load_Rlx(__wrphase_futex)
	ready = false
	while(1){
		while(wpf == 0)
			wpf = Load_Rlx(__wrphase_futex) //le wake spine sur la val 0 de wrphase donc ok
		if(ready)
			break
		if(Load_Acq(__readers) & WRPHASE)
			ready = true
	}
	Store_Rlx(curWriter, tid)
	return 0;
}

int writeUnlock(l){
	Store_Rlx(curWriter, 0)
	Store_Rlx(__writers_futex, 0)
	r = Load_Rlx(__readers)
	while(1){
		int rNew = r ^ WRLOCKED
		if(r / 8 > 0)
			rNew ^= WRPHASE
		if(CAS(__readers, r, rNew))
			break;
	}
	if(r > 0)
		Store_Rlx(__wrphase_futex, 0)
}
