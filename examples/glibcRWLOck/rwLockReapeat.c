__reader : offest of 2 (WRLOCKED, WRPHASE)

//repeat syntax: repeatedly evaluate expression until it evaluates to false

int readLock(l){
	int r = FetchAndAdd_Acq(__readers, 4) + 4
	repeat getsInReadPhase end
}

//assume this was inlined, I separated it for syntax clarity
bool getsInReadPhase(int *r){
	if(r & WRPHASE){
		if(r & WRLOCKED){
			r = Load_rlx(__readers)
			r & WRPHASE
		}
		else{
			t = CAS_Acq(__readers, &r, r^WRPHASE)
			!t && (r & WRPHASE)
		}
	}
	else
		false
}

int readUnlock(){
	int r = Load_rlx(r)
	repeat !CAS_Rel(__readers, &r, newRReadUnlock(r)) end
}

int newRReadUnlock(int r){
	if(r == 1 && (r & WRLOCKED))//we need to swap to write phase
		return (r - 4) | WRPHASE
	return r - 4
}

int writeLock(){
	int r = FetchAndOr_Acq(r, WRLOCKED)
	if(r & WRLOCKED){
		repeat getWrlocked(&r) end
		r |= WRLOCKED
	}
	if(r & WRPHASE) {done = true}
	repeat canSetWrPhase(&r) end
	repeat waitForWrPhase(&r) end
}

bool getWrlocked(int * r){
	if(r & WRLOCKED == 0)
		!CAS_Acq(__readers, r, r|WRLOCKED)
	else{
		r = Load_rlx(__readers)
		true
}

bool canSetWrPhase(r){
	if(r & WRPHASE == 0){
		if(r / 4 == 0)
			!CAS_Acq(__readers, r, r|WRPHASE)
		else
			false
	}
	else
		false
}

bool waitForWrPhase(r){
	r = Load_rlx(__readers)
	return (r & WRPHASE == 0)
}

int writeUnlock(){
	int r = Load_rlx(__readers)
	repeat !CAS(__readers, r, newRWrite(r)) end
}

int newRWriteUnlock(r) {
	if(r / 4 != 0)
		return r ^ WRLOCKED ^ WRPHASE
	else
		return r ^ WRLOCKED
}
