#ifndef _ERRNO_H_
#define	_ERRNO_H_

#ifdef	__cplusplus
extern "C" {
#endif

int* __cdecl omcore_errno(void);
#define	errno		(*omcore_errno())

#ifdef	__cplusplus
}
#endif

#endif	/* Not _ERRNO_H_ */
