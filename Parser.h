#include <iostream>

// �Ľ̿� ��� ����
#define MAX_PARSE_LINE		512			// ���Ͽ��� �о� ���� �� ���� �ִ� ����
#define MAX_KEYWORD_LEN		64			// �Ľ��� ���� Ű������ �ִ� ����
#define MAX_NAME_LEN		256			// ���� �̸����� �ִ� ����

typedef char ANYNAME[MAX_NAME_LEN];		// �̸� ����� ���� ����
typedef char KEYWORD[MAX_KEYWORD_LEN];	// Ű���� ����� ���� ����

// �Ľ̿� ���� ���� (extern)
extern char		TempString[MAX_PARSE_LINE];
extern float	ParseFloats[MAX_PARSE_LINE];

// �Լ� ���� ����
char*	StringTrim(char* sBuf);
char*	StringTrim_OBJ(char* sBuf);
char*	SplitString(char* String, char* SplitChar, int SplitIndex);
char*	GetNameBetweenBraces(char* val);
char*	GetNameBetweenQuotes(char* val);
bool	FindString(char* val, char* cmp);
bool	FindString_MD5(char* val, char* cmp);
bool	FindChar(char* val, char cmp[]);
void	GetFloatFromLine(char* line, char* split, char* splita);