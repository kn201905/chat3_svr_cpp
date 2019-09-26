#pragma once

template<typename T>
struct  KSmplListElmt : public T
{
	KSmplListElmt<T>*  m_pPrev = NULL;
	KSmplListElmt<T>*  m_pNext = NULL;
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

	KSmplListElmt<T_Base>*  Get_LastEmnt()
	{
		if (m_pNewElmt_inList == NULL)
		{ return  m_pEnd; }

		return  m_pNewElmt_inList->m_pPrev;
	}

	// バッファがフルで新しい要素を返せない場合、NULLが返される
	KSmplListElmt<T_Base>*  Get_NewElmt()
	{
		if (m_pNewElmt_inList == NULL) { return NULL; }

		KSmplListElmt<T_Base>*  ret_val = m_pNewElmt_inList;
		m_pNewElmt_inList = m_pNewElmt_inList->m_pNext;

		m_len++;
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

		m_len--;
	}

	uint16_t  Get_Len() { return  m_len; }

private:
	KSmplListElmt<T_Base>  ma_Elmts[size];
	KSmplListElmt<T_Base>*  m_pBegin = ma_Elmts;
	KSmplListElmt<T_Base>*  m_pEnd = &ma_Elmts[size - 1];
	// リストがフルになっている場合は NULL となる
	// リストが空の場合は m_pBegin が設定される
	KSmplListElmt<T_Base>*  m_pNewElmt_inList = m_pBegin;

	// Get_NewElmt() で＋１、MoveToEnd() で－１としている
	uint16_t  m_len = 0;
};
