#include "sdsl/wavelet_trees.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <algorithm> // for std::min
#include <random>

namespace
{

using namespace sdsl;
using namespace std;

typedef int_vector<>::size_type size_type;

string test_file;
string temp_file;
bool in_memory;

template<class T>
class WtByteTest : public ::testing::Test { };

using testing::Types;

typedef Types<
wt_pc<balanced_shape>
,wt_blcd<rrr_vector<63>>
                      ,wt_blcd<bit_vector_il<>>
                      ,wt_blcd<bit_vector>
                      ,wt_huff<bit_vector_il<>>
                      ,wt_huff<bit_vector, rank_support_v<>>
                      ,wt_huff<bit_vector, rank_support_v5<>>
                      ,wt_huff<rrr_vector<63>>
                      ,wt_rlmn<>
                      ,wt_rlmn<bit_vector>
                      ,wt_gmr_rs<>
                      ,wt_hutu<bit_vector_il<>>
                      ,wt_hutu<bit_vector, rank_support_v<>>
                      ,wt_hutu<bit_vector, rank_support_v5<>>
                      ,wt_hutu<rrr_vector<63>>
                      > Implementations;

TYPED_TEST_CASE(WtByteTest, Implementations);

TYPED_TEST(WtByteTest, CreateAndStoreTest)
{
    TypeParam wt;
    construct(wt, test_file, 1);
    bool success = store_to_file(wt, temp_file);
    ASSERT_EQ(true, success);
}

//! Test sigma
TYPED_TEST(WtByteTest, Sigma)
{
    TypeParam wt;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    ASSERT_EQ(text.size(), wt.size());
    bit_vector occur(256, 0);
    uint16_t sigma = 0;
    for (size_type j=0; j<text.size(); ++j) {
        if (!occur[(unsigned char)text[j]]) {
            occur[(unsigned char)text[j]] = 1;
            ++sigma;
        }
    }
    ASSERT_EQ(sigma, wt.sigma);
}

template<class t_wt>
void compare_wt(const int_vector<8>& text, const t_wt& wt)
{
    ASSERT_EQ(text.size(), wt.size());
    for (size_type j=0; j<text.size(); ++j) {
        ASSERT_EQ((typename t_wt::value_type)text[j], wt[j])<<" j="<<j;
    }
}

//! Test Access method, Copy-construtor, Move-constructor, Copy-assign and Move-assign
TYPED_TEST(WtByteTest, AccessCopyMoveAndSwap)
{
    TypeParam wt;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    compare_wt(text, wt);

    // Copy-constructor
    TypeParam wt2(wt);
    compare_wt(text, wt2);

    // Move-constructor
    TypeParam wt3(std::move(wt2));
    compare_wt(text, wt3);

    // Copy-Assign
    TypeParam wt4;
    wt4 = wt3;
    compare_wt(text, wt4);

    // Move-Assign
    TypeParam wt5;
    wt5 = std::move(wt4);
    compare_wt(text, wt5);

    // Swap
    TypeParam wt6;
    wt6.swap(wt5);
    compare_wt(text, wt6);
}

//! Test rank methods
TYPED_TEST(WtByteTest, Rank)
{
    TypeParam wt;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));

    vector<size_type> cnt(256, 0);
    ASSERT_EQ(text.size(), wt.size());
    for (size_type j=0; j < wt.size(); ++j) {
        cnt[text[j]]++;
        ASSERT_EQ(cnt[text[j]], wt.rank(j+1, text[j]))<< " j = "<<j<<" text[j]"<<text[j];
    }
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> distribution(0, wt.size());
    auto dice = bind(distribution, rng);
    // Do random queries for all characters that do not occur in the string
    for (size_type j=0; j<cnt.size(); ++j) {
        if (cnt[j] == 0) {
            for (size_type k=0; k<1000; ++k) {
                size_type pos = dice();
                ASSERT_EQ((size_type)0, wt.rank(pos, (unsigned char)j))<<" pos="<<pos;
            }
        }
    }
    // Test rank(size(), c) for each character c
    for (size_type c=0; c < 256; ++c) {
        ASSERT_EQ(cnt[c], wt.rank(wt.size(), (unsigned char)c))<<" c="<<c;
    }
}

//! Test select methods
TYPED_TEST(WtByteTest, Select)
{
    TypeParam wt;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    vector<size_type> cnt(256, 0);
    ASSERT_EQ(text.size(), wt.size());
    for (size_type j=0; j<text.size(); ++j) {
        cnt[text[j]]++;
        ASSERT_EQ(j, wt.select(cnt[text[j]], text[j]))<< " j = "<<j<<" text[j] = "<<text[j];
    }
}

//! Test inverse select method
TYPED_TEST(WtByteTest, InverseSelect)
{
    TypeParam wt;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    std::vector<size_type> cnt(256, 0);
    ASSERT_EQ(text.size(), wt.size());
    for (size_type j=0; j<text.size(); ++j) {
        auto rc = wt.inverse_select(j);
        ASSERT_EQ(cnt[text[j]], rc.first);
        ASSERT_EQ(text[j], rc.second);
        cnt[text[j]]++;
    }
}

template<class t_wt>
void
test_interval_symbols(typename std::enable_if<!(has_node_type<t_wt>::value),
                      t_wt>::type&)
{
    // interval_symbols not implemented
}

template<class t_wt>
void
test_interval_symbols(typename std::enable_if<has_node_type<t_wt>::value,
                      t_wt>::type& wt)
{

    typedef typename t_wt::value_type value_type;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> distribution(0, wt.size());
    auto dice = bind(distribution, rng);
    size_type k;
    std::vector<value_type> cs(wt.sigma);
    std::vector<size_type> rank_c_i(wt.sigma);
    std::vector<size_type> rank_c_j(wt.sigma);
    for (size_type t=0; t<(wt.size()/100+100); ++t) {
        size_type i = dice(), j = dice();
        if (i<j) {
            std::swap(j,i);
        }
        interval_symbols(wt, i, j, k, cs, rank_c_i, rank_c_j);

        size_type symbols = (j-i);
        for (size_type m = 0; m<k; ++m) {
            ASSERT_EQ(wt.rank(i, cs[m]), rank_c_i[m]);
            ASSERT_EQ(wt.rank(j, cs[m]), rank_c_j[m]);
            ASSERT_LT((size_type)0, rank_c_j[m]-rank_c_i[m]);
            symbols -= (rank_c_j[m]-rank_c_i[m]);
            if (m>0 and t_wt::lex_ordered) {
                ASSERT_LT(cs[m-1],cs[m]);
            }
        }

        ASSERT_EQ((size_type)0, symbols);
        if (!t_wt::lex_ordered) {
            sort(cs.begin(), cs.begin()+k);
            for (size_type m=1; m<k; m++) {
                ASSERT_LT(cs[m-1], cs[m]);
            }
        }
    }
}

//! Test interval symbols method
TYPED_TEST(WtByteTest, IntervalSymbols)
{
    TypeParam wt;
    test_interval_symbols<TypeParam>(wt);
}

template<class t_wt>
void
test_lex_count(typename std::enable_if<!(t_wt::lex_ordered), t_wt>::type&)
{
    // lex_count not implemented
}

template<class t_wt>
void
test_lex_count(typename std::enable_if<t_wt::lex_ordered, t_wt>::type& wt)
{
    typedef typename t_wt::value_type value_type;
    ASSERT_EQ(true, load_from_file(wt, temp_file));
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    if (wt.size()) {
        std::mt19937_64 rng;
        std::uniform_int_distribution<uint64_t> distribution(0, wt.size());
        auto dice = bind(distribution, rng);
        for (size_type t=0; t<1000; ++t) {
            size_type i = dice();
            size_type j = dice();
            if (j<i) {
                std::swap(j,i);
            }
            std::vector<size_type> rank_c_i_n(256,0);
            std::vector<size_type> rank_c_j_n(256,0);
            for (size_type c=0; c<256; ++c) {
                rank_c_i_n[c] = wt.rank(i,(value_type)c);
                rank_c_j_n[c] = wt.rank(j,(value_type)c);
            }
            size_type num_i_s = 0;
            size_type num_j_s = 0;
            size_type num_c = 0;
            size_type num_s = 0;
            size_type num_g = j-i;
            for (size_type c=0; c<256; ++c) {
                // Test lex_count
                num_s += num_c;
                num_c = rank_c_j_n[c]-rank_c_i_n[c];
                num_g -= num_c;
                auto res = wt.lex_count(i, j, (value_type)c);
                ASSERT_EQ(rank_c_i_n[c], std::get<0>(res));
                ASSERT_EQ(num_s, std::get<1>(res));
                ASSERT_EQ(num_g, std::get<2>(res));
                // Test lex_smaller_count
                auto res2 = wt.lex_smaller_count(i, (value_type)c);
                ASSERT_EQ(rank_c_i_n[c], std::get<0>(res2)) << "lex_smaller_count(" << i << "," << c << ")";
                ASSERT_EQ(num_i_s, std::get<1>(res2)) << "lex_smaller_count(" << i << "," << c << ")";
                num_i_s += rank_c_i_n[c];
                auto res3 = wt.lex_smaller_count(j, (value_type)c);
                ASSERT_EQ(rank_c_j_n[c], std::get<0>(res3)) << "lex_smaller_count(" << i << "," << c << ")";
                ASSERT_EQ(num_j_s, std::get<1>(res3)) << "lex_smaller_count(" << i << "," << c << ")";
                num_j_s += rank_c_j_n[c];
            }
        }
    }
}

//! Test lex_count method
TYPED_TEST(WtByteTest, LexCount)
{
    TypeParam wt;
    test_lex_count<TypeParam>(wt);
}

TYPED_TEST(WtByteTest, CreatePartiallyTest)
{
    int_vector_buffer<8> text_buf(test_file, std::ios::in, 1024*1024, 8, true);
    int_vector<8> text;
    ASSERT_EQ(true, load_vector_from_file(text, test_file, 1));
    size_type n = min(text.size(), (size_type)50);
    text.resize(n);
    TypeParam wt(text_buf, n);
    compare_wt(text, wt);
}

TYPED_TEST(WtByteTest, DeleteTest)
{
    sdsl::remove(temp_file);
}

}  // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 3) {
        // LCOV_EXCL_START
        cout << "Usage: " << argv[0] << " test_file temp_file [in-memory]" << endl;
        cout << " (1) Generates a WT out of test_file; stores it in temp_file." << endl;
        cout << "     If `in-memory` is specified, the in-memory construction is tested." << endl;
        cout << " (2) Performs tests." << endl;
        cout << " (3) Deletes temp_file." << endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    test_file    = argv[1];
    temp_file    = argv[2];
    in_memory    = argc > 3;
    if (in_memory) {
        int_vector<8> data;
        load_vector_from_file(data, test_file, 1);
        test_file = ram_file_name(test_file);
        store_to_plain_array<uint8_t>(data, test_file);
        temp_file = ram_file_name(temp_file);
    }
    return RUN_ALL_TESTS();
}
