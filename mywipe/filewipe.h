#ifndef __FILE_WIPE_H__
#define __FILE_WIPE_H__

/* return codes */
#define SUCCESS 0
#define FAILED -1
#define NOT_SUPPORT_TYPE -2
#define PATH_NAME_TOO_LONG -3
#define STAT_FAILED -4
#define WIPE_WRITE_FAILED -5

//The parameter name should be an absolute path
int do_file(const char *name);

#endif // __FILE_WIPE_H__
