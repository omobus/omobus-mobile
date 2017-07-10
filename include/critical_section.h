// critical_section.h - interface for critical_section,
//	dyn_critical_section and auto_lock classes

#ifndef __CRITICAL_SECTION_HEADER_VER_1_1_0_H__
#define __CRITICAL_SECTION_HEADER_VER_1_1_0_H__

#ifndef __cplusplus
# error Must use C++ for CRITICAL_SECTION.H
#endif

#include <windows.h>

//-----------------------------------------------------------------
// критическая секция
class critical_section
{	// ##P-S
  public:
  	critical_section()
    	{ ::InitializeCriticalSection(&cs_); }

  	~critical_section()
    	{ ::DeleteCriticalSection(&cs_); }

  	void Lock()
    	{ ::EnterCriticalSection(&cs_); }

  	void Unlock()
    	{ ::LeaveCriticalSection(&cs_); }

  	bool TryEnter()
    	{ return (::TryEnterCriticalSection(&cs_)!=0); }

  protected:
  	CRITICAL_SECTION cs_;

  private:
  	critical_section& operator=(const critical_section&) {return *this;};
	  // запрещаем копирование
};

//-----------------------------------------------------------------
// динамическая критическая секция (для использования в контейнерах)
class dyn_critical_section
{
  public:
  	dyn_critical_section()
  	{ cs_ = new critical_section; }

  	~dyn_critical_section()
  	{
	    if (cs_)
  		{
	    	delete cs_;
  			cs_ = NULL;
	    }
  	}

  	void Lock()
  	{
	    if (cs_ == NULL) throw 1;
  		cs_->Lock();
  	}

  	void Unlock()
  	{
  		if (cs_ == NULL) throw 1;
	      cs_->Unlock();
  	}

  	bool TryEnter()
  	{
      bool b = false;
	    if (cs_ == NULL) throw 1;
    		b = cs_->TryEnter();
      return b;
  	}

  	dyn_critical_section& operator=(const dyn_critical_section& s)
  	{
	    cs_ = s.cs_;
  		const_cast<dyn_critical_section&>(s).cs_ = NULL;
	  	// только один указатель допустим !
  		return *this;
  	}

  protected:
  	critical_section* cs_;
};

//-----------------------------------------------------------------
// защелка для критической секции
// деблокируем в деструкторе,
// блокируем (опционально) в конструкторе
template<class T>
class auto_lock
{
  public:
  	auto_lock(T& cs, bool lock_now = true) : cs_(cs), nlocks_(0)
  	{ if (lock_now) Lock(); }

  	~auto_lock()
  	{
	    for (long i = 0; i < nlocks_; ++i)
  			cs_.Unlock();
  	}

  	void Lock()
  	{ cs_.Lock(); ++nlocks_; }

  	void Unlock()
  	{
  		cs_.Unlock();
	    if (nlocks_ > 0) --nlocks_;
  	}

  private:
  	T& cs_;
  	long nlocks_;
};

typedef auto_lock<critical_section> s_guard;
typedef auto_lock<dyn_critical_section> s_dynguard;


#endif // __CRITICAL_SECTION_HEADER_VER_1_0_H__
