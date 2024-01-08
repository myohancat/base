/**
 * My simple network util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __PROCESS_UTIL_H_
#define __PROCESS_UTIL_H_

namespace ProcessUtil
{

int get_pid(const char* process);
int get_pid_from_proc_by_name(const char* process);

int kill(const char* process);
int kill_force(const char* process);

int system(const char* command);

} // namepspace ProcessUtil

#endif /* __PROCESS_UTIL_H_ */
