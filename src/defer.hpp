#pragma once


namespace apfsdedup {

template <typename F>
class Defer {
	F deferred;
	bool enabled{true};
public:
	Defer(F f): deferred{f} {}
	~Defer() { if (enabled) { deferred(); } }
	void cancel() { enabled = false; };
};

template <typename F>
Defer<F> defer(F f) {
	return Defer<F>(f);
}

} // namespace apfsdedup


