/*hpp����ʵ�ʾ��ǽ�.cpp��ʵ�ִ������.hͷ�ļ����У�������ʵ�ֶ�������ͬһ�ļ��������ĵ�����ֻ��Ҫinclude��hpp�ļ����ɣ�
������ ��cpp���뵽project�н��б��롣��ʵ�ִ��뽫ֱ�ӱ��뵽�����ߵ�obj�ļ��У��������ɵ�����obj,����hpp������ȼ��ٵ���
project�е�cpp�ļ�������������Ҳ�����ٷ������˵�lib��dll,��˷ǳ��ʺ�������д���õĿ�Դ�⡣*/

//�����ǿ������� �Ǹ�ֵ���� ������̳и��༴���и��๦��  �ο���˶��linux���̷߳������˱��
#pragma once
class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable(const noncopyable&);
	const noncopyable& operator=(const noncopyable&);
};