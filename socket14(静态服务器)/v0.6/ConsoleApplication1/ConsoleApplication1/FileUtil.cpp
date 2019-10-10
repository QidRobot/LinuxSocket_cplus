#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(std::string filename) : fp_(fopen(filename.c_str(), "ae"))
{
	//�ú���������stdio.h��ͷ�ļ��� �ǻ���linuxϵͳ��
	//void setbuffer(FILE * stream, char * buf, size_t size);
	/*�ڴ��ļ�����, ��ȡ����֮ǰ, ����setbuffer()�����������ļ����Ļ�����. 
	����stream Ϊָ�����ļ���, ����buf ָ���Զ��Ļ�������ʼ��ַ, ����size Ϊ��������С.*/
	setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
	fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
	size_t n = write(logline, len);
	size_t remain = len - n;
	//ֱ����logline�е�����д��Ϊֹ
	while (remain > 0)
	{
		size_t x = write(logline + n, remain);
		if (x == 0)
		{
			int err = ferror(fp_);
			if (err)
			{
				fprintf(stderr, "AppendFile::append() failed! \n");
			}
			break;
		}
		n += x;
		remain = len - n;
	}
}

void AppendFile::flush()
{
	fflush(fp_);
}

size_t AppendFile::write(const char* logfile, const size_t len)
{
	// #undef fwrite_unlocked
	return ::fwrite_unlocked(logfile, 1, len, fp_);
}