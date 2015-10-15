


#define HC_NOUPDATE	1
#define HC_NOCOMPUTE	2
#define HC_RECOMPUTE	4
#define HC_DELETE	8

#define HC_RET_NOATTR	1
#define HC_RET_STALE	2
#define HC_RET_COMP	4


enum err_ret { ERR_SUCCESS, ERR_MISMATCH, ERR_USAGE, ERR_SYSTEM, ERR_NOFILE };

extern int hashcache(unsigned char digest[32], int fd, unsigned int flags);



