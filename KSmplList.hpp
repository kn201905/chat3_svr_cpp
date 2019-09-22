#pragma once

template<typename T>
struct  KSmplListElmt : public T
{
	KSmplListElmt<T>*  m_pPrev = NULL;
	KSmplListElmt<T>*  m_pNext = NULL;
};

// -------------------------------------------------------------

template<typename T_Elmnt, size_t size>
class  KSmplList
{
	friend class  KServer;

public:
	KSmplList<T_Elmnt, size>()
	{
		T_Elmnt*  pelmt = ma_Elmts;
		for (int i = 0; i < size; ++i)
		{
			pelmt->m_pPrev = pelmt - 1;
			pelmt->m_pNext = pelmt + 1;
			pelmt++;
		}
		m_pBegin->m_pPrev = NULL;
		m_pEnd->m_pNext = NULL;
	}

	// 次の要素がない場合、NULLが返される
	T_Elmnt*  GetNext()
	{
		if (m_pNext == NULL) { return NULL; }

		T_Elmnt*  ret_val = m_pNext;
		m_pNext = m_pNext->m_pNext;

		return  ret_val;
	}

	void  MoveToEnd(T_Elmnt* pelmt)
	{
		if (pelmt->m_pPrev == NULL)  // pelmt が先頭の場合
		{
			m_pBegin = pelmt->m_pNext;
			m_pBegin->m_pPrev = NULL;

			m_pEnd->m_pNext = pelmt;
			pelmt->m_pPrev = m_pEnd;
			pelmt->m_pNext = NULL;
			m_pEnd = pelmt;
		}
		else if (pelmt->m_pNext != NULL)  // pelmt が中間要素である場合
		{
			T_Elmnt* const  pprev = pelmt->m_pPrev;
			T_Elmnt* const  pnext = pelmt->m_pNext;
			pprev->m_pNext = pnext;
			pnext->m_pPrev = pprev;

			m_pEnd->m_pNext = pelmt;
			pelmt->m_pPrev = m_pEnd;
			pelmt->m_pNext = NULL;
			m_pEnd = pelmt;
		}
		// pelmt が pEnd である場合は、何もしなくてよい

		if (m_pNext == NULL) { m_pNext = m_pEnd; }
	}

private:
	T_Elmnt  ma_Elmts[size];
	T_Elmnt*  m_pBegin = ma_Elmts;
	T_Elmnt*  m_pEnd = &ma_Elmts[size - 1];
	T_Elmnt*  m_pNext = m_pBegin;  // リストがフルになっている場合は NULL となる
};


// -------------------------------------------------------------
// このクラスは、T_Base に KSmplListElmt を被せたものを自動的に生成する

template<typename T_Base, size_t size>
class  KSmplList2
{
	friend class  KServer;

public:
	KSmplList2<T_Base, size>()
	{
		KSmplListElmt<T_Base>*  pelmt = ma_Elmts;
		for (int i = 0; i < size; ++i)
		{
			pelmt->m_pPrev = pelmt - 1;
			pelmt->m_pNext = pelmt + 1;
			pelmt++;
		}
		m_pBegin->m_pPrev = NULL;
		m_pEnd->m_pNext = NULL;
	}

	// リストが空の場合は、NULL が返される
	// リストの要素の順次取得は、KSmplListElmt の m_pNext を用いれば良い
	KSmplListElmt<T_Base>*  Get_FirstEmnt()
	{
		if (m_pNewElmt_inList == m_pBegin) { return  NULL; }
		return  m_pBegin;
	}

	// バッファがフルで新しい要素を返せない場合、NULLが返される
	KSmplListElmt<T_Base>*  Get_NewElmt()
	{
		if (m_pNewElmt_inList == NULL) { return NULL; }

		KSmplListElmt<T_Base>*  ret_val = m_pNewElmt_inList;
		m_pNewElmt_inList = m_pNewElmt_inList->m_pNext;

		return  ret_val;
	}

	void  MoveToEnd(KSmplListElmt<T_Base>* pelmt)
	{
		if (pelmt->m_pPrev == NULL)  // pelmt が先頭の場合
		{
			m_pBegin = pelmt->m_pNext;
			m_pBegin->m_pPrev = NULL;

			m_pEnd->m_pNext = pelmt;
			pelmt->m_pPrev = m_pEnd;
			pelmt->m_pNext = NULL;
			m_pEnd = pelmt;
		}
		else if (pelmt->m_pNext != NULL)  // pelmt が中間要素である場合
		{
			KSmplListElmt<T_Base>* const  pprev = pelmt->m_pPrev;
			KSmplListElmt<T_Base>* const  pnext = pelmt->m_pNext;
			pprev->m_pNext = pnext;
			pnext->m_pPrev = pprev;

			m_pEnd->m_pNext = pelmt;
			pelmt->m_pPrev = m_pEnd;
			pelmt->m_pNext = NULL;
			m_pEnd = pelmt;
		}
		// pelmt が pEnd である場合は、何もしなくてよい

		if (m_pNewElmt_inList == NULL) { m_pNewElmt_inList = m_pEnd; }
	}

private:
	KSmplListElmt<T_Base>  ma_Elmts[size];
	KSmplListElmt<T_Base>*  m_pBegin = ma_Elmts;
	KSmplListElmt<T_Base>*  m_pEnd = &ma_Elmts[size - 1];
	// リストがフルになっている場合は NULL となる
	// リストが空の場合は m_pBegin が設定される
	KSmplListElmt<T_Base>*  m_pNewElmt_inList = m_pBegin;
};
