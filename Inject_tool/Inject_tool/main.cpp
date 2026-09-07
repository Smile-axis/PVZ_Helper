#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>  // 包含进程快照API
#include <string.h>

char DllPath[MAX_PATH] = "PVZHook.dll";



// 通过进程名获取进程句柄
// 参数: lpProcessName - 进程名(例如 "notepad.exe")
// 返回: 成功返回进程句柄，失败返回NULL
HANDLE GetProcessHandleByName(LPCWSTR lpProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	// 开始遍历第一个进程
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// 循环遍历所有进程
	do {
		// 比较进程名 (不区分大小写)
		if (_wcsicmp(pe32.szExeFile, lpProcessName) == 0) {
			// 找到目标进程，通过PID获取句柄
			// 注意: 这里请求了 PROCESS_ALL_ACCESS 权限，根据实际需要可调整[reference:13]
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
			CloseHandle(hSnapshot); // 关闭快照句柄
			return hProcess;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return NULL; // 未找到进程
}

int main()
{
	// 获取目标进程的句柄
	HANDLE A_Handle = GetProcessHandleByName(L"popcapgame1.exe");
	if (A_Handle == NULL) {
		printf("未找到进程或打开失败，错误码: %lu\n", GetLastError());
		system("pause");
		return 1;
	}

	//2.申请远程内存空间
	LPVOID ptr = VirtualAllocEx(A_Handle, NULL, strlen(DllPath) + 1, MEM_COMMIT, PAGE_READWRITE); //线程函数的参数

	//3.路径字符写到我们申请的内存
	WriteProcessMemory(A_Handle, ptr, DllPath, strlen(DllPath) + 1, NULL);

	//4.创建远程线程
	CreateRemoteThread(A_Handle, //目标进程A的句柄
		NULL,	//默认的安全描述符
		0,		//堆栈使用默认大小
		(LPTHREAD_START_ROUTINE)LoadLibraryA,      //线程函数
		ptr,	//线程函数的参数
		0,		//创建后立刻执行
		NULL);  //不返回线程标识符
	

	return 0;
}

