Variables : 
__reader : offset of 3

int readLock(){
	int r = fetchAndAdd_Acq(__readers, 1) + 1
	if(r & WRPHASE == 0)
		return 0
	while(r & WRPHASE != 0 && r & WRLOCKED == 0){
		if(CAS_Acq(__readers, &r, r ^ WRPHASE))
			return 0;
	}
	//there is a writer (maybe in waiting), it will get us to read mode at some point
	while(Load_Rlx(__readers) & WRPHASE == 0){;}
}

int readUnlock(){
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
}

int writeLock(){
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
		return 0
	while(r & WRPHASE == 0 && r / 8 == 0){
		if(CAS_Acq(__readers, r, r | WRPHASE)){
			return 0
		}
	}
	while(Load_Rlx(__readers) & WRPHASE == 0) {;}
	return 0;
}

int writeUnlock(l){
	r = Load_Rlx(__readers)
	while(1){
		int rNew = r ^ WRLOCKED
		if(r / 8 > 0)
			rNew ^= WRPHASE
		if(CAS_Rel(__readers, r, rNew))
			break;
	}
}
