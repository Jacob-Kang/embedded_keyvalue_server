int processEvents(aeEventLoop* eventLoop, int flags) {
	int processed = 0, numEvents = 0;

	if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;
