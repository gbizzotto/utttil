
#ifndef ON_SCOPE_EXIT

#include <functional>
#include <utility>

#define CONCAT(x, y) x ## y
#define CONCAT2(x, y) CONCAT(x, y)

#define ON_SCOPE_EXIT_NAMED(name,x) \
	auto CONCAT2(lambda_, name) = x; \
	utttil::__OnScopeExit<decltype(CONCAT2(lambda_, name))> name(std::move(CONCAT2(lambda_, name)));

#define ON_SCOPE_EXIT(x) ON_SCOPE_EXIT_NAMED(CONCAT2(on_scope_exit_, __LINE__), x)

namespace utttil {

	template<typename Functor>
	class __OnScopeExit
	{
	public:
		__OnScopeExit(Functor & f)       = delete;
		__OnScopeExit(const Functor & f) = delete;
		inline __OnScopeExit(Functor && f)
			:functor(std::forward<Functor>(f))
		{
			do_it = true;
		}
		inline ~__OnScopeExit()
		{
			if (do_it)
				functor();
		}
		void Cancel()
		{
			do_it = false;
		}

	private:
		bool do_it;
		Functor functor;
	};

} // namespace

#endif // ON_SCOPE_EXIT
