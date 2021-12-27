
#include <utttil/small_shared_ptr.hpp>
#include <utttil/assert.hpp>

size_t count = 0;

struct Test
{
	int x;
	Test(int a)
		: x(a)
	{
		++count;
	}
	~Test()
	{
		--count;
	}
};

struct TestSFT : utttil::enable_small_shared_from_this<TestSFT>
{
	int x;
	TestSFT(int a)
		: x(a)
	{
		++count;
	}
	~TestSFT()
	{
		--count;
	}
	utttil::small_shared_ptr<TestSFT> shared()
	{
		return this->small_shared_from_this();
	}
};

bool test_shared_ptr()
{
	utttil::small_shared_ptr<int> a(new int(123));
	ASSERT_ACT(!!a, ==, true, return false);
	ASSERT_ACT(*a, ==, 123, return false);
	ASSERT_ACT(a.get(), !=, nullptr, return false);
	ASSERT_ACT(a.use_count(), ==, 1, return false);

	auto b = a;
	ASSERT_ACT(!!a, ==, true, return false);
	ASSERT_ACT(!!b, ==, true, return false);
	ASSERT_ACT(*a, ==, 123, return false);
	ASSERT_ACT(*b, ==, 123, return false);
	ASSERT_ACT(a.get(), !=, nullptr, return false);
	ASSERT_ACT(a.get(), ==, b.get(), return false);
	ASSERT_ACT(a.use_count(), ==, 2, return false);
	ASSERT_ACT(b.use_count(), ==, 2, return false);

	*a = 666;
	ASSERT_ACT(!!a, ==, true, return false);
	ASSERT_ACT(!!b, ==, true, return false);
	ASSERT_ACT(*a, ==, 666, return false);
	ASSERT_ACT(*b, ==, 666, return false);
	ASSERT_ACT(a.get(), !=, nullptr, return false);
	ASSERT_ACT(a.get(), ==, b.get(), return false);
	ASSERT_ACT(a.use_count(), ==, 2, return false);
	ASSERT_ACT(b.use_count(), ==, 2, return false);

	a.reset();
	ASSERT_ACT(!!a, ==, false, return false);
	ASSERT_ACT(!!b, ==, true, return false);
	ASSERT_ACT(*b, ==, 666, return false);
	ASSERT_ACT(a.get(), ==, nullptr, return false);
	ASSERT_ACT(b.get(), !=, nullptr, return false);
	ASSERT_ACT(a.use_count(), ==, 0, return false);
	ASSERT_ACT(b.use_count(), ==, 1, return false);

	b.reset();
	ASSERT_ACT(!!a, ==, false, return false);
	ASSERT_ACT(!!b, ==, false, return false);
	ASSERT_ACT(a.get(), ==, nullptr, return false);
	ASSERT_ACT(b.get(), ==, nullptr, return false);
	ASSERT_ACT(a.use_count(), ==, 0, return false);
	ASSERT_ACT(b.use_count(), ==, 0, return false);

	a.reset(new int(444));
	ASSERT_ACT(!!a, ==, true, return false);
	ASSERT_ACT(!!b, ==, false, return false);
	ASSERT_ACT(*a, ==, 444, return false);
	ASSERT_ACT(a.get(), !=, nullptr, return false);
	ASSERT_ACT(a.use_count(), ==, 1, return false);
	ASSERT_ACT(b.get(), ==, nullptr, return false);
	ASSERT_ACT(b.use_count(), ==, 0, return false);

	return true;
}

bool test_shared_ptr_object()
{
	{
		ASSERT_ACT(count, ==, 0, return false);

		utttil::small_shared_ptr<Test> a(new Test(123));
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT((*a).x, ==, 123, return false);
		ASSERT_ACT(a->x, ==, 123, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		auto b = a;
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(!!b, ==, true, return false);
		ASSERT_ACT((*a).x, ==, 123, return false);
		ASSERT_ACT(a->x, ==, 123, return false);
		ASSERT_ACT((*b).x, ==, 123, return false);
		ASSERT_ACT(b->x, ==, 123, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.get(), ==, b.get(), return false);
		ASSERT_ACT(a.use_count(), ==, 2, return false);
		ASSERT_ACT(b.use_count(), ==, 2, return false);
		ASSERT_ACT(count, ==, 1, return false);

		*a = 666;
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(!!b, ==, true, return false);
		ASSERT_ACT((*a).x, ==, 666, return false);
		ASSERT_ACT(a->x, ==, 666, return false);
		ASSERT_ACT((*b).x, ==, 666, return false);
		ASSERT_ACT(b->x, ==, 666, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.get(), ==, b.get(), return false);
		ASSERT_ACT(a.use_count(), ==, 2, return false);
		ASSERT_ACT(b.use_count(), ==, 2, return false);
		ASSERT_ACT(count, ==, 1, return false);

		a.reset();
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(!!b, ==, true, return false);
		ASSERT_ACT((*b).x, ==, 666, return false);
		ASSERT_ACT(b->x, ==, 666, return false);
		ASSERT_ACT(a.get(), ==, nullptr, return false);
		ASSERT_ACT(b.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 0, return false);
		ASSERT_ACT(b.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		b.reset();
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(!!b, ==, false, return false);
		ASSERT_ACT(a.get(), ==, nullptr, return false);
		ASSERT_ACT(a.get(), ==, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 0, return false);
		ASSERT_ACT(b.use_count(), ==, 0, return false);
		ASSERT_ACT(count, ==, 0, return false);

		a.reset(new Test(444));
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(!!b, ==, false, return false);
		ASSERT_ACT((*a).x, ==, 444, return false);
		ASSERT_ACT(a->x, ==, 444, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(b.get(), ==, nullptr, return false);
		ASSERT_ACT(b.use_count(), ==, 0, return false);
		ASSERT_ACT(count, ==, 1, return false);
	}

	ASSERT_ACT(count, ==, 0, return false);

	return true;
}

bool test_make_shared()
{
	{
		ASSERT_ACT(count, ==, 0, return false);

		auto a = utttil::make_small_shared<Test>(123);
		ASSERT_ACT((*a).x, ==, 123, return false);
		ASSERT_ACT(a->x, ==, 123, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		a = utttil::make_small_shared<Test>(1312);
		ASSERT_ACT((*a).x, ==, 1312, return false);
		ASSERT_ACT(a->x, ==, 1312, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);
	}

	ASSERT_ACT(count, ==, 0, return false);

	return true;
}

bool test_move()
{
	ASSERT_ACT(count, ==, 0, return false);
	{
		auto a = utttil::make_small_shared<Test>(1337);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT((*a).x, ==, 1337, return false);
		ASSERT_ACT(a->x, ==, 1337, return false);
		ASSERT_ACT(a.get(), !=, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_shared_ptr<Test> b;

		b = std::move(a);
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(!!b, ==, true, return false);
		ASSERT_ACT(a.get(), ==, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 0, return false);
		ASSERT_ACT(a.counter, ==, nullptr, return false);

		ASSERT_ACT((*b).x, ==, 1337, return false);
		ASSERT_ACT(b->x, ==, 1337, return false);
		ASSERT_ACT(b.get(), !=, nullptr, return false);
		ASSERT_ACT(b.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_shared_ptr<Test> c(std::move(b));
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(!!b, ==, false, return false);
		ASSERT_ACT(!!c, ==, true, return false);
		ASSERT_ACT(a.get(), ==, nullptr, return false);
		ASSERT_ACT(a.use_count(), ==, 0, return false);
		ASSERT_ACT(a.counter, ==, nullptr, return false);
		ASSERT_ACT(b.get(), ==, nullptr, return false);
		ASSERT_ACT(b.use_count(), ==, 0, return false);
		ASSERT_ACT(b.counter, ==, nullptr, return false);

		ASSERT_ACT((*c).x, ==, 1337, return false);
		ASSERT_ACT(c->x, ==, 1337, return false);
		ASSERT_ACT(c.get(), !=, nullptr, return false);
		ASSERT_ACT(c.use_count(), ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_shared_ptr<Test> d(std::move(d));
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(!!b, ==, false, return false);
		ASSERT_ACT(!!c, ==, true, return false);
		ASSERT_ACT(!!d, ==, false, return false);
	}
	ASSERT_ACT(count, ==, 0, return false);

	return true;
}

bool test_weak_ptr()
{
	ASSERT_ACT(count, ==, 0, return false);
	{
		utttil::small_shared_ptr<Test> a;
		ASSERT_ACT(!!a, ==, false, return false);
		ASSERT_ACT(a.use_count(), ==, 0, return false);
		ASSERT_ACT(a.counter, ==, nullptr, return false);
		ASSERT_ACT(count, ==, 0, return false);

		utttil::small_weak_ptr<Test> aw;
		ASSERT_ACT(!!aw.lock(), ==, false, return false);
		ASSERT_ACT(aw.counter, ==, nullptr, return false);
		ASSERT_ACT(count, ==, 0, return false);

		a = utttil::make_small_shared<Test>(333);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 0, return false);
		ASSERT_ACT(count, ==, 1, return false);

		aw = utttil::small_weak_ptr<Test>(a);
		ASSERT_ACT(!!aw.lock(), ==, true, return false);
		ASSERT_ACT(aw.counter->weak_count, ==, 1, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 1, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_weak_ptr<Test> bw(a);
		ASSERT_ACT(!!aw.lock(), ==, true, return false);
		ASSERT_ACT(aw.counter->weak_count, ==, 2, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 2, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 2, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_weak_ptr<Test> cw(aw);
		ASSERT_ACT(!!aw.lock(), ==, true, return false);
		ASSERT_ACT(aw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!cw.lock(), ==, true, return false);
		ASSERT_ACT(cw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_weak_ptr<Test> dw(std::move(cw));
		ASSERT_ACT(!!aw.lock(), ==, true, return false);
		ASSERT_ACT(aw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!cw.lock(), ==, false, return false);
		ASSERT_ACT(cw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!dw.lock(), ==, true, return false);
		ASSERT_ACT(dw.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 3, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_weak_ptr<Test> ew;
		ew = aw;
		ASSERT_ACT(!!aw.lock(), ==, true, return false);
		ASSERT_ACT(aw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!cw.lock(), ==, false, return false);
		ASSERT_ACT(cw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!dw.lock(), ==, true, return false);
		ASSERT_ACT(dw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!ew.lock(), ==, true, return false);
		ASSERT_ACT(ew.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(count, ==, 1, return false);

		utttil::small_weak_ptr<Test> fw;
		fw = std::move(aw);
		ASSERT_ACT(!!aw.lock(), ==, false, return false);
		ASSERT_ACT(aw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!cw.lock(), ==, false, return false);
		ASSERT_ACT(cw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!dw.lock(), ==, true, return false);
		ASSERT_ACT(dw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!ew.lock(), ==, true, return false);
		ASSERT_ACT(ew.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!fw.lock(), ==, true, return false);
		ASSERT_ACT(fw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 1, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(count, ==, 1, return false);

		auto b = a;
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 2, return false);
		ASSERT_ACT(!!b, ==, true, return false);
		ASSERT_ACT(b.use_count(), ==, 2, return false);
		ASSERT_ACT(!!aw.lock(), ==, false, return false);
		ASSERT_ACT(aw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!bw.lock(), ==, true, return false);
		ASSERT_ACT(bw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!cw.lock(), ==, false, return false);
		ASSERT_ACT(cw.counter, ==, nullptr, return false);
		ASSERT_ACT(!!dw.lock(), ==, true, return false);
		ASSERT_ACT(dw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!ew.lock(), ==, true, return false);
		ASSERT_ACT(ew.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!fw.lock(), ==, true, return false);
		ASSERT_ACT(fw.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(!!a, ==, true, return false);
		ASSERT_ACT(a.use_count(), ==, 2, return false);
		ASSERT_ACT(a.counter->weak_count, ==, 4, return false);
		ASSERT_ACT(count, ==, 1, return false);
	}
	ASSERT_ACT(count, ==, 0, return false);

	return true;
}

bool test_shared_from_this()
{
	ASSERT_ACT(count, ==, 0, return false);
	{
		utttil::small_shared_ptr<TestSFT> a(new TestSFT(888));
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a->x, ==, 888, return false);

		auto b = a->shared();
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a.get(), ==, b.get(), return false);
		ASSERT_ACT(a->x, ==, 888, return false);
		ASSERT_ACT(b->x, ==, 888, return false);
	}
	{
		utttil::small_shared_ptr<TestSFT> a = utttil::make_small_shared<TestSFT>(777);
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a->x, ==, 777, return false);

		auto b = a->shared();
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a.get(), ==, b.get(), return false);
		ASSERT_ACT(a->x, ==, 777, return false);
		ASSERT_ACT(b->x, ==, 777, return false);
	}
	{
		utttil::small_shared_ptr<TestSFT> a(utttil::make_small_shared<TestSFT>(555));
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a->x, ==, 555, return false);

		auto b(a->shared());
		ASSERT_ACT(count, ==, 1, return false);
		ASSERT_ACT(a.get(), ==, b.get(), return false);
		ASSERT_ACT(a->x, ==, 555, return false);
		ASSERT_ACT(b->x, ==, 555, return false);
	}
	ASSERT_ACT(count, ==, 0, return false);
	return true;
}

bool test_size()
{
	ASSERT_ACT(sizeof(utttil::small_shared_ptr<int>), ==, 8, return false);
	ASSERT_ACT(sizeof(utttil::small_shared_ptr<Test>), ==, 8, return false);

	return true;
}

int main()
{
	bool success = true
		&& test_shared_ptr()
		&& test_shared_ptr_object()
		&& test_make_shared()
		&& test_move()
		&& test_weak_ptr()
		&& test_shared_from_this()
		&& test_size()
		;

	return success ? 0 : 1;
}