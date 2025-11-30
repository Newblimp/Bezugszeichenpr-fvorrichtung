#include <gtest/gtest.h>
#include "utils_core.h"
#include <map>
#include <unordered_map>

// Test StemVector basic operations
TEST(StemVectorTest, Creation_SingleElement) {
  StemVector vec = {L"lager"};
  ASSERT_EQ(vec.size(), 1);
  EXPECT_EQ(vec[0], L"lager");
}

TEST(StemVectorTest, Creation_MultipleElements) {
  StemVector vec = {L"erst", L"lager"};
  ASSERT_EQ(vec.size(), 2);
  EXPECT_EQ(vec[0], L"erst");
  EXPECT_EQ(vec[1], L"lager");
}

TEST(StemVectorTest, Equality_SameElements) {
  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"lager"};
  EXPECT_EQ(vec1, vec2);
}

TEST(StemVectorTest, Equality_DifferentElements) {
  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"motor"};
  EXPECT_NE(vec1, vec2);
}

TEST(StemVectorTest, Equality_DifferentLength) {
  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"erst", L"lager"};
  EXPECT_NE(vec1, vec2);
}

// Test StemVectorHash
TEST(StemVectorHashTest, ConsistentHash_SameVector) {
  StemVector vec = {L"lager"};
  StemVectorHash hasher;

  size_t hash1 = hasher(vec);
  size_t hash2 = hasher(vec);

  EXPECT_EQ(hash1, hash2);
}

TEST(StemVectorHashTest, ConsistentHash_EqualVectors) {
  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"lager"};
  StemVectorHash hasher;

  EXPECT_EQ(hasher(vec1), hasher(vec2));
}

TEST(StemVectorHashTest, DifferentHash_DifferentVectors) {
  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"motor"};
  StemVectorHash hasher;

  // Hashes should be different (with high probability)
  EXPECT_NE(hasher(vec1), hasher(vec2));
}

TEST(StemVectorHashTest, DifferentHash_MultiWord) {
  StemVector vec1 = {L"erst", L"lager"};
  StemVector vec2 = {L"zweit", L"lager"};
  StemVectorHash hasher;

  EXPECT_NE(hasher(vec1), hasher(vec2));
}

TEST(StemVectorHashTest, UnorderedMap_Usage) {
  std::unordered_map<StemVector, int, StemVectorHash> map;

  StemVector key1 = {L"lager"};
  StemVector key2 = {L"motor"};

  map[key1] = 10;
  map[key2] = 20;

  EXPECT_EQ(map[key1], 10);
  EXPECT_EQ(map[key2], 20);
  EXPECT_EQ(map.size(), 2);
}

TEST(StemVectorHashTest, UnorderedMap_FindByEqualKey) {
  std::unordered_map<StemVector, int, StemVectorHash> map;

  StemVector key1 = {L"lager"};
  map[key1] = 42;

  StemVector key2 = {L"lager"}; // Equal but different instance
  EXPECT_EQ(map[key2], 42);
}

// Test BZComparatorForMap
TEST(BZComparatorTest, NumericOrder_Simple) {
  BZComparatorForMap comp;

  EXPECT_TRUE(comp(L"1", L"2"));
  EXPECT_FALSE(comp(L"2", L"1"));
  EXPECT_FALSE(comp(L"1", L"1"));
}

TEST(BZComparatorTest, NumericOrder_MultiDigit) {
  BZComparatorForMap comp;

  EXPECT_TRUE(comp(L"2", L"10"));
  EXPECT_TRUE(comp(L"9", L"10"));
  EXPECT_TRUE(comp(L"10", L"100"));
}

TEST(BZComparatorTest, NumericOrder_WithLetters) {
  BZComparatorForMap comp;

  EXPECT_TRUE(comp(L"10", L"10a"));
  EXPECT_TRUE(comp(L"10a", L"10b"));
  EXPECT_TRUE(comp(L"10a", L"11"));
}

TEST(BZComparatorTest, NumericOrder_WithApostrophe) {
  BZComparatorForMap comp;

  EXPECT_TRUE(comp(L"10", L"10'"));
  EXPECT_TRUE(comp(L"10'", L"10a"));
}

TEST(BZComparatorTest, Map_SortingOrder) {
  std::map<std::wstring, int, BZComparatorForMap> bzMap;

  bzMap[L"10"] = 1;
  bzMap[L"2"] = 2;
  bzMap[L"10a"] = 3;
  bzMap[L"1"] = 4;
  bzMap[L"100"] = 5;

  std::vector<std::wstring> keys;
  for (const auto& [key, value] : bzMap) {
    keys.push_back(key);
  }

  ASSERT_EQ(keys.size(), 5);
  EXPECT_EQ(keys[0], L"1");
  EXPECT_EQ(keys[1], L"2");
  EXPECT_EQ(keys[2], L"10");
  EXPECT_EQ(keys[3], L"10a");
  EXPECT_EQ(keys[4], L"100");
}

TEST(BZComparatorTest, Map_ComplexReferences) {
  std::map<std::wstring, int, BZComparatorForMap> bzMap;

  bzMap[L"10'"] = 1;
  bzMap[L"10a"] = 2;
  bzMap[L"10"] = 3;
  bzMap[L"11"] = 4;
  bzMap[L"9"] = 5;

  std::vector<std::wstring> keys;
  for (const auto& [key, value] : bzMap) {
    keys.push_back(key);
  }

  ASSERT_EQ(keys.size(), 5);
  EXPECT_EQ(keys[0], L"9");
  EXPECT_EQ(keys[1], L"10");
  EXPECT_EQ(keys[2], L"10'");
  EXPECT_EQ(keys[3], L"10a");
  EXPECT_EQ(keys[4], L"11");
}

// Test edge cases
TEST(UtilsEdgeCasesTest, StemVector_EmptyVector) {
  StemVector vec;
  EXPECT_TRUE(vec.empty());
}

TEST(UtilsEdgeCasesTest, StemVector_EmptyString) {
  StemVector vec = {L""};
  ASSERT_EQ(vec.size(), 1);
  EXPECT_TRUE(vec[0].empty());
}

TEST(UtilsEdgeCasesTest, BZComparator_EmptyStrings) {
  BZComparatorForMap comp;

  EXPECT_FALSE(comp(L"", L""));
  EXPECT_TRUE(comp(L"", L"1"));
  EXPECT_FALSE(comp(L"1", L""));
}

TEST(UtilsEdgeCasesTest, BZComparator_NonNumeric) {
  BZComparatorForMap comp;

  // Lexicographic comparison for non-numeric strings
  EXPECT_TRUE(comp(L"a", L"b"));
  EXPECT_FALSE(comp(L"b", L"a"));
}

// Test performance characteristics
TEST(UtilsPerformanceTest, StemVectorHash_LargeMap) {
  std::unordered_map<StemVector, int, StemVectorHash> map;

  // Insert many elements
  for (int i = 0; i < 1000; ++i) {
    StemVector key = {std::to_wstring(i)};
    map[key] = i;
  }

  EXPECT_EQ(map.size(), 1000);

  // Verify lookup
  StemVector lookup = {L"500"};
  EXPECT_EQ(map[lookup], 500);
}

TEST(UtilsPerformanceTest, BZComparator_LargeMap) {
  std::map<std::wstring, int, BZComparatorForMap> map;

  // Insert many elements
  for (int i = 1; i <= 100; ++i) {
    map[std::to_wstring(i)] = i;
  }

  EXPECT_EQ(map.size(), 100);

  // Verify ordering
  int prev = 0;
  for (const auto& [key, value] : map) {
    EXPECT_EQ(value, prev + 1);
    prev = value;
  }
}

// Test hash collision behavior
TEST(StemVectorHashTest, CollisionResistance_SimilarStrings) {
  StemVectorHash hasher;

  StemVector vec1 = {L"lager"};
  StemVector vec2 = {L"lagen"};
  StemVector vec3 = {L"lager", L"motor"};

  size_t hash1 = hasher(vec1);
  size_t hash2 = hasher(vec2);
  size_t hash3 = hasher(vec3);

  // All should have different hashes (with high probability)
  EXPECT_NE(hash1, hash2);
  EXPECT_NE(hash1, hash3);
  EXPECT_NE(hash2, hash3);
}
