#ifndef __DLPS_LIST_HPP_INCLUDED
#define __DLPS_LIST_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear Double-Linked Pointed Simple List Class Definitions        */
/*                                                                      */
/* WARNING!                                                             */
/* This library contains a synchronization technique protected by       */
/* the U.S. Patent 9,983,913.                                           */
/*                                                                      */
/* THIS IS A PRE-RELEASE LIBRARY SNAPSHOT.                              */
/* AWAIT THE RELEASE AT https://mutexgear.com                           */
/*                                                                      */
/* Copyright (c) 2016-2021 Oleh Derevenko. All rights are reserved.     */
/*                                                                      */
/* E-mail: oleh.derevenko@gmail.com                                     */
/* Skype: oleh_derevenko                                                */
/************************************************************************/

/**
 *	\file
 *	\brief Double-Linked Pointed Simple List class definitions.
 *
 *	\note "Simple" means "without atomics" here.
 *
 *	The header defines classes for an embeddable list item and an embeddable double-linked
 *	list to connect these items with plain pointers. These classes are C++ counterparts
 *	for \c mutexgear_dlpsitem_* and \c mutexgear_dlpslist_* set of functions defined in
 *	mutexgear/dlpslist.h.
 *
 *	The classes are not related to synchronization on their own. However, variants of them are used in the library internally
 *	and this header-only C++ equivalent is a mean to make the library's public C++ interface complete with respect to 
 *	the C counterpart. Also, author's hope is this class could serve as a great educational idea for some library users
 *	and will help to improve efficiency and robustness of their future programs.
 */


#include <mutexgear/utility.h>
#include <iterator>


_MUTEXGEAR_BEGIN_NAMESPACE()


/**
 *	\class dlps_info
 *	\brief An embeddable list item class.
 *
 *	The class is to be used as a parent, possibly for multi-inheritance via \c parent_wrapper, to allow linking the host object
 *	into one or more \c dlps_list double-linked lists.
 *
 *	Even though the same result could be achieved via including \c dlps_info objects as member fields, using inheritance is preferable 
 *	as it allows transition from the info objects to the host with static casts rather than using field offsets and pointer arithmetic.
 *
 *	\see parent_wrapper
 *	\see dlps_list
 */
class dlps_info
{
public:
	dlps_info() noexcept { InitializeAsUnlinked(); }
	dlps_info(const dlps_info &siAnotherInfo) noexcept { MG_ASSERT(!siAnotherInfo.linked()); InitializeAsUnlinked(); }
	dlps_info(dlps_info &&siRefAnotherInfo) noexcept { MG_ASSERT(!siRefAnotherInfo.linked()); InitializeAsUnlinked(); }

	~dlps_info() noexcept
	{
		// Do nothing, do not assert the item to be not linked.
	}

	bool linked() const noexcept { return m_psiPreviousItem != this; }

	dlps_info &operator =(const dlps_info &siAnotherInfo) noexcept { MG_ASSERT(!linked()); MG_ASSERT(!siAnotherInfo.linked()); return *this; }
	dlps_info &operator =(dlps_info &&siRefAnotherInfo) noexcept { MG_ASSERT(!linked()); MG_ASSERT(!siRefAnotherInfo.linked()); return *this; }

protected:
	friend class dlps_list;

	enum selflinked_init_tag
	{
		init_selflinked,
	};

	explicit dlps_info(selflinked_init_tag) noexcept : m_psiNextItem(this), m_psiPreviousItem(this) {}

	void NaivelySwap(dlps_info &siAnotherInfo) noexcept { std::swap(m_psiNextItem, siAnotherInfo.m_psiNextItem); std::swap(m_psiPreviousItem, siAnotherInfo.m_psiPreviousItem); }

	void InitializeAsUnlinked() noexcept { m_psiPreviousItem = this; }
	void ResetToUnlinked() noexcept { m_psiPreviousItem = this; }
	void ResetToSelflinked() noexcept { m_psiPreviousItem = m_psiNextItem = this; }

private:
	void swap(dlps_info &siAnotherInstance) = delete; // The method is not expected to be called

private:
	dlps_info			*m_psiNextItem;
	dlps_info			*m_psiPreviousItem;
};


/**
 *	\class dlps_list
 *	\brief An embeddable double-linked list with interface similar to STL lists
 *
 *	The class implements a double-linked list with items embedded as parent classes of user objects
 *	rather than been allocated. The list is handy for temporarily or permanently gathering objects already 
 *	organized in some way (e.g., being kept in a STL container) into another ordered collection without 
 *	memory allocations. A single object can be linked into several lists like this. The object lifetime
 *	is not affected by linking or unlinking from the lists.
 *	
 *	Another perk is that the element unlink method is static and the list object pointer is not needed for the operation.
 *	I.e., an element can determine if it's linked and unlink itself without having the list pointer available.
 *	Also, having a list object permits iterating and unlinking objects with just their embedded \c dlps_info structures
 *	without examining the host objects themselves.
 *
 *	There is a restriction though, that an object being a member of the list is not copyable/movable in memory.
 *	However, when it's not linked in lists, the object can be copied or moved, provided that would be possible originally.
 *
 *	Here follow some usage demonstration code snippets.
 *	\code
 *	class CSomeClass:
		// In multi-inheritance case with several dlps_info structures used as parents,
		// each should be wrapped with a parent_wrapper with unique tuiInstanceTag parameter
		// to create distinct types that would be possible to be used as the multi-inheritance ancestors.
 *		private parent_wrapper<dlps_info, 0x21032119>
 *	{
 *		typedef parent_wrapper<dlps_info, 0x21032119> CSomeClass_SampleLinkParent;
 *
 *	public:
 *		void LinkIntoSampleList(dlps_list &dlListInstance)
 *		{
 *			dlListInstance.link_back(static_cast<CSomeClass_SampleLinkParent *>(this));
 *		}
 *
 *		bool IsLinkedInSampleList() const
 *		{
 *			return static_cast<const CSomeClass_SampleLinkParent *>(this)->linked();
 *		}
 *
 *		void UnlinkFromSampleList()
 *		{
 *			if (IsLinkedInSampleList())
 *			{
 *				dlps_list::unlink(dlps_list::make_iterator(static_cast<CSomeClass_SampleLinkParent *>(this)));
 *			}
 *		}
 *
 *		static CSomeClass *GetInstanceFromSampleLink(const dlps_info *pdiSampleLink)
 *		{
 *			// This approach with explicit casting via CSomeClass_SampleLinkParent allows having more than one parent like this with different typedef-ed names
 *			// and thus, allows being able to include the object into several lists at once.
 *			return static_cast<CSomeClass *>(const_cast<CSomeClass_SampleLinkParent *>(static_cast<const CSomeClass_SampleLinkParent *>(pdiSampleLink)));
 *		}
 *	};
 *
 *	class CMajorClass
 *	{
 *	public:
 *		void ProcessAndKeepSampleList()
 *		{
 *			const dlps_list::const_iterator itListEnd = m_dlSampleList.end();
 *			for (dlps_list::const_iterator itCurrentItem = m_dlSampleList.begin(); itCurrentItem != itListEnd; ++itCurrentItem)
 *			{
 *				CSomeClass *pscCurrentObject = CSomeClass::GetInstanceFromSampleLink(&*itCurrentItem);
 *
 *				// work with pscCurrentObject
 *			}
 *		};
 *
 *		void ProcessAndUnlinkSampleList()
 *		{
 *			while (!m_dlSampleList.empty())
 *			{
 *				CSomeClass *pscCurrentObject = CSomeClass::GetInstanceFromSampleLink(&m_dlSampleList.front());
 *				m_dlSampleList.unlink_front(); // Could have also been pscCurrentObject->UnlinkFromSampleList();
 *
 *				// work with pscCurrentObject
 *			}
 *		};
 *
 *	private:
 *		dlps_list		m_dlSampleList;
 *	};
 *	\endcode
 *
 *	\see dlps_info
 *	\see parent_wrapper
 */
class dlps_list
{
public:
	dlps_list() noexcept : m_siEndInfo(dlps_info::init_selflinked) {}
	dlps_list(const dlps_list &slAnotherInstance) noexcept : m_siEndInfo(dlps_info::init_selflinked) { MG_ASSERT(slAnotherInstance.empty()); }
	dlps_list(dlps_list &&slRefAnotherInstance) noexcept : m_siEndInfo(dlps_info::init_selflinked) { MG_ASSERT(slRefAnotherInstance.empty()); }

	~dlps_list() noexcept
	{
		// Do nothing, do not assert the list to be empty
	}

	void detach() noexcept { m_siEndInfo.ResetToSelflinked(); }
	void unlink_all() { while (!empty()) { unlink_front(); } }
	bool empty() const noexcept { return !m_siEndInfo.linked(); }

	dlps_list &operator =(const dlps_list &slAnotherInstance) noexcept { MG_ASSERT(empty()); MG_ASSERT(slAnotherInstance.empty()); return *this; }
	dlps_list &operator =(dlps_list &&slRefAnotherInstance) noexcept { MG_ASSERT(empty()); MG_ASSERT(slRefAnotherInstance.empty()); return *this; }

	void swap(dlps_list &slAnotherInstance) noexcept
	{
		if (this != &slAnotherInstance)
		{
			dlps_info *psiOwnNextItem = m_siEndInfo.m_psiNextItem;
			dlps_info *psiOwnPreviousItem = m_siEndInfo.m_psiPreviousItem;
			psiOwnNextItem->m_psiPreviousItem = &slAnotherInstance.m_siEndInfo;
			psiOwnPreviousItem->m_psiNextItem = &slAnotherInstance.m_siEndInfo;

			dlps_info *psiAnotherNextItem = slAnotherInstance.m_siEndInfo.m_psiNextItem;
			dlps_info *psiAnotherPreviousItem = slAnotherInstance.m_siEndInfo.m_psiPreviousItem;
			psiAnotherNextItem->m_psiPreviousItem = &m_siEndInfo;
			psiAnotherPreviousItem->m_psiNextItem = &m_siEndInfo;

			m_siEndInfo.NaivelySwap(slAnotherInstance.m_siEndInfo);
		}
	}

public:
	class reverse_iterator;
	class const_reverse_iterator;

	class iterator
	{
	public:
		iterator() noexcept {}
		iterator(const iterator &itAnotherIterator) noexcept : m_psiIteratorInfo(itAnotherIterator.m_psiIteratorInfo) {}
		inline iterator(const reverse_iterator &itAnotherIterator) noexcept;

	protected:
		friend class dlps_list;

		explicit iterator(dlps_info *psiIteratorInfo) noexcept : m_psiIteratorInfo(psiIteratorInfo) {}

	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef dlps_info value_type;
		typedef ptrdiff_t difference_type;
		typedef dlps_info &reference;
		typedef dlps_info *pointer;

		dlps_info &operator *() const noexcept { return *m_psiIteratorInfo; }
		dlps_info *operator ->() const noexcept { return m_psiIteratorInfo; }

		bool operator ==(const iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo == itAnotherIterator.m_psiIteratorInfo; }
		bool operator !=(const iterator &itAnotherIterator) const noexcept { return !operator ==(itAnotherIterator); }

		iterator &operator ++() noexcept { m_psiIteratorInfo = m_psiIteratorInfo->m_psiNextItem; return *this; }
		iterator operator ++(int) noexcept { dlps_info *psiIteratorInfoCopy = m_psiIteratorInfo; m_psiIteratorInfo = psiIteratorInfoCopy->m_psiNextItem; return iterator(psiIteratorInfoCopy); }
		iterator &operator --() noexcept { m_psiIteratorInfo = m_psiIteratorInfo->m_psiPreviousItem; return *this; }
		iterator operator --(int) noexcept { dlps_info *psiIteratorInfoCopy = m_psiIteratorInfo; m_psiIteratorInfo = psiIteratorInfoCopy->m_psiPreviousItem; return iterator(psiIteratorInfoCopy); }

		iterator &operator =(const iterator &itAnotherIterator) noexcept { m_psiIteratorInfo = itAnotherIterator.m_psiIteratorInfo; return *this; }
		inline iterator &operator =(const reverse_iterator &itAnotherIterator) noexcept;

	private:
		friend class const_iterator;

	private:
		dlps_info		*m_psiIteratorInfo;
	};

	class const_iterator
	{
	public:
		const_iterator() noexcept {}
		const_iterator(const const_iterator &itAnotherIterator) noexcept : m_psiIteratorInfo(itAnotherIterator.m_psiIteratorInfo) {}
		const_iterator(const iterator &itAnotherIterator) noexcept : m_psiIteratorInfo(itAnotherIterator.m_psiIteratorInfo) {}
		inline const_iterator(const const_reverse_iterator &itAnotherIterator) noexcept;

	protected:
		friend class dlps_list;

		explicit const_iterator(const dlps_info *psiIteratorInfo) noexcept : m_psiIteratorInfo(psiIteratorInfo) {}

	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef dlps_info value_type;
		typedef ptrdiff_t difference_type;
		typedef const dlps_info &reference;
		typedef const dlps_info *pointer;

		const dlps_info &operator *() const noexcept { return *m_psiIteratorInfo; }
		const dlps_info *operator ->() const noexcept { return m_psiIteratorInfo; }

		bool operator ==(const const_iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo == itAnotherIterator.m_psiIteratorInfo; }
		bool operator !=(const const_iterator &itAnotherIterator) const noexcept { return !operator ==(itAnotherIterator); }

		bool operator <(const const_iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo < itAnotherIterator.m_psiIteratorInfo; }
		bool operator >(const const_iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo > itAnotherIterator.m_psiIteratorInfo; }
		bool operator <=(const const_iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo <= itAnotherIterator.m_psiIteratorInfo; }
		bool operator >=(const const_iterator &itAnotherIterator) const noexcept { return m_psiIteratorInfo >= itAnotherIterator.m_psiIteratorInfo; }

		const_iterator &operator ++() noexcept { m_psiIteratorInfo = m_psiIteratorInfo->m_psiNextItem; return *this; }
		const_iterator operator ++(int) noexcept { const dlps_info *psiIteratorInfoCopy = m_psiIteratorInfo; m_psiIteratorInfo = psiIteratorInfoCopy->m_psiNextItem; return const_iterator(psiIteratorInfoCopy); }
		const_iterator &operator --() noexcept { m_psiIteratorInfo = m_psiIteratorInfo->m_psiPreviousItem; return *this; }
		const_iterator operator --(int) noexcept { const dlps_info *psiIteratorInfoCopy = m_psiIteratorInfo; m_psiIteratorInfo = psiIteratorInfoCopy->m_psiPreviousItem; return const_iterator(psiIteratorInfoCopy); }

		const_iterator &operator =(const const_iterator &itAnotherIterator) noexcept { m_psiIteratorInfo = itAnotherIterator.m_psiIteratorInfo; return *this; }
		const_iterator &operator =(const iterator &itAnotherIterator) noexcept { m_psiIteratorInfo = itAnotherIterator.m_psiIteratorInfo; return *this; }
		inline const_iterator &operator =(const const_reverse_iterator &itAnotherIterator) noexcept;

	private:
		const dlps_info	*m_psiIteratorInfo;
	};

	class reverse_iterator :
		public std::reverse_iterator<dlps_list::iterator>
	{
	private:
		typedef std::reverse_iterator<dlps_list::iterator> parent_type;

	public:
		reverse_iterator() noexcept : parent_type() {}
		reverse_iterator(const reverse_iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}
		reverse_iterator(const dlps_list::iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}

	protected:
		friend class dlps_list;

		explicit reverse_iterator(dlps_info *psiIteratorInfo) noexcept : parent_type(dlps_list::iterator(psiIteratorInfo)) {}
	};

	class const_reverse_iterator :
		public std::reverse_iterator<dlps_list::const_iterator>
	{
	private:
		typedef std::reverse_iterator<dlps_list::const_iterator> parent_type;

	public:
		const_reverse_iterator() noexcept {}
		const_reverse_iterator(const const_reverse_iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}
		const_reverse_iterator(const reverse_iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}
		const_reverse_iterator(const dlps_list::const_iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}
		const_reverse_iterator(const dlps_list::iterator &itAnotherIterator) noexcept : parent_type(itAnotherIterator) {}

	protected:
		friend class dlps_list;

		explicit const_reverse_iterator(const dlps_info *psiIteratorInfo) noexcept : parent_type(dlps_list::const_iterator(psiIteratorInfo)) {}
	};

	static iterator make_iterator(dlps_info *psiIteratorInfo) noexcept { return iterator(psiIteratorInfo); }
	static const_iterator make_iterator(const dlps_info *psiIteratorInfo) noexcept { return const_iterator(psiIteratorInfo); }
	static reverse_iterator make_reverse_iterator(dlps_info *psiIteratorInfo) noexcept { return reverse_iterator(psiIteratorInfo); }
	static const_reverse_iterator make_reverse_iterator(const dlps_info *psiIteratorInfo) noexcept { return const_reverse_iterator(psiIteratorInfo); }

public:
	iterator begin() noexcept { return iterator(m_siEndInfo.m_psiNextItem); }
	iterator end() noexcept { return iterator(&m_siEndInfo); }

	const_iterator begin() const noexcept { return const_iterator(m_siEndInfo.m_psiNextItem); }
	const_iterator end() const noexcept { return const_iterator(&m_siEndInfo); }

	reverse_iterator rbegin() noexcept { return reverse_iterator(&m_siEndInfo); }
	reverse_iterator rend() noexcept { return reverse_iterator(m_siEndInfo.m_psiNextItem); }

	const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(&m_siEndInfo); }
	const_reverse_iterator rend() const noexcept { return const_reverse_iterator(m_siEndInfo.m_psiNextItem); }

	dlps_info &front() noexcept { MG_ASSERT(!empty()); return *begin(); }
	dlps_info &back() noexcept { MG_ASSERT(!empty()); return *rbegin(); }

	const dlps_info &front() const noexcept { MG_ASSERT(!empty()); return *begin(); }
	const dlps_info &back() const noexcept { MG_ASSERT(!empty()); return *rbegin(); }

public:
	void link_front(dlps_info *psiItemToBeLinked) noexcept { link(psiItemToBeLinked, begin()); }
	void link_back(dlps_info *psiItemToBeLinked) noexcept { link(psiItemToBeLinked, end()); }

	static 
	void link(dlps_info *psiItemToBeLinked, iterator itBeforeItem) noexcept
	{
		MG_ASSERT(!psiItemToBeLinked->linked());

		dlps_info *psiBeforeItem = &*itBeforeItem;

		psiItemToBeLinked->m_psiNextItem = psiBeforeItem;

		dlps_info *psiAfterItem = psiBeforeItem->m_psiPreviousItem;
		psiItemToBeLinked->m_psiPreviousItem = psiAfterItem;

		psiBeforeItem->m_psiPreviousItem = psiItemToBeLinked;
		psiAfterItem->m_psiNextItem = psiItemToBeLinked;
	}

	void unlink_front() noexcept { unlink(begin()); }
	void unlink_back() noexcept { iterator itEnd = end(); unlink(--itEnd); }

	static
	void unlink(iterator itItemToBeUnlinked) noexcept
	{
		dlps_info *psiItemInstance = &*itItemToBeUnlinked;
		MG_ASSERT(psiItemInstance->linked());

		dlps_info *psiBeforeItem = psiItemInstance->m_psiNextItem;
		dlps_info *psiAfterItem = psiItemInstance->m_psiPreviousItem;

		psiBeforeItem->m_psiPreviousItem = psiAfterItem;
		psiAfterItem->m_psiNextItem = psiBeforeItem;

		psiItemInstance->ResetToUnlinked();
	}

public:
	void splice_front(dlps_list &slAnotherInstance) noexcept { splice(begin(), slAnotherInstance); }
	void splice_front(iterator itAnotherIterator) noexcept { splice(begin(), itAnotherIterator); }
	void splice_front(iterator itAnotherRangeBegin, iterator itAnotherRangeEnd) noexcept { splice(begin(), itAnotherRangeBegin, itAnotherRangeEnd); }

	void splice_back(dlps_list &slAnotherInstance) noexcept { splice(end(), slAnotherInstance); }
	void splice_back(iterator itAnotherIterator) noexcept { splice(end(), itAnotherIterator); }
	void splice_back(iterator itAnotherRangeBegin, iterator itAnotherRangeEnd) noexcept { splice(end(), itAnotherRangeBegin, itAnotherRangeEnd); }

	void splice(iterator itBeforeItem, dlps_list &slAnotherInstance) noexcept
	{
		splice(itBeforeItem, slAnotherInstance.begin(), slAnotherInstance.end());
	}

	void splice(iterator itBeforeItem, iterator itAnotherIterator) noexcept
	{
		iterator itAnotherIteratorNext = itAnotherIterator; ++itAnotherIteratorNext;
		SpliceRange(itBeforeItem, itAnotherIterator, itAnotherIteratorNext);
	}

	void splice(iterator itBeforeItem, iterator itRangeBegin, iterator itRangeEnd) noexcept
	{
		if (itRangeBegin != itRangeEnd)
		{
			SpliceRange(itBeforeItem, itRangeBegin, itRangeEnd);
		}
	}

private:
	void SpliceRange(iterator itBeforeItem, iterator itRangeBegin, iterator itRangeEnd) noexcept
	{
		dlps_info *psiRangeBegin = &*itRangeBegin;
		dlps_info *psiRangeEnd = &*itRangeEnd;
		dlps_info *psiBeforeItem = &*itBeforeItem;

		dlps_info *psiRangeLast = psiRangeEnd->m_psiPreviousItem;
		psiRangeLast->m_psiNextItem = psiBeforeItem;

		// Save the m_psiPreviousItem of the first item to be spliced as the pointer is going to be replaced.
		dlps_info *psiOriginalRangePrevious = psiRangeBegin->m_psiPreviousItem;

		dlps_info *psiAfterItem = psiBeforeItem->m_psiPreviousItem;
		psiRangeBegin->m_psiPreviousItem = psiAfterItem;

		psiBeforeItem->m_psiPreviousItem = psiRangeLast;
		psiAfterItem->m_psiNextItem = psiRangeBegin;

		psiOriginalRangePrevious->m_psiNextItem = psiRangeEnd;
		psiRangeEnd->m_psiPreviousItem = psiOriginalRangePrevious;
	}

private:
	dlps_info			m_siEndInfo;
};


dlps_list::iterator::iterator(const dlps_list::reverse_iterator &itAnotherIterator) noexcept :
	m_psiIteratorInfo(&*itAnotherIterator)
{
}

dlps_list::iterator &dlps_list::iterator::operator =(const dlps_list::reverse_iterator &itAnotherIterator) noexcept
{
	m_psiIteratorInfo = &*itAnotherIterator;
	return *this;
}

dlps_list::const_iterator::const_iterator(const dlps_list::const_reverse_iterator &itAnotherIterator) noexcept :
	m_psiIteratorInfo(&*itAnotherIterator)
{
}

dlps_list::const_iterator &dlps_list::const_iterator::operator =(const dlps_list::const_reverse_iterator &itAnotherIterator) noexcept
{
	m_psiIteratorInfo = &*itAnotherIterator;
	return *this;
}


_MUTEXGEAR_END_NAMESPACE();


namespace std
{

template<>
inline
void swap(_MUTEXGEAR_NAMESPACE::dlps_list &slRefOneList, _MUTEXGEAR_NAMESPACE::dlps_list &slRefAnotherList) noexcept
{
	slRefOneList.swap(slRefAnotherList);
}

}; // namespace std


#endif // #ifndef __DLPS_LIST_HPP_INCLUDED
