#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/syscall.h>
#include <spdlog/spdlog.h>
#include <sstream>

#include "util.h"

namespace amot {

static auto logger = spdlog::get("system");

long GetThreadId()
{
	return syscall(SYS_gettid);
}

// uint32_t GetFiberId()
// {
// 	return Fiber::GetFiberId();
// }

uint64_t GetCurrentMS()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

// 不要在栈上分配很大的内存空间
void Backtrace(std::vector<std::string>& bt, int size, int skip)
{
	void **array = (void **)malloc((sizeof(void *) * size));
	size_t s = ::backtrace(array, size);

	char **strings = backtrace_symbols(array, s);
	if(strings == NULL)
	{
		logger->error("backtrace_synbols error");
		return;
	}

	for (size_t i = skip; i < s; ++i)
	{
		bt.push_back(strings[i]);
	}

	free(strings);
	free(array);
}

std::string BacktraceToString(int size, int skip, const::std::string& prefix)
{
	std::vector<std::string> bt;
	Backtrace(bt, size, skip);
	std::stringstream ss;
	for (size_t i = 0; i < bt.size(); ++i)
	{
		ss << prefix << bt[i] << std::endl;
	}
	return ss.str();
}
}