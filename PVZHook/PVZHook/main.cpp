#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

HANDLE hMondule;   // 保存popcapgame1.exe模块加载到进程地址空间的内存基址
BOOL g_bStopConsoleThread = FALSE;  // 控制台线程停止标志
bool g_bHookInstalled = FALSE;//记录是否已经装载钩子
DWORD g_returnAddress = 0;
DWORD g_OriginalZombieSpawn = 0;
char g_OriginalCode[6] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };

//内存写入函数
DWORD ChangeMemoryData(LPVOID Address, SIZE_T size, PVOID ptr)
{
   
    //获取修改地址的内存属性
    MEMORY_BASIC_INFORMATION mbi = { 0 };
    DWORD oldProtect = mbi.Protect;
    SIZE_T result = VirtualQuery((LPCVOID)Address, &mbi, sizeof(mbi));
    if (result == 0)
    {
        printf("VirtualQuery 失败，错误码: %lu\n", GetLastError());
        return 1;
    }
    //修改目标地址的内存属性
    if (mbi.Protect != PAGE_EXECUTE_READWRITE)
    {
        if (!VirtualProtect((LPVOID)Address, size, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            printf("修改内存属性失败，错误码: %lu\n", GetLastError());
            return 1;
        }
    }


    memcpy((void*)Address, ptr, size);//把数据复制到指定地址

    //还原内存属性属性
    DWORD temp;
    VirtualProtect((LPVOID)Address, size, oldProtect, &temp);
    return 0;
}



//修改阳光数值
void ModifySunshineValue()
{
    //获取要存储阳光的地址
    HANDLE hMondule = GetModuleHandleW(L"popcapgame1.exe");
    DWORD BaseAdress = (DWORD)hMondule + 0x002A9EC0;//基址
    //存储阳光的地址
    DWORD TrueAddress = *(DWORD*)((*(DWORD*)BaseAdress) + 0x768) + 0x5560;
    DWORD SunNum = 0;
    printf("输入要修改的阳光数量:  ");
    scanf("%d", &SunNum);

    //数据修改函数
    if (ChangeMemoryData((LPVOID)TrueAddress, 4, &SunNum))
    {
        printf("修改失败!\n");
        return;
    }
    printf("修改阳光成功\n");
    return;
}


//获取特征码,进行特征码匹配,查询僵尸生成函数
char AttributeCode[] =
{
  0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF0, 0x8B, 0x86, 0x98, 0x00, 
  0x00, 0x00, 0x83, 0xE8, 0x01, 0x39, 0x86, 0xA0, 0x00, 0x00,
  0x00, 0x57, 0x72, 0x0A
};
DWORD FindFunctionAddress(BYTE* BaseAddress, DWORD BaseSize, DWORD Offset, char* AttributeCode)
{
    
    for (int i = 0; i < BaseSize - sizeof(AttributeCode); i++)
    {
        if (memcmp(BaseAddress + i, AttributeCode, sizeof(AttributeCode)) == 0)
        {
            //printf("Find: %x\n", BaseAddress + i - Offset);
            return (DWORD)BaseAddress + i - Offset;
        }
    }
    return 0;
}

//在随机道路生成一只随机种类的僵尸
void CallZombieSpawn()
{
    //获取进程基地址
    HANDLE hMondule = GetModuleHandleW(L"popcapgame1.exe");
    //printf("%x\n",  hMondule);
    if (hMondule == NULL)
    {
        printf("GetModuleHandleW Fail, Error Code: %x\n", GetLastError());
        return;
    }

    //根据CE + x32dbg分析，我们知道这个Call有两个参数，同时eax也有值
    //两个参数分别是   生成的坐标   僵尸种类
    //eax根据分析后的结构可以判断为一个结构，可能和僵尸的初始化有关系。
    //获取僵尸类
    DWORD TrueAddress = (DWORD)(*(DWORD*)((DWORD)hMondule + 0x002A9EC0) + 0x768);
    DWORD Date = *(DWORD*)TrueAddress;
    //printf("%x %x\n", *(DWORD*)TrueAddress, hMondule);


    //DWORD para1ZombiePos = rand() % 5; //生成的坐标 0-5
    //DWORD para2ZombieType = rand() % 20;	// 僵尸种类
    DWORD para1ZombiePos = 0; //生成的坐标 0-5
    DWORD para2ZombieType = 0;	// 僵尸种类

    __asm
    {
        mov eax, para1ZombiePos;
        mov ebx, para2ZombieType;
        push eax;
        push ebx;
        mov eax, Date;
        mov ebx, g_OriginalZombieSpawn;
        call ebx;

    }
}



//跳板函数
void  __declspec(naked) HookFunc()
{
    __asm
    {
        mov dword ptr ds : [esp + 8] , 2; //僵尸生成位置
        mov dword ptr ds : [esp + 4] , 0;//生成僵尸的编号
        _emit 0x55;
        _emit 0x8B;
        _emit 0xEC;
        _emit 0x83;
        _emit 0xE4;
        _emit 0xF8;
        jmp g_returnAddress;
    }
}
void InstallInlineHook()
{
    if (g_bHookInstalled == TRUE)
    {
        printf("警告: 钩子已安装!\n");
        return;
    }
    g_returnAddress = g_OriginalZombieSpawn + 6;
    DWORD jmp_size = 5;
    //计算偏移量
    DWORD value = (DWORD)HookFunc - (g_OriginalZombieSpawn + 5);
    char buffer[6] = { 0 };
    buffer[0] = 0xE9;
    buffer[5] = 0x90;
    *(DWORD*)&buffer[1] = value;

    if (ChangeMemoryData((LPVOID)g_OriginalZombieSpawn, jmp_size, buffer))
    {
        printf("修改失败!");
        return;
    }
    g_bHookInstalled = TRUE;
    printf("钩子装载成功!\n");
    printf("僵尸只能出现在中路\n");
}


void UninstallInlineHook()
{
    if (g_bHookInstalled != TRUE)
    {
        printf("警告: 钩子尚未安装!\n");
        return;
    }
    if (ChangeMemoryData((LPVOID)g_OriginalZombieSpawn, sizeof(g_OriginalCode), g_OriginalCode))
    {
        printf("修改Hook失败!");
        return;
    }
    g_bHookInstalled = FALSE;
    printf("卸载钩子成功!\n");
}


void ModifyMoneyValue()
{
    HANDLE hMondule = GetModuleHandleW(L"popcapgame1.exe");
    DWORD BaseAddress = (DWORD)hMondule + 0x002A9EC0;//基址 //0x2A9EC0
    //存储金币的地址
    DWORD TrueAddress = *(DWORD*)(*(DWORD*)BaseAddress + 0x82C) + 0x28;

    DWORD Money = 0;
    printf("输入要修改的金币数量: \n");
    scanf("%d", &Money);
    if (ChangeMemoryData((LPVOID)TrueAddress, 4, &Money))
    {
        printf("修改失败!\n");
        return;
    }
    printf("金币修改成功\n");
    return;
}


DWORD WINAPI MyThreadFunction(LPVOID lpParam) {

    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    // 可选：同时重定向错误和输入流
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);


    hMondule = GetModuleHandleW(L"popcapgame1.exe");//保存模块的内存基址


    //初始化获得僵尸函数
    PIMAGE_OPTIONAL_HEADER pOp = (PIMAGE_OPTIONAL_HEADER)((DWORD)hMondule + sizeof(IMAGE_DOS_HEADER) + 4 + sizeof(IMAGE_FILE_HEADER));
    DWORD SizeOfImage = pOp->SizeOfImage;//文件在内存中的大小
    DWORD Offset = 6;  //根据我们选取的特征码分析得出偏移量
    g_OriginalZombieSpawn = FindFunctionAddress((BYTE*)hMondule, SizeOfImage, Offset, AttributeCode);; //通过特征码查询 僵尸生成函数

    DWORD cmd;
    printf("\n[1]阳光 [2]金币 [3]生成僵尸 [4]安装Hook [5]卸载Hook\n");
    while (!g_bStopConsoleThread)
    {
        printf("请选择: ");
        scanf("%d", &cmd);
        switch (cmd)
        {
            case 1:
            {
                ModifySunshineValue();//修改阳光数量
                break;
            }
            case 2:
            {
                ModifyMoneyValue();//修改金币数量//在一路生成僵尸
                break;
            }
            case 3:
            {
                CallZombieSpawn();//掉用僵尸生成函数
                printf("僵尸生成！\n");
                break;
            }
            case 4:
            {
                InstallInlineHook();//给僵尸生成函数挂钩子
                break;
            }
            case 5:
            {
                UninstallInlineHook();//卸载钩子
                break;
            }
            case 6:
            {
                //FindZombieSpawnAddress();
                break;
            }
            default:
            {
                printf("无效命令\n");
                break;
            }
        }
        Sleep(50);
    }

    return 0;
}


BOOL  __stdcall DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE hThread = CreateThread(NULL, 0, MyThreadFunction, NULL, 0, NULL);
        if (hThread) CloseHandle(hThread);
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        g_bStopConsoleThread = TRUE;
        Sleep(200);
        FreeConsole();
        break;
    }
      
    }
    return TRUE;
}
