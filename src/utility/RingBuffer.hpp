#include <queue>
#include <deque>

namespace utility
{
	/*template <typename T>
	struct bufferino {
		std::array<T, 32> m_array;
		int m_maxLen = 32;
		int m_index = 0;

		typedef typename Container::iterator iterator;
		typedef typename Container::const_iterator const_iterator;

		iterator begin() { return m_array.begin(); }
		iterator end() { return m_array.end(); }
		const_iterator begin() const { return m_array.begin(); }
		const_iterator end() const { return m_array.end(); }

		void push(T* value) {
			m_array[m_index++ % m_maxLen] = value;
		}

		void pop(int index) {
			T* ptr = m_array[index];
			delete ptr;
			m_array[index] = nullptr;
		}
	};*/
	template <typename T, int MaxLen, typename Container=std::deque<T>>
	class FixedQueue : public std::queue<T, Container> {
	public:
		typedef typename Container::iterator iterator;
		typedef typename Container::const_iterator const_iterator;

		iterator begin() { return this->c.begin(); }
		iterator end() { return this->c.end(); }
		const_iterator begin() const { return this->c.begin(); }
		const_iterator end() const { return this->c.end(); }

		void push(const T& value) {
			if (this->size() == MaxLen) {
				this->c.pop_back();
			}
			std::queue<T, Container>::push(value);
		}
		void pop_back() {
			this->c.pop_back();
		}
	};
}