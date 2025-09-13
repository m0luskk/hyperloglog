#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "dataset.h"

#include "hyperloglog.h"

START_TEST(str_hash_test) {
  const char* str = "hello world";
  struct option_hash_t h1 = hash(strlen(str), str);

  ck_assert(h1.has_value);
}
END_TEST

START_TEST(empty_str_hash_test) {
  const char* str = "";
  struct option_hash_t h1 = hash(strlen(str), str);

  ck_assert(!h1.has_value);
}
END_TEST

START_TEST(null_str_hash_test) {
  const char* str = nullptr;
  struct option_hash_t h1 = hash(0, str);

  ck_assert(!h1.has_value);
}
END_TEST

START_TEST(small_str_hash_test) {
  const char* str = "ys";
  struct option_hash_t h1 = hash(strlen(str), str);

  ck_assert(h1.has_value);
}
END_TEST

START_TEST(str_hash_determanistic_test) {
  const char* str1 = "hello world";
  const char* str2 = "hello world";
  struct option_hash_t h1 = hash(strlen(str1), str1);
  struct option_hash_t h2 = hash(strlen(str2), str2);

  ck_assert(h1.has_value);
  ck_assert(h2.has_value);

  auto h1_v = option_hash_t_unwrap(h1);
  auto h2_v = option_hash_t_unwrap(h2);

  ck_assert(h1_v == h2_v);
}
END_TEST

START_TEST(str_hash_collision_test) {
  const char* str1 = "hello world";
  const char* str2 = "hello world!";
  struct option_hash_t opt_h1 = hash(strlen(str1), str1);
  struct option_hash_t opt_h2 = hash(strlen(str2), str2);

  ck_assert(opt_h1.has_value);
  ck_assert(opt_h2.has_value);

  auto h1 = option_hash_t_unwrap(opt_h1);
  auto h2 = option_hash_t_unwrap(opt_h2);

  ck_assert(h1 != h2);
}
END_TEST

START_TEST(hll_create) {
  auto hll_opt = hyperloglog_create(3);
  ck_assert(hll_opt.has_value);

  auto hll_v = option_hll_unwrap(hll_opt);

  ck_assert(hll_v._initial_bits == 3);
  ck_assert(hll_v._register_count == 2 * 2 * 2);
  ck_assert(hll_v._registers != nullptr);

  hyperloglog_destroy(&hll_v);
}
END_TEST

START_TEST(hll_add_element) {
  auto hll_opt = hyperloglog_create(3);
  ck_assert(hll_opt.has_value);
  auto hll_v = option_hll_unwrap(hll_opt);

  const char* str = "привет!!!";

  hyperloglog_add_element(&hll_v, strlen(str), str);

  hyperloglog_destroy(&hll_v);
}
END_TEST

START_TEST(hll_cardinality) {
  auto hll_opt = hyperloglog_create(3);
  ck_assert(hll_opt.has_value);
  auto hll_v = option_hll_unwrap(hll_opt);

  const char* str = "hello";

  hyperloglog_add_element(&hll_v, strlen(str), str);

  double c =  hyperloglog_cardinality(&hll_v); // Should be 0.891

  printf("Cardinality of 1 element = %f\n", c);

  hyperloglog_destroy(&hll_v);
}
END_TEST

START_TEST(hll_data_test) {
  auto hll_opt = hyperloglog_create(7);
  ck_assert(hll_opt.has_value);
  auto hll_v = option_hll_unwrap(hll_opt);

  for (size_t i = 0; i < LARGE_DATASET_SIZE; ++i) {
    hyperloglog_add_element(&hll_v, strlen(large_names_dataset[i]), large_names_dataset[i]);
  }
  double c =  hyperloglog_cardinality(&hll_v);

  printf("dataset cardinality = %f\n", c);

  hyperloglog_destroy(&hll_v);
}
END_TEST

START_TEST(hll_small_test) {
  auto hll_opt = hyperloglog_create(7);
  ck_assert(hll_opt.has_value);
  auto hll_v = option_hll_unwrap(hll_opt);

  for (size_t i = 0; i < sizeof(small_names_dataset) / sizeof(small_names_dataset[0]); ++i) {
    hyperloglog_add_element(&hll_v, strlen(small_names_dataset[i]), small_names_dataset[i]);
  }
  double c =  hyperloglog_cardinality(&hll_v);

  printf("small dataset cardinality = %f\n", c);

  hyperloglog_destroy(&hll_v);
}
END_TEST

START_TEST(hll_int_test) {
  auto hll_opt = hyperloglog_create(7);
  ck_assert(hll_opt.has_value);
  auto hll_v = option_hll_unwrap(hll_opt);

  for (size_t i = 0; i < sizeof(int_dataset) / sizeof(int_dataset[0]); ++i) {
    hyperloglog_add_element(&hll_v, sizeof(int_dataset[0]), &int_dataset[i]);
  }
  double c =  hyperloglog_cardinality(&hll_v);

  printf("small dataset cardinality = %f\n", c);

  hyperloglog_destroy(&hll_v);
}
END_TEST

Suite* hll_suite() {
  Suite* s;
  TCase* tc_core;

  s = suite_create("hll");

  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, str_hash_test);
  tcase_add_test(tc_core, str_hash_determanistic_test);
  tcase_add_test(tc_core, empty_str_hash_test);
  tcase_add_test(tc_core, small_str_hash_test);
  tcase_add_test(tc_core, null_str_hash_test);
  tcase_add_test(tc_core, str_hash_collision_test);

  tcase_add_test(tc_core, hll_create);
  tcase_add_test(tc_core, hll_add_element);

  tcase_add_test(tc_core, hll_cardinality);
  
  tcase_add_test(tc_core, hll_data_test);
  tcase_add_test(tc_core, hll_small_test);
  tcase_add_test(tc_core, hll_int_test);
  
  suite_add_tcase(s, tc_core);

  return s;
}

int main() {
  int failed = 0;
  Suite* s;
  SRunner* sr;

  s = hll_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);

  failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return ( failed ? EXIT_SUCCESS : EXIT_FAILURE );
}
