/*
��Logger������2���ڲ���SourceFile��Impl��
��SouceFile����ȷ����־�ļ������֣���Impl������ʵ����־����ĵط���
��Logger���п�������Logger��־�����Լ����û������ջ��溯����
*/
#pragma once
#include "LogStream.h"
#include <pthread.h>
#include <string.h>
#include <string>
#include <stdio.h>
class AsyncLogging;
class Logger
{
public:
	~Logger();
	Logger(const char *fileName, int line);
	//LogStream������ << ���������˿���ֱ��ʹ��
	LogStream& stream()
	{
		return impl_.stream_;
	}

private:
	class Impl
	{
	public:
		Impl(const char *fileName, int line);
		void formatTime();

		LogStream stream_;
		int line_;
		std::string basename_;
	};
	Impl impl_;
};
/*
LogStream.h������Buffer���LogStream�ࡣBuffer���ǻ��棬��־�Ȼ��浽Buffer�У�֮���������
LogStream�����buffer����Ҫʵ�ָ����������ݵ��ַ�����ת��������������ӵ�buffer�С�
���ʱʹ��append()�����������־̫�������޷���ӣ�����ÿ����־������ǹ�����һ�������࣬��������־���ӵ�һ����������⡣
*/
#define LOG Logger(__FILE__, __LINE__).stream()