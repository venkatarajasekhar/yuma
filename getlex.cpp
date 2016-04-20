#include <iostream>
#include <string.h>
#include <ctype.h>
#include <vector>
#include "getlex.hpp"

using namespace std;

bool Identifier::Set (const string &s)
{
  if (type == undef_type || type == string_type)
    {
      type = string_type;
      val.s = new string(s);
      return true;
    }
  else
    return false;
}

bool Identifier::Set (double n)
{
  if (type == undef_type || type == number_type)
    {
      type = number_type;
      val.n = n;
      return true;
    }
  else
    return false;
}

bool Identifier::TryGetVal (string &s)
{
  if (type != string_type) return false;
  
  s = *(val.s);
  return true;
}

bool Identifier::TryGetVal (double &n)
{
  if (type != number_type) return false;
  
  n = val.n;
  return true;
}

ostream& operator<< (ostream &out, const Identifier &id)
{
  out << id.name;
  return out;
}


const char *const Interpreter::Operators[] = {"==", "!=", "&&", "||", "+", "-", "*", "/", "=", "(", ")", "{", "}", ";", "<", ">", "<=", ">="};
const char *const Interpreter::KeyWords[] = {"for", "while", "if", "else", "print", "get"};

char* Interpreter::GetOperator(char *s, int &num)
{
  static int size = sizeof(Operators)/sizeof(char *);
  
  char *result = NULL;
  unsigned int len;

  for (int i = 0; i < size; i++)
    {
      char *tmp = strstr(s, Operators[i]);
      
      if (tmp)
	if (
	    !result ||
	    result > tmp ||
	    ( result == tmp && len < strlen(Operators[i]) )
	    ) //Bad-bad check :( 
	  {
	    result = tmp;
	    num = i;
	    len = strlen(Operators[num]);
	  }
    }

  return result;
}

bool Interpreter::TryGetNum(const char *s, double &d)
{
  if (!s || !s[0]) return false;

  double res = 0, frac = 1;
  bool got_dot = false;
  
  for (int i = 0; s[i]; i++)
    if (isdigit(s[i]))
      {
	res = 10 * res + (s[i] - '0');
	if (got_dot) frac *= 10;
      }
    else if ('.' == s[i])
      {
	if (!got_dot) got_dot = true;
	else return false;
      }
    else return false;

  d = res/frac;
  return true;
}

int Interpreter::GetKWNum (const char *s)
{
  static int size = sizeof(KeyWords)/sizeof(char *);

  for (int i = 0; i < size; i++)
    if ( 0 == strcmp(s, KeyWords[i]) )
      return i;

  return -1; 
}

bool Interpreter::IsIdentifier (const char *s)
{
  if (!s || !s[0]) return false;

  if ( !(isalpha(s[0]) || s[0] == '_') ) return false;

  for (int i = 1; s[i]; i++)
    if ( !(isalnum(s[i]) || s[i] == '_') ) return false;

  return true;
}

void Interpreter::TryProcess (const char *s)
{
  const char * const IDENT_ERR = "Bad identifier";
  const char * const NUM_ERR = "Bad number";

  int n = GetKWNum(s);

  if (-1 != n)
    Lexemes.push_back( Lexeme(kw, n) );
  
  else if ( isdigit(s[0]) || '.' == s[0] )
    {
      double res;
      if (TryGetNum(s, res))
	{
	  Numbers.push_back(res); //add a number
	  Lexemes.push_back( Lexeme(num, Numbers.size() - 1) );
	}
      else throw NUM_ERR;
    }
  
  else if (IsIdentifier(s))
    {
      Identifiers.push_back( Identifier(s) ); //add an identifier
      Lexemes.push_back( Lexeme(id, Identifiers.size() - 1) );
    }
  
  else throw IDENT_ERR;
}

void Interpreter::GetLex (char *s)
{
  #define SPACE_TOKENS " \n\t"
  
  const char * const COMMA_ERR = "Not paired commas";
  
  //the commas and comments check
  if (s)
    {
      char *comma_start, *comma_end, *comment_start;
  
      comment_start = strstr(s, "#");
      comma_start = strstr(s, "\"");

      if (comment_start)
	{
	  if (!comma_start || comment_start < comma_start)
	    {
	      *comment_start = '\0';
	      
	      GetLex(s);
	      
	      return;
	    }
	}
      
      if (comma_start) //no comment - check commas
	{ 
	  if ( !(comma_end = strstr(comma_start + 1, "\"")) ) throw COMMA_ERR;

	  *comma_start = '\0';
	  *comma_end = '\0';

	  GetLex(s); //interprete the first part
      
	  Strings.push_back(comma_start + 1); //add a string
	  Lexemes.push_back( Lexeme(str, Strings.size() - 1) );
      
	  GetLex(comma_end + 1); //interprete the last part

	  return;
	}
    }

  //split the string into space-tokens, get a long word

  char *long_word = strtok(s, SPACE_TOKENS);
  if (long_word == NULL) return;  

  //now make words from a long word

  char *cur_pos = long_word;
  char *next_pos;
  int num;

  //process words in cycle while there are operators in the long word
  while ( (next_pos = GetOperator(cur_pos, num)) )
    {
      if ( next_pos != cur_pos )
	{
	  //get a word
	  int n = next_pos - cur_pos;
	  char buf [n + 1];
	  strncpy(buf, cur_pos, n);
	  buf[n] = '\0';
      
	  //try to process it
	  try
	    {
	      TryProcess(buf);
	    }
	  catch (...)
	    {
	      throw;
	    }
	}
      
      Lexemes.push_back( Lexeme(op, num) );

      cur_pos = next_pos + strlen(Operators[num]); //move to the next position
    }
  
  //process the rest
  if (*cur_pos)
    {
      try
	{
	  TryProcess(cur_pos);
	}
      catch (...)
	{
	  throw;
	}
    }

  //and the tail
  GetLex(NULL);
}

void Interpreter::PrintLex () const
{
  for (vector<Lexeme>::const_iterator it = Lexemes.begin(); it != Lexemes.end(); ++it) 
    {
      cout << "Table: ";

      switch (it->table)
	{
	case op:
	  cout << "Operators; Value: " << Operators[it->num];
	  break;

	case kw:
	  cout << "Key words; Value: " << KeyWords[it->num];
	  break;

	case id:
	  cout << "Identifiers; Value: " << Identifiers[it->num];
	  break;


	case str:
	  cout << "Strings; Value: " << Strings[it->num];
	  break;

	case num:
	  cout << "Numbers; Value: " << Numbers[it->num];
	  break;   
	}

      cout << endl;
    }
}
