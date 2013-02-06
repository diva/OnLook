/**
 * @file ailist.h
 * @brief A linked list with iterators that advance when their element is deleted.
 *
 * Copyright (c) 2013, Aleric Inglewood.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution.
 *
 * CHANGELOG
 *   and additional copyright holders.
 *
 *   05/02/2013
 *   Initial version, written by Aleric Inglewood @ SL
 */

#ifndef AILIST_H
#define AILIST_H

#include <list>

template<typename T>
class AIConstListIterator;

template<typename T>
class AIListIterator {
  private:
	typedef AIListIterator<T>				_Self;
	typedef std::list<T>					_Container;
	typedef typename _Container::iterator	_Iterator;

	_Container* mContainer;
	_Iterator mIterator;

  public:
	typedef typename _Iterator::difference_type		difference_type;
	typedef typename _Iterator::iterator_category	iterator_category;
	typedef typename _Iterator::value_type			value_type;
	typedef typename _Iterator::pointer				pointer;
	typedef typename _Iterator::reference			reference;

	AIListIterator(void) : mContainer(NULL) { }
	AIListIterator(_Container* __c, _Iterator const& __i) : mContainer(__c), mIterator(__i) { }

	_Self& operator=(_Self const& __x) { mContainer = __x.mContainer; mIterator = __x.mIterator; return *this; }

	reference operator*() const { return *mIterator; }
	pointer operator->() const { return mIterator.operator->(); }
	_Self& operator++() { ++mIterator; return *this; }
	_Self operator++(int) { _Self tmp = *this; ++mIterator; return tmp; }
	_Self& operator--() { --mIterator; return *this; }
	_Self operator--(int) { _Self tmp = *this; --mIterator; return tmp; }

	bool operator==(_Self const& __x) const { return mIterator == __x.mIterator; } 
	bool operator!=(_Self const& __x) const { return mIterator != __x.mIterator; } 

	template<typename T2> friend class AIConstListIterator;
	template<typename T2> friend bool operator==(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	template<typename T2> friend bool operator!=(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
};

template<typename T>
class AIConstListIterator {
  private:
	typedef AIConstListIterator<T>					_Self;
	typedef std::list<T>							_Container;
	typedef typename _Container::const_iterator		_ConstIterator;
	typedef AIListIterator<T>						iterator;

	_Container const* mContainer;
	_ConstIterator mConstIterator;

  public:
	typedef typename _ConstIterator::difference_type	difference_type;
	typedef typename _ConstIterator::iterator_category	iterator_category;
	typedef typename _ConstIterator::value_type			value_type;
	typedef typename _ConstIterator::pointer			pointer;
	typedef typename _ConstIterator::reference			reference;

	AIConstListIterator(void) : mContainer(NULL) { }
	AIConstListIterator(_Container const* __c, _ConstIterator const& __i) : mContainer(__c), mConstIterator(__i) { }
	AIConstListIterator(iterator const& __x) : mContainer(__x.mContainer), mConstIterator(__x.mIterator) { }

	_Self& operator=(_Self const& __x) { mContainer = __x.mContainer; mConstIterator = __x.mConstIterator; return *this; }
	_Self& operator=(iterator const& __x) { mContainer = __x.mContainer; mConstIterator = __x.mIterator; return *this; }

	reference operator*() const { return *mConstIterator; }
	pointer operator->() const { return mConstIterator.operator->(); }
	_Self& operator++() { ++mConstIterator; return *this; }
	_Self operator++(int) { _Self tmp = *this; ++mConstIterator; return tmp; }
	_Self& operator--() { --mConstIterator; return *this; }
	_Self operator--(int) { _Self tmp = *this; --mConstIterator; return tmp; }

	bool operator==(_Self const& __x) const { return mConstIterator == __x.mConstIterator; } 
	bool operator!=(_Self const& __x) const { return mConstIterator != __x.mConstIterator; } 

	template<typename T2> friend bool operator==(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
	template<typename T2> friend bool operator!=(AIListIterator<T2> const& __x, AIConstListIterator<T2> const& __y);
};

template<typename T>
inline bool operator==(AIListIterator<T> const& __x, AIConstListIterator<T> const& __y)
{
  return __x.mIterator == __y.mConstIterator;
}

template<typename T>
inline bool operator!=(AIListIterator<T> const& __x, AIConstListIterator<T> const& __y)
{
  return __x.mIterator != __y.mConstIterator;
}

template<typename T>
class AIList {
  private:
	std::list<T> mContainer;

  public:
	typedef T										value_type;
	typedef T*										pointer;
	typedef T const*								const_pointer;
	typedef T&										reference;
	typedef T const&								const_reference;
	typedef AIListIterator<T>						iterator;
	typedef AIConstListIterator<T>					const_iterator;
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	typedef size_t									size_type;
	typedef ptrdiff_t								difference_type;

	AIList(void) { }
	//AIList(std::list<T> const& __list) : mContainer(__list) { }

	explicit AIList(size_type __n, value_type const& __value = value_type()) : mContainer(__n, __value) { }
	AIList(AIList const& __list) : mContainer(__list.mContainer) { }

	template<typename _InputIterator>
	  AIList(_InputIterator __first, _InputIterator __last) : mContainer(__first, __last) { }

	AIList& operator=(AIList const& __list) { mContainer = __list.mContainer; return *this; }

	iterator begin() { return iterator(&mContainer, mContainer.begin()); }
	const_iterator begin() const { return const_iterator(&mContainer, mContainer.begin()); }
	iterator end() { return iterator(&mContainer, mContainer.end()); }
	const_iterator end() const { return const_iterator(&mContainer, mContainer.end()); }
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool empty() const { return mContainer.empty(); } 
	size_type size() const { return mContainer.size(); } 
	size_type max_size() const { return mContainer.max_size(); } 

	reference front() { return mContainer.front(); }
	const_reference front() const { return mContainer.front(); }
	reference back() { return mContainer.back(); }
	const_reference back() const { return mContainer.back(); }

	void push_front(value_type const& __x) { mContainer.push_front(__x); }
	void pop_front() { mContainer.pop_front(); }
	void push_back(value_type const& __x) { mContainer.push_back(__x); }
	void pop_back() { mContainer.pop_back(); }
	iterator insert(iterator __position, value_type const& __x) { return iterator(&mContainer, mContainer.insert(__position, __x)); }
	iterator erase(iterator __position) { return iterator(&mContainer, mContainer.erase(__position)); }
	void clear() { mContainer.clear(); }
	void remove(value_type const& __value) { mContainer.remove(__value); }

	// Use this with care. No iterators should be left pointing at elements after the code returns.
	std::list<T> const& get_std_list(void) const { return mContainer; }

	void sort() { mContainer.sort(); }
	template<typename _StrictWeakOrdering>
	  void sort(_StrictWeakOrdering pred) { mContainer.sort(pred); }
};

#endif
