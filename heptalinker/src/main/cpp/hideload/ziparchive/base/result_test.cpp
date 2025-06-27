/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android-base/result.h"
#include <utils/ErrorsMacros.h>
#include "android-base/errors.h"
#include "errno.h"

#include <istream>
#include <memory>
#include <string>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "android-base/result-gmock.h"

using namespace std::string_literals;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::StartsWith;

namespace android {
namespace base {

TEST(result, result_accessors) {
  Result<std::string> result = "success";
  ASSERT_RESULT_OK(result);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ("success", *result);
  EXPECT_EQ("success", result.value());

  EXPECT_EQ('s', result->data()[0]);
}

TEST(result, result_accessors_rvalue) {
  ASSERT_TRUE(Result<std::string>("success").ok());
  ASSERT_TRUE(Result<std::string>("success").has_value());

  EXPECT_EQ("success", *Result<std::string>("success"));
  EXPECT_EQ("success", Result<std::string>("success").value());

  EXPECT_EQ('s', Result<std::string>("success")->data()[0]);
}

TEST(result, result_void) {
  Result<void> ok = {};
  EXPECT_RESULT_OK(ok);
  ok.value();  // should not crash
  ASSERT_DEATH(ok.error(), "");

  Result<void> fail = Error() << "failure" << 1;
  EXPECT_FALSE(fail.ok());
  EXPECT_EQ("failure1", fail.error().message());
  EXPECT_EQ(0, fail.error().code());
  EXPECT_TRUE(ok != fail);
  ASSERT_DEATH(fail.value(), "");

  auto test = [](bool ok) -> Result<void> {
    if (ok) return {};
    else return Error() << "failure" << 1;
  };
  EXPECT_TRUE(test(true).ok());
  EXPECT_FALSE(test(false).ok());
  test(true).value();  // should not crash
  ASSERT_DEATH(test(true).error(), "");
  ASSERT_DEATH(test(false).value(), "");
  EXPECT_EQ("failure1", test(false).error().message());
}

TEST(result, result_error) {
  Result<void> result = Error() << "failure" << 1;
  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  EXPECT_EQ(0, result.error().code());
  EXPECT_EQ("failure1", result.error().message());
}

TEST(result, result_error_empty) {
  Result<void> result = Error();
  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  EXPECT_EQ(0, result.error().code());
  EXPECT_EQ("", result.error().message());
}

TEST(result, result_error_rvalue) {
  // Error() and ErrnoError() aren't actually used to create a Result<T> object.
  // Under the hood, they are an intermediate class that can be implicitly constructed into a
  // Result<T>.  This is needed both to create the ostream and because Error() itself, by
  // definition will not know what the type, T, of the underlying Result<T> object that it would
  // create is.

  auto MakeRvalueErrorResult = []() -> Result<void> { return Error() << "failure" << 1; };
  ASSERT_FALSE(MakeRvalueErrorResult().ok());
  ASSERT_FALSE(MakeRvalueErrorResult().has_value());

  EXPECT_EQ(0, MakeRvalueErrorResult().error().code());
  EXPECT_EQ("failure1", MakeRvalueErrorResult().error().message());
}

TEST(result, result_errno_error) {
  constexpr int test_errno = 6;
  errno = test_errno;
  Result<void> result = ErrnoError() << "failure" << 1;

  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  EXPECT_EQ(test_errno, result.error().code());
  EXPECT_EQ("failure1: "s + strerror(test_errno), result.error().message());
}

TEST(result, result_errno_error_no_text) {
  constexpr int test_errno = 6;
  errno = test_errno;
  Result<void> result = ErrnoError();

  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  EXPECT_EQ(test_errno, result.error().code());
  EXPECT_EQ(strerror(test_errno), result.error().message());
}

TEST(result, result_error_from_other_result) {
  auto error_text = "test error"s;
  Result<void> result = Error() << error_text;

  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  Result<std::string> result2 = result.error();

  ASSERT_FALSE(result2.ok());
  ASSERT_FALSE(result2.has_value());

  EXPECT_EQ(0, result2.error().code());
  EXPECT_EQ(error_text, result2.error().message());
}

TEST(result, result_error_through_ostream) {
  auto error_text = "test error"s;
  Result<void> result = Error() << error_text;

  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  Result<std::string> result2 = Error() << result.error();

  ASSERT_FALSE(result2.ok());
  ASSERT_FALSE(result2.has_value());

  EXPECT_EQ(0, result2.error().code());
  EXPECT_EQ(error_text, result2.error().message());
}

TEST(result, result_errno_error_through_ostream) {
  auto error_text = "test error"s;
  constexpr int test_errno = 6;
  errno = 6;
  Result<void> result = ErrnoError() << error_text;

  errno = 0;

  ASSERT_FALSE(result.ok());
  ASSERT_FALSE(result.has_value());

  Result<std::string> result2 = Error() << result.error();

  ASSERT_FALSE(result2.ok());
  ASSERT_FALSE(result2.has_value());

  EXPECT_EQ(test_errno, result2.error().code());
  EXPECT_EQ(error_text + ": " + strerror(test_errno), result2.error().message());
}

enum class CustomError { A, B };

struct CustomErrorWrapper {
  CustomErrorWrapper() : val_(CustomError::A) {}
  CustomErrorWrapper(const CustomError& e) : val_(e) {}
  CustomError value() const { return val_; }
  operator CustomError() const { return value(); }
  std::string print() const {
    switch (val_) {
      case CustomError::A:
        return "A";
      case CustomError::B:
        return "B";
    }
  }
  CustomError val_;
};

#define NewCustomError(e) Error<CustomErrorWrapper>(CustomError::e)

TEST(result, result_with_custom_errorcode) {
  Result<void, CustomError> ok = {};
  EXPECT_RESULT_OK(ok);
  ok.value();  // should not crash
  EXPECT_DEATH(ok.error(), "");

  auto error_text = "test error"s;
  Result<void, CustomError> err = NewCustomError(A) << error_text;

  EXPECT_FALSE(err.ok());
  EXPECT_FALSE(err.has_value());

  EXPECT_EQ(CustomError::A, err.error().code());
  EXPECT_EQ(error_text + ": A", err.error().message());
}

Result<std::string, CustomError> success_or_fail(bool success) {
  if (success)
    return "success";
  else
    return NewCustomError(A) << "fail";
}

TEST(result, constructor_forwarding) {
  auto result = Result<std::string>(std::in_place, 5, 'a');

  ASSERT_RESULT_OK(result);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ("aaaaa", *result);
}

TEST(result, unwrap_or_return) {
  auto f = [](bool success) -> Result<size_t, CustomError> {
    return OR_RETURN(success_or_fail(success)).size();
  };

  auto r = f(true);
  EXPECT_TRUE(r.ok());
  EXPECT_EQ(strlen("success"), *r);

  auto s = f(false);
  EXPECT_FALSE(s.ok());
  EXPECT_EQ(CustomError::A, s.error().code());
  EXPECT_EQ("fail: A", s.error().message());
}

TEST(result, unwrap_or_return_errorcode) {
  auto f = [](bool success) -> CustomError {
    // Note that we use the same OR_RETURN macro for different return types: Result<U, CustomError>
    // and CustomError.
    std::string val = OR_RETURN(success_or_fail(success));
    EXPECT_EQ("success", val);
    return CustomError::B;
  };

  auto r = f(true);
  EXPECT_EQ(CustomError::B, r);

  auto s = f(false);
  EXPECT_EQ(CustomError::A, s);
}

TEST(result, unwrap_or_fatal) {
  auto r = OR_FATAL(success_or_fail(true));
  EXPECT_EQ("success", r);

  EXPECT_DEATH(OR_FATAL(success_or_fail(false)), "fail: A");
}

TEST(result, unwrap_ambiguous_int) {
  const std::string firstSuccess{"a"};
  constexpr int secondSuccess = 5;
  auto enum_success_or_fail = [&](bool success) -> Result<std::string, StatusT> {
    if (success) return firstSuccess;
    return ResultError<StatusT>("Fail", 10);
  };
  auto f = [&](bool success) -> Result<int, StatusT> {
    auto val = OR_RETURN(enum_success_or_fail(success));
    EXPECT_EQ(firstSuccess, val);
    return secondSuccess;
  };

  auto r = f(true);
  ASSERT_TRUE(r.ok());
  EXPECT_EQ(r.value(), secondSuccess);
  auto s = f(false);
  ASSERT_TRUE(!s.ok());
  EXPECT_EQ(s.error().code(), 10);
}

TEST(result, unwrap_ambiguous_uint_conv) {
  const std::string firstSuccess{"a"};
  constexpr size_t secondSuccess = 5ull;
  auto enum_success_or_fail = [&](bool success) -> Result<std::string, StatusT> {
    if (success) return firstSuccess;
    return ResultError<StatusT>("Fail", 10);
  };

  auto f = [&](bool success) -> Result<size_t, StatusT> {
    auto val = OR_RETURN(enum_success_or_fail(success));
    EXPECT_EQ(firstSuccess, val);
    return secondSuccess;
  };

  auto r = f(true);
  ASSERT_TRUE(r.ok());
  EXPECT_EQ(r.value(), secondSuccess);
  auto s = f(false);
  ASSERT_TRUE(!s.ok());
  EXPECT_EQ(s.error().code(), 10);
}

struct IntConst {
    int val_;
    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, int>>>
    IntConst(T&& val) : val_(val) {}
    operator status_t() {return val_;}
};

TEST(result, unwrap_ambiguous_constructible) {
  constexpr int firstSuccess = 5;
  constexpr int secondSuccess = 7;
  struct A {
    A (int val) : val_(val) {}
    operator status_t() { return 0; }
    int val_;
  };
  // If this returns Result<A, ...> instead of Result<IntConst, ...>,
  // compilation fails unless we compile with c++20
  auto enum_success_or_fail = [&](bool success) -> Result<IntConst, StatusT, false> {
    if (success) return firstSuccess;
    return ResultError<StatusT, false>(10);
  };
  auto f = [&](bool success) -> Result<IntConst, StatusT, false> {
    auto val = OR_RETURN(enum_success_or_fail(success));
    EXPECT_EQ(firstSuccess, val.val_);
    return secondSuccess;
  };
  auto r = f(true);
  EXPECT_EQ(r.value().val_, secondSuccess);
  auto s = f(false);
  EXPECT_EQ(s.error().code(), 10);
}

struct Dangerous {};
struct ImplicitFromDangerous {
  ImplicitFromDangerous(Dangerous);
};
template <typename U>
struct Templated {
    U val_;
    template <typename T, typename=std::enable_if_t<std::is_convertible_v<T, U>>>
    Templated(T val) : val_(val) {}
};


TEST(result, dangerous_result_conversion) {
  ResultError<Dangerous, false> error {Dangerous{}};
  Result<Templated<Dangerous>, Dangerous, false> surprise {error};
  EXPECT_TRUE(!surprise.ok());
  Result<Templated<ImplicitFromDangerous>, Dangerous, false> surprise2 {error};
  EXPECT_TRUE(!surprise2.ok());
}

TEST(result, generic_convertible) {
  const std::string firstSuccess{"a"};
  struct A {};
  struct B {
    operator A() {return A{};}
  };

  auto enum_success_or_fail = [&](bool success) -> Result<std::string, B> {
    if (success) return firstSuccess;
    return ResultError<B>("Fail", B{});
  };
  auto f = [&](bool success) -> Result<A, B> {
    auto val = OR_RETURN(enum_success_or_fail(success));
    EXPECT_EQ(firstSuccess, val);
    return A{};
  };

  auto r = f(true);
  EXPECT_TRUE(r.ok());
  auto s = f(false);
  EXPECT_TRUE(!s.ok());
}

TEST(result, generic_exact) {
  const std::string firstSuccess{"a"};
  struct A {};
  auto enum_success_or_fail = [&](bool success) -> Result<std::string, A> {
    if (success) return firstSuccess;
    return ResultError<A>("Fail", A{});
  };
  auto f = [&](bool success) -> Result<A, A> {
    auto val = OR_RETURN(enum_success_or_fail(success));
    EXPECT_EQ(firstSuccess, val);
    return A{};
  };

  auto r = f(true);
  EXPECT_TRUE(r.ok());
  auto s = f(false);
  EXPECT_TRUE(!s.ok());
}

struct MyData {
  const int data;
  static int copy_constructed;
  static int move_constructed;
  explicit MyData(int d) : data(d) {}
  MyData(const MyData& other) : data(other.data) { copy_constructed++; }
  MyData(MyData&& other) : data(other.data) { move_constructed++; }
  MyData& operator=(const MyData&) = delete;
  MyData& operator=(MyData&&) = delete;
};

int MyData::copy_constructed = 0;
int MyData::move_constructed = 0;

TEST(result, unwrap_does_not_incur_additional_copying) {
  MyData::copy_constructed = 0;
  MyData::move_constructed = 0;
  auto f = []() -> Result<MyData> { return MyData{10}; };

  [&]() -> Result<void> {
    int data = OR_RETURN(f()).data;
    EXPECT_EQ(10, data);
    EXPECT_EQ(0, MyData::copy_constructed);
    // Moved once when MyData{10} is returned as Result<MyData> in the lambda f.
    // Moved once again when the variable d is constructed from OR_RETURN.
    EXPECT_EQ(2, MyData::move_constructed);
    return {};
  }();
}

TEST(result, supports_move_only_type) {
  auto f = [](bool success) -> Result<std::unique_ptr<std::string>> {
    if (success) return std::make_unique<std::string>("hello");
    return Error() << "error";
  };

  auto g = [&](bool success) -> Result<std::unique_ptr<std::string>> {
    auto r = OR_RETURN(f(success));
    EXPECT_EQ("hello", *(r.get()));
    return std::make_unique<std::string>("world");
  };

  auto s = g(true);
  EXPECT_RESULT_OK(s);
  EXPECT_EQ("world", *(s->get()));

  auto t = g(false);
  EXPECT_FALSE(t.ok());
  EXPECT_EQ("error", t.error().message());
}

TEST(result, unique_ptr) {
  using testing::Ok;

  auto return_unique_ptr = [](bool success) -> Result<std::unique_ptr<int>> {
    auto result = OR_RETURN(Result<std::unique_ptr<int>>(std::make_unique<int>(3)));
    if (!success) {
      return Error() << __func__ << " failed.";
    }
    return result;
  };
  Result<std::unique_ptr<int>> result1 = return_unique_ptr(false);
  ASSERT_THAT(result1, Not(Ok()));
  Result<std::unique_ptr<int>> result2 = return_unique_ptr(true);
  ASSERT_THAT(result2, Ok());
  EXPECT_EQ(**result2, 3);
}

TEST(result, void) {
  using testing::Ok;

  auto return_void = []() -> Result<void> {
    OR_RETURN(Result<void>());
    return {};
  };

  ASSERT_THAT(return_void(), Ok());
}

struct ConstructorTracker {
  static size_t constructor_called;
  static size_t copy_constructor_called;
  static size_t move_constructor_called;
  static size_t copy_assignment_called;
  static size_t move_assignment_called;

  template <typename T>
  ConstructorTracker(T&& string) : string(string) {
    ++constructor_called;
  }

  ConstructorTracker(const ConstructorTracker& ct) {
    ++copy_constructor_called;
    string = ct.string;
  }
  ConstructorTracker(ConstructorTracker&& ct) noexcept {
    ++move_constructor_called;
    string = std::move(ct.string);
  }
  ConstructorTracker& operator=(const ConstructorTracker& ct) {
    ++copy_assignment_called;
    string = ct.string;
    return *this;
  }
  ConstructorTracker& operator=(ConstructorTracker&& ct) noexcept {
    ++move_assignment_called;
    string = std::move(ct.string);
    return *this;
  }

  std::string string;
};

size_t ConstructorTracker::constructor_called = 0;
size_t ConstructorTracker::copy_constructor_called = 0;
size_t ConstructorTracker::move_constructor_called = 0;
size_t ConstructorTracker::copy_assignment_called = 0;
size_t ConstructorTracker::move_assignment_called = 0;

Result<ConstructorTracker> ReturnConstructorTracker(const std::string& in) {
  if (in.empty()) {
    return "literal string";
  }
  if (in == "test2") {
    return ConstructorTracker(in + in + "2");
  }
  ConstructorTracker result(in + " " + in);
  return result;
};

TEST(result, no_copy_on_return) {
  // If returning parameters that may be used to implicitly construct the type T of Result<T>,
  // then those parameters are forwarded to the construction of Result<T>.

  // If returning an prvalue or xvalue, it will be move constructed during the construction of
  // Result<T>.

  // This check ensures that that is the case, and particularly that no copy constructors
  // are called.

  auto result1 = ReturnConstructorTracker("");
  ASSERT_RESULT_OK(result1);
  EXPECT_EQ("literal string", result1->string);
  EXPECT_EQ(1U, ConstructorTracker::constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::move_constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_assignment_called);
  EXPECT_EQ(0U, ConstructorTracker::move_assignment_called);

  auto result2 = ReturnConstructorTracker("test2");
  ASSERT_RESULT_OK(result2);
  EXPECT_EQ("test2test22", result2->string);
  EXPECT_EQ(2U, ConstructorTracker::constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_constructor_called);
  EXPECT_EQ(1U, ConstructorTracker::move_constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_assignment_called);
  EXPECT_EQ(0U, ConstructorTracker::move_assignment_called);

  auto result3 = ReturnConstructorTracker("test3");
  ASSERT_RESULT_OK(result3);
  EXPECT_EQ("test3 test3", result3->string);
  EXPECT_EQ(3U, ConstructorTracker::constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_constructor_called);
  EXPECT_EQ(2U, ConstructorTracker::move_constructor_called);
  EXPECT_EQ(0U, ConstructorTracker::copy_assignment_called);
  EXPECT_EQ(0U, ConstructorTracker::move_assignment_called);
}

// Below two tests require that we do not hide the move constructor with our forwarding reference
// constructor.  This is done with by disabling the forwarding reference constructor if its first
// and only type is Result<T>.
TEST(result, result_result_with_success) {
  auto return_result_result_with_success = []() -> Result<Result<void>> { return Result<void>(); };
  auto result = return_result_result_with_success();
  ASSERT_RESULT_OK(result);
  ASSERT_RESULT_OK(*result);

  auto inner_result = result.value();
  ASSERT_RESULT_OK(inner_result);
}

TEST(result, result_result_with_failure) {
  auto return_result_result_with_error = []() -> Result<Result<void>> {
    return Result<void>(ResultError("failure string", 6));
  };
  auto result = return_result_result_with_error();
  ASSERT_RESULT_OK(result);
  ASSERT_FALSE(result->ok());
  EXPECT_EQ("failure string", (*result).error().message());
  EXPECT_EQ(6, (*result).error().code());
}

// This test requires that we disable the forwarding reference constructor if Result<T> is the
// *only* type that we are forwarding.  In otherwords, if we are forwarding Result<T>, int to
// construct a Result<T>, then we still need the constructor.
TEST(result, result_two_parameter_constructor_same_type) {
  struct TestStruct {
    TestStruct(int value) : value_(value) {}
    TestStruct(Result<TestStruct> result, int value) : value_(result->value_ * value) {}
    int value_;
  };

  auto return_test_struct = []() -> Result<TestStruct> {
    return Result<TestStruct>(std::in_place, Result<TestStruct>(std::in_place, 6), 6);
  };

  auto result = return_test_struct();
  ASSERT_RESULT_OK(result);
  EXPECT_EQ(36, result->value_);
}

TEST(result, die_on_access_failed_result) {
  Result<std::string> result = Error();
  ASSERT_DEATH(*result, "");
}

TEST(result, die_on_get_error_succesful_result) {
  Result<std::string> result = "success";
  ASSERT_DEATH(result.error(), "");
}

template <class CharT>
std::basic_ostream<CharT>& SetErrnoToTwo(std::basic_ostream<CharT>& ss) {
  errno = 2;
  return ss;
}

TEST(result, preserve_errno) {
  errno = 1;
  int old_errno = errno;
  Result<int> result = Error() << "Failed" << SetErrnoToTwo<char>;
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(old_errno, errno);

  errno = 1;
  old_errno = errno;
  Result<int> result2 = ErrnoError() << "Failed" << SetErrnoToTwo<char>;
  ASSERT_FALSE(result2.ok());
  EXPECT_EQ(old_errno, errno);
  EXPECT_EQ(old_errno, result2.error().code());
}

TEST(result, error_with_fmt) {
  Result<int> result = Errorf("{} {}!", "hello", "world");
  EXPECT_EQ("hello world!", result.error().message());

  result = Errorf("{} {}!", std::string("hello"), std::string("world"));
  EXPECT_EQ("hello world!", result.error().message());

  result = Errorf("{1} {0}!", "world", "hello");
  EXPECT_EQ("hello world!", result.error().message());

  result = Errorf("hello world!");
  EXPECT_EQ("hello world!", result.error().message());

  Result<int> result2 = Errorf("error occurred with {}", result.error());
  EXPECT_EQ("error occurred with hello world!", result2.error().message());

  constexpr int test_errno = 6;
  errno = test_errno;
  result = ErrnoErrorf("{} {}!", "hello", "world");
  EXPECT_EQ(test_errno, result.error().code());
  EXPECT_EQ("hello world!: "s + strerror(test_errno), result.error().message());
}

TEST(result, error_with_fmt_carries_errno) {
  constexpr int inner_errno = 6;
  errno = inner_errno;
  Result<int> inner_result = ErrnoErrorf("inner failure");
  errno = 0;
  EXPECT_EQ(inner_errno, inner_result.error().code());

  // outer_result is created with Errorf, but its error code is got from inner_result.
  Result<int> outer_result = Errorf("outer failure caused by {}", inner_result.error());
  EXPECT_EQ(inner_errno, outer_result.error().code());
  EXPECT_EQ("outer failure caused by inner failure: "s + strerror(inner_errno),
            outer_result.error().message());

  // now both result objects are created with ErrnoErrorf. errno from the inner_result
  // is not passed to outer_result.
  constexpr int outer_errno = 10;
  errno = outer_errno;
  outer_result = ErrnoErrorf("outer failure caused by {}", inner_result.error());
  EXPECT_EQ(outer_errno, outer_result.error().code());
  EXPECT_EQ("outer failure caused by inner failure: "s + strerror(inner_errno) + ": "s +
                strerror(outer_errno),
            outer_result.error().message());
}

TEST(result, errno_chaining_multiple) {
  constexpr int errno1 = 6;
  errno = errno1;
  Result<int> inner1 = ErrnoErrorf("error1");

  constexpr int errno2 = 10;
  errno = errno2;
  Result<int> inner2 = ErrnoErrorf("error2");

  // takes the error code of inner2 since its the last one.
  Result<int> outer = Errorf("two errors: {}, {}", inner1.error(), inner2.error());
  EXPECT_EQ(errno2, outer.error().code());
  EXPECT_EQ("two errors: error1: "s + strerror(errno1) + ", error2: "s + strerror(errno2),
            outer.error().message());
}

TEST(result, error_without_message) {
  constexpr bool include_message = false;
  Result<void, Errno, include_message> res = Error<Errno, include_message>(10);
  EXPECT_FALSE(res.ok());
  EXPECT_EQ(10, res.error().code());
  EXPECT_EQ(sizeof(int), sizeof(res.error()));
}

namespace testing {

class Listener : public ::testing::MatchResultListener {
 public:
  Listener() : MatchResultListener(&ss_) {}
  ~Listener() = default;
  std::string message() const { return ss_.str(); }

 private:
  std::stringstream ss_;
};

class ResultMatchers : public ::testing::Test {
 public:
  Result<int> result = 1;
  Result<int> error = Error(EBADF) << "error message";
  Listener listener;
};

TEST_F(ResultMatchers, ok_result) {
  EXPECT_TRUE(ExplainMatchResult(Ok(), result, &listener));
  EXPECT_THAT(listener.message(), Eq("result is OK"));
}

TEST_F(ResultMatchers, ok_error) {
  EXPECT_FALSE(ExplainMatchResult(Ok(), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("error is"));
  EXPECT_THAT(listener.message(), HasSubstr(error.error().message()));
  EXPECT_THAT(listener.message(), HasSubstr(strerror(error.error().code())));
}

TEST_F(ResultMatchers, not_ok_result) {
  EXPECT_FALSE(ExplainMatchResult(Not(Ok()), result, &listener));
  EXPECT_THAT(listener.message(), Eq("result is OK"));
}

TEST_F(ResultMatchers, not_ok_error) {
  EXPECT_TRUE(ExplainMatchResult(Not(Ok()), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("error is"));
  EXPECT_THAT(listener.message(), HasSubstr(error.error().message()));
  EXPECT_THAT(listener.message(), HasSubstr(strerror(error.error().code())));
}

TEST_F(ResultMatchers, has_value_result) {
  EXPECT_TRUE(ExplainMatchResult(HasValue(*result), result, &listener));
}

TEST_F(ResultMatchers, has_value_wrong_result) {
  EXPECT_FALSE(ExplainMatchResult(HasValue(*result + 1), result, &listener));
}

TEST_F(ResultMatchers, has_value_error) {
  EXPECT_FALSE(ExplainMatchResult(HasValue(*result), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("error is"));
  EXPECT_THAT(listener.message(), HasSubstr(error.error().message()));
  EXPECT_THAT(listener.message(), HasSubstr(strerror(error.error().code())));
}

TEST_F(ResultMatchers, has_error_code_result) {
  EXPECT_FALSE(ExplainMatchResult(HasError(WithCode(error.error().code())), result, &listener));
  EXPECT_THAT(listener.message(), Eq("result is OK"));
}

TEST_F(ResultMatchers, has_error_code_wrong_code) {
  EXPECT_FALSE(ExplainMatchResult(HasError(WithCode(error.error().code() + 1)), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("actual error is"));
  EXPECT_THAT(listener.message(), HasSubstr(strerror(error.error().code())));
}

TEST_F(ResultMatchers, has_error_code_correct_code) {
  EXPECT_TRUE(ExplainMatchResult(HasError(WithCode(error.error().code())), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("actual error is"));
  EXPECT_THAT(listener.message(), HasSubstr(strerror(error.error().code())));
}

TEST_F(ResultMatchers, has_error_message_result) {
  EXPECT_FALSE(
      ExplainMatchResult(HasError(WithMessage(error.error().message())), result, &listener));
  EXPECT_THAT(listener.message(), Eq("result is OK"));
}

TEST_F(ResultMatchers, has_error_message_wrong_message) {
  EXPECT_FALSE(ExplainMatchResult(HasError(WithMessage("foo")), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("actual error is"));
  EXPECT_THAT(listener.message(), HasSubstr(error.error().message()));
}

TEST_F(ResultMatchers, has_error_message_correct_message) {
  EXPECT_TRUE(ExplainMatchResult(HasError(WithMessage(error.error().message())), error, &listener));
  EXPECT_THAT(listener.message(), StartsWith("actual error is"));
  EXPECT_THAT(listener.message(), HasSubstr(error.error().message()));
}

}  // namespace testing
}  // namespace base
}  // namespace android
