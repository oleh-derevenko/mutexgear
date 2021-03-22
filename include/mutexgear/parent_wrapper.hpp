#ifndef __PARENT_WRAPPER_HPP_INCLUDED
#define __PARENT_WRAPPER_HPP_INCLUDED


/************************************************************************/
/* The MutexGear Library                                                */
/* MutexGear parent_wrapper Class Definition                            */
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
*	\brief MutexGear \c parent_wrapper class definition
*
*	The header defines a wrapper class to be used for disambiguation between
*	matching parent types in multiple inheritance by adding an integer tag 
*	that can make the type declarations distinct. In particular, this allows
*	deriving from multiple parent list info structures (such as \c dlps_info)
*	and linking the class object into multiple embedded lists simultaneously.
*/


#include <mutexgear/config.h>
#include <type_traits>
#include <utility>
#include <cstdint>


_MUTEXGEAR_BEGIN_NAMESPACE()


/**
 *	\class parent_wrapper
 *	\brief A wrapper class for parent disambiguation.
 *
 *	The class uses \p tuiInstanceTag template parameter to make distinct types from the same class 
 *	and allow using the class for multiple inheritance.
 *
 *	Within the MutexGear library, for example, this can allow linking a single object into several embeddable lists at once.
 *
 *	\see dlps_info
 *	\see dlps_list
 */
template<class TParentType, std::uint32_t tuiInstanceTag=0>
class parent_wrapper:
	public TParentType
{
public:
	typedef TParentType parent_type;
	typedef typename std::add_const<TParentType>::type const_parent_type;

	template<typename=typename std::enable_if<std::is_default_constructible<TParentType>::value>::type>
	parent_wrapper() noexcept(std::is_nothrow_default_constructible<TParentType>::value): parent_type() {}

	template<class... TArgs>
	parent_wrapper(TArgs... taArgs) noexcept(std::is_nothrow_constructible<TParentType, TArgs...>::value) : parent_type(std::forward<TArgs>(taArgs)...) {}

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_constructible<TParentType, TArgumentParentType>::value>::type>
	parent_wrapper(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherInstance) noexcept(std::is_nothrow_constructible<TParentType, TArgumentParentType>::value): parent_type(static_cast<const TArgumentParentType &>(pwAnotherInstance)) {}

	template<std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_move_constructible<TParentType>::value>::type>
	parent_wrapper(parent_wrapper<TParentType, tuiArgumentInstanceTag> &&pwRefAnotherInstance) noexcept(std::is_nothrow_move_constructible<TParentType>::value): parent_type(std::move(static_cast<TParentType &&>(pwRefAnotherInstance))) {}


	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_assignable<TParentType, TArgumentParentType>::value>::type>
	parent_wrapper &operator =(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) noexcept(std::is_nothrow_assignable<TParentType, TArgumentParentType>::value) { parent_type::operator =(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); return *this; }

	template<std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_move_assignable<TParentType>::value>::type>
	parent_wrapper &operator =(parent_wrapper<TParentType, tuiArgumentInstanceTag> &&pwRefAnotherWrapper) noexcept(std::is_nothrow_move_assignable<TParentType>::value) { parent_type::operator =(std::move(static_cast<TParentType &&>(pwRefAnotherWrapper))); return *this; }


	template<std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_move_constructible<TParentType>::value && std::is_move_assignable<TParentType>::value>::type>
	void swap(parent_wrapper<TParentType, tuiArgumentInstanceTag> &pwRefAnotherWrapper) noexcept(std::is_nothrow_move_constructible<TParentType>::value && std::is_nothrow_move_assignable<TParentType>::value) { std::swap(static_cast<parent_type &>(*this), static_cast<TParentType &>(pwRefAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator ==(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator ==(std::declval<TArgumentParentType>()))) { return parent_type::operator ==(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator !=(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator !=(std::declval<TArgumentParentType>()))) { return parent_type::operator !=(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator <(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator <(std::declval<TArgumentParentType>()))) { return parent_type::operator <(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator >(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator >(std::declval<TArgumentParentType>()))) { return parent_type::operator >(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator <=(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator <=(std::declval<TArgumentParentType>()))) { return parent_type::operator <=(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	template<class TArgumentParentType, std::uint32_t tuiArgumentInstanceTag>
	bool operator >=(const parent_wrapper<TArgumentParentType, tuiArgumentInstanceTag> &pwAnotherWrapper) const noexcept(noexcept(std::declval<const_parent_type>().operator >=(std::declval<TArgumentParentType>()))) { return parent_type::operator >=(static_cast<const TArgumentParentType &>(pwAnotherWrapper)); }

	// operator bool() const { ; } -- operator bool is too dangerous as it tends to be used instead of casting to int
	bool operator !() const noexcept(noexcept(std::declval<const_parent_type>().operator !())) { return parent_type::operator !(); }

	parent_wrapper &operator ++() noexcept(noexcept(std::declval<parent_type>().operator ++())) { parent_type::operator ++(); return *this; }
	parent_wrapper &operator --() noexcept(noexcept(std::declval<parent_type>().operator --())) { parent_type::operator --(); return *this; }
};


_MUTEXGEAR_END_NAMESPACE();


namespace std
{

template<class TParentType, std::uint32_t tuiInstanceTag, std::uint32_t tuiArgumentInstanceTag, typename=typename std::enable_if<std::is_move_constructible<TParentType>::value && std::is_move_assignable<TParentType>::value && tuiInstanceTag != tuiArgumentInstanceTag>::type>
inline
void swap(_MUTEXGEAR_NAMESPACE::parent_wrapper<TParentType, tuiInstanceTag> &pwRefOneWrapper, _MUTEXGEAR_NAMESPACE::parent_wrapper<TParentType, tuiArgumentInstanceTag> &pwRefAnotherWrapper) noexcept(std::is_nothrow_move_constructible<TParentType>::value && std::is_nothrow_move_assignable<TParentType>::value)
{
	pwRefOneWrapper.swap(pwRefAnotherWrapper);
}

};

#endif // #ifndef __PARENT_WRAPPER_HPP_INCLUDED
