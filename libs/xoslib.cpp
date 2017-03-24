#include "XsriptVM.h"
#include "xoslib.h"
#include <vector>
#include <string>

void init_os_lib()
{

	std::vector<HostFunction> osfuncVec;
	osfuncVec.push_back(HostFunction("listdir", xos_listdir));
	osfuncVec.push_back(HostFunction("remove", xos_remove));
	osfuncVec.push_back(HostFunction("rmdir", xos_rmdir));
	osfuncVec.push_back(HostFunction("mkdir", xos_mkdir));
	osfuncVec.push_back(HostFunction("isfile", xos_isfile));
	osfuncVec.push_back(HostFunction("exists", xos_exists));
	osfuncVec.push_back(HostFunction("getsize", xos_getsize));
	osfuncVec.push_back(HostFunction("getpwd", xos_getpwd));
	osfuncVec.push_back(HostFunction("setpwd", xos_setpwd));
	osfuncVec.push_back(HostFunction("system", xos_system));
	gScriptVM.RegisterHostLib("os", osfuncVec);
}

void GenerateFileList(const std::string& dir, std::vector<std::string>& fileList)
{
	HANDLE file;
	WIN32_FIND_DATA fileData;
	std::string curDir = dir;
	curDir += "/*";

	file = FindFirstFile(curDir.c_str(), &fileData);

	while (FindNextFile(file, &fileData))
	{
		if (strcmp(fileData.cFileName, ".") == 0 ||
			strcmp(fileData.cFileName, "..") == 0)
		{
			continue;
		}

		int isDir = fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (isDir != 0)
		{
			std::string subDir = dir;
			subDir += "/";
			subDir += fileData.cFileName;
			fileList.push_back(subDir);
			std::vector<std::string> fileList;
			GenerateFileList(subDir, fileList);
		}
		else
		{
			std::string filePath = dir;
			filePath += "/";
			filePath += fileData.cFileName;
			fileList.push_back(filePath);
		}
	}
}

bool DeleteDir(const std::string& dir)
{
	HANDLE file;
	WIN32_FIND_DATA fileData;
	std::string curDir = dir;
	curDir += "/*";

	file = FindFirstFile(curDir.c_str(), &fileData);

	while (FindNextFile(file, &fileData))
	{
		if (strcmp(fileData.cFileName, ".") == 0 ||
			strcmp(fileData.cFileName, "..") == 0)
		{
			continue;
		}

		int isDir = fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (isDir != 0)
		{
			std::string subDir = dir;
			subDir += "/";
			subDir += fileData.cFileName;
			DeleteDir(subDir);
		}
		else
		{
			std::string filePath = dir;
			filePath += "/";
			filePath += fileData.cFileName;
			DeleteFile(filePath.c_str());
		}
	}

	return RemoveDirectory(dir.c_str());
}

void xos_listdir(XScriptVM* vm)
{
	CheckParam(xos_listdir, 0, dirName, OP_TYPE_STRING);
	std::vector<std::string> dirVec;
	GenerateFileList(stringRawValue(&dirName), dirVec);
	if (dirVec.size() > 0)
	{
		TABLE table = vm->newTable();
		for (int i = 0; i < (int)dirVec.size(); i++)
		{
			vm->setTableValue(table, i, dirVec[i].c_str());
		}

		vm->setReturnAsTable(table);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void xos_getpwd(XScriptVM* vm)
{
	char pwd[256] = { 0 };
	GetCurrentDirectory(256, pwd);
	vm->setReturnAsStr(pwd);
}

void xos_setpwd(XScriptVM* vm)
{
	CheckParam(xos_setpwd, 0, pwd, OP_TYPE_STRING);
	SetCurrentDirectory(stringRawValue(&pwd));
}

void xos_system(XScriptVM* vm)
{
	CheckParam(xos_system, 0, cmd, OP_TYPE_STRING);
	system(stringRawValue(&cmd));
}

void xos_remove(XScriptVM* vm)
{
	CheckParam(xos_remove, 0, fileName, OP_TYPE_STRING);
	vm->setReturnAsInt(DeleteFile(stringRawValue(&fileName)));
}

void xos_rmdir(XScriptVM* vm)
{
	CheckParam(xos_rmdir, 0, dir, OP_TYPE_STRING);
	vm->setReturnAsInt(DeleteDir(stringRawValue(&dir)));

}

void xos_mkdir(XScriptVM* vm)
{
	CheckParam(xos_mkdir, 0, dir, OP_TYPE_STRING);
	vm->setReturnAsInt(CreateDirectory(stringRawValue(&dir), NULL));
}

void xos_isfile(XScriptVM* vm)
{
	CheckParam(xos_isfile, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE && !(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

void xos_isdir(XScriptVM* vm)
{
	CheckParam(xos_isdir, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE && (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

void xos_exists(XScriptVM* vm)
{
	CheckParam(xos_exists, 0, fileName, OP_TYPE_STRING);
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(stringRawValue(&fileName), &fileData);
	vm->setReturnAsInt(file != INVALID_HANDLE_VALUE);
}

void xos_getsize(XScriptVM* vm)
{
	CheckParam(xos_getsize, 0, fileName, OP_TYPE_STRING);
	struct _stat info;
	_stat(stringRawValue(&fileName), &info);
	vm->setReturnAsInt(info.st_size);
}

